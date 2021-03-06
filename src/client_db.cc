/*
 * DEBUG: section 00    Client Database
 * AUTHOR: Duane Wessels
 *
 * SQUID Web Proxy Cache          http://www.squid-cache.org/
 * ----------------------------------------------------------
 *
 *  Squid is the result of efforts by numerous individuals from
 *  the Internet community; see the CONTRIBUTORS file for full
 *  details.   Many organizations have provided support for Squid's
 *  development; see the SPONSORS file for full details.  Squid is
 *  Copyrighted (C) 2001 by the Regents of the University of
 *  California; see the COPYRIGHT file for full details.  Squid
 *  incorporates software developed and/or copyrighted by other
 *  sources; see the CREDITS file for full details.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 *
 */

#include "squid.h"
#include "base/RunnersRegistry.h"
#include "client_db.h"
#include "ClientInfo.h"
#include "event.h"
#include "format/Token.h"
#include "fqdncache.h"
#include "ip/Address.h"
#include "log/access_log.h"
#include "Mem.h"
#include "mgr/Registration.h"
#include "SquidConfig.h"
#include "SquidMath.h"
#include "SquidTime.h"
#include "StatCounters.h"
#include "Store.h"
#include "tools.h"

#if SQUID_SNMP
#include "snmp_core.h"
#endif

static hash_table *client_table = NULL;

static ClientInfo *clientdbAdd(const Ip::Address &addr);
static FREE clientdbFreeItem;
static void clientdbStartGC(void);
static void clientdbScheduledGC(void *);

#if USE_DELAY_POOLS
static int max_clients = 32768;
#else
static int max_clients = 32;
#endif

static int cleanup_running = 0;
static int cleanup_scheduled = 0;
static int cleanup_removed;

#if USE_DELAY_POOLS
#define CLIENT_DB_HASH_SIZE 65357
#else
#define CLIENT_DB_HASH_SIZE 467
#endif

static ClientInfo *

clientdbAdd(const Ip::Address &addr)
{
    ClientInfo *c;
    char *buf = static_cast<char*>(xmalloc(MAX_IPSTRLEN)); // becomes hash.key
    c = (ClientInfo *)memAllocate(MEM_CLIENT_INFO);
    debugs(77, 9, "ClientInfo constructed, this=" << c);
    c->hash.key = addr.toStr(buf,MAX_IPSTRLEN);
    c->addr = addr;
#if USE_DELAY_POOLS
    /* setup default values for client write limiter */
    c->writeLimitingActive=false;
    c->writeSpeedLimit=0;
    c->bucketSize = 0;
    c->firstTimeConnection=true;
    c->quotaQueue = NULL;
    c->rationedQuota = 0;
    c->rationedCount = 0;
    c->selectWaiting = false;
    c->eventWaiting = false;

    /* get current time */
    getCurrentTime();
    c->prevTime=current_dtime;/* put current time to have something sensible here */
#endif
    hash_join(client_table, &c->hash);
    ++statCounter.client_http.clients;

    if ((statCounter.client_http.clients > max_clients) && !cleanup_running && cleanup_scheduled < 2) {
        ++cleanup_scheduled;
        eventAdd("client_db garbage collector", clientdbScheduledGC, NULL, 90, 0);
    }

    return c;
}

static void
clientdbInit(void)
{
    if (client_table)
        return;

    client_table = hash_create((HASHCMP *) strcmp, CLIENT_DB_HASH_SIZE, hash_string);
}

class ClientDbRr: public RegisteredRunner
{
public:
    /* RegisteredRunner API */
    virtual void useConfig();
};
RunnerRegistrationEntry(ClientDbRr);

void
ClientDbRr::useConfig()
{
    clientdbInit();
    Mgr::RegisterAction("client_list", "Cache Client List", clientdbDump, 0, 1);
}

#if USE_DELAY_POOLS
/* returns ClientInfo for given IP addr
   Returns NULL if no such client (or clientdb turned off)
   (it is assumed that clientdbEstablished will be called before and create client record if needed)
*/
ClientInfo * clientdbGetInfo(const Ip::Address &addr)
{
    char key[MAX_IPSTRLEN];
    ClientInfo *c;

    if (!Config.onoff.client_db)
        return NULL;

    addr.toStr(key,MAX_IPSTRLEN);

    c = (ClientInfo *) hash_lookup(client_table, key);
    if (c==NULL) {
        debugs(77, DBG_IMPORTANT,"Client db does not contain information for given IP address "<<(const char*)key);
        return NULL;
    }
    return c;
}
#endif
void
clientdbUpdate(const Ip::Address &addr, LogTags ltype, AnyP::ProtocolType p, size_t size)
{
    char key[MAX_IPSTRLEN];
    ClientInfo *c;

    if (!Config.onoff.client_db)
        return;

    addr.toStr(key,MAX_IPSTRLEN);

    c = (ClientInfo *) hash_lookup(client_table, key);

    if (c == NULL)
        c = clientdbAdd(addr);

    if (c == NULL)
        debug_trap("clientdbUpdate: Failed to add entry");

    if (p == AnyP::PROTO_HTTP) {
        ++ c->Http.n_requests;
        ++ c->Http.result_hist[ltype];
        kb_incr(&c->Http.kbytes_out, size);

        if (logTypeIsATcpHit(ltype))
            kb_incr(&c->Http.hit_kbytes_out, size);
    } else if (p == AnyP::PROTO_ICP) {
        ++ c->Icp.n_requests;
        ++ c->Icp.result_hist[ltype];
        kb_incr(&c->Icp.kbytes_out, size);

        if (LOG_UDP_HIT == ltype)
            kb_incr(&c->Icp.hit_kbytes_out, size);
    }

    c->last_seen = squid_curtime;
}

/**
 * This function tracks the number of currently established connections
 * for a client IP address.  When a connection is accepted, call this
 * with delta = 1.  When the connection is closed, call with delta =
 * -1.  To get the current value, simply call with delta = 0.
 */
int
clientdbEstablished(const Ip::Address &addr, int delta)
{
    char key[MAX_IPSTRLEN];
    ClientInfo *c;

    if (!Config.onoff.client_db)
        return 0;

    addr.toStr(key,MAX_IPSTRLEN);

    c = (ClientInfo *) hash_lookup(client_table, key);

    if (c == NULL) {
        c = clientdbAdd(addr);
    }

    if (c == NULL)
        debug_trap("clientdbUpdate: Failed to add entry");

    c->n_established += delta;

    return c->n_established;
}

#define CUTOFF_SECONDS 3600
int

clientdbCutoffDenied(const Ip::Address &addr)
{
    char key[MAX_IPSTRLEN];
    int NR;
    int ND;
    double p;
    ClientInfo *c;

    if (!Config.onoff.client_db)
        return 0;

    addr.toStr(key,MAX_IPSTRLEN);

    c = (ClientInfo *) hash_lookup(client_table, key);

    if (c == NULL)
        return 0;

    /*
     * If we are in a cutoff window, we don't send a reply
     */
    if (squid_curtime - c->cutoff.time < CUTOFF_SECONDS)
        return 1;

    /*
     * Calculate the percent of DENIED replies since the last
     * cutoff time.
     */
    NR = c->Icp.n_requests - c->cutoff.n_req;

    if (NR < 150)
        NR = 150;

    ND = c->Icp.result_hist[LOG_UDP_DENIED] - c->cutoff.n_denied;

    p = 100.0 * ND / NR;

    if (p < 95.0)
        return 0;

    debugs(1, DBG_CRITICAL, "WARNING: Probable misconfigured neighbor at " << key);

    debugs(1, DBG_CRITICAL, "WARNING: " << ND << " of the last " << NR <<
           " ICP replies are DENIED");

    debugs(1, DBG_CRITICAL, "WARNING: No replies will be sent for the next " <<
           CUTOFF_SECONDS << " seconds");

    c->cutoff.time = squid_curtime;

    c->cutoff.n_req = c->Icp.n_requests;

    c->cutoff.n_denied = c->Icp.result_hist[LOG_UDP_DENIED];

    return 1;
}

void
clientdbDump(StoreEntry * sentry)
{
    const char *name;
    ClientInfo *c;
    int icp_total = 0;
    int icp_hits = 0;
    int http_total = 0;
    int http_hits = 0;
    storeAppendPrintf(sentry, "Cache Clients:\n");
    hash_first(client_table);

    while ((c = (ClientInfo *) hash_next(client_table))) {
        storeAppendPrintf(sentry, "Address: %s\n", hashKeyStr(&c->hash));
        if ( (name = fqdncache_gethostbyaddr(c->addr, 0)) ) {
            storeAppendPrintf(sentry, "Name:    %s\n", name);
        }
        storeAppendPrintf(sentry, "Currently established connections: %d\n",
                          c->n_established);
        storeAppendPrintf(sentry, "    ICP  Requests %d\n",
                          c->Icp.n_requests);

        for (LogTags l = LOG_TAG_NONE; l < LOG_TYPE_MAX; ++l) {
            if (c->Icp.result_hist[l] == 0)
                continue;

            icp_total += c->Icp.result_hist[l];

            if (LOG_UDP_HIT == l)
                icp_hits += c->Icp.result_hist[l];

            storeAppendPrintf(sentry, "        %-20.20s %7d %3d%%\n",LogTags_str[l], c->Icp.result_hist[l], Math::intPercent(c->Icp.result_hist[l], c->Icp.n_requests));
        }

        storeAppendPrintf(sentry, "    HTTP Requests %d\n", c->Http.n_requests);

        for (LogTags l = LOG_TAG_NONE; l < LOG_TYPE_MAX; ++l) {
            if (c->Http.result_hist[l] == 0)
                continue;

            http_total += c->Http.result_hist[l];

            if (logTypeIsATcpHit(l))
                http_hits += c->Http.result_hist[l];

            storeAppendPrintf(sentry,
                              "        %-20.20s %7d %3d%%\n",
                              LogTags_str[l],
                              c->Http.result_hist[l],
                              Math::intPercent(c->Http.result_hist[l], c->Http.n_requests));
        }

        storeAppendPrintf(sentry, "\n");
    }

    storeAppendPrintf(sentry, "TOTALS\n");
    storeAppendPrintf(sentry, "ICP : %d Queries, %d Hits (%3d%%)\n",
                      icp_total, icp_hits, Math::intPercent(icp_hits, icp_total));
    storeAppendPrintf(sentry, "HTTP: %d Requests, %d Hits (%3d%%)\n",
                      http_total, http_hits, Math::intPercent(http_hits, http_total));
}

static void
clientdbFreeItem(void *data)
{
    ClientInfo *c = (ClientInfo *)data;
    safe_free(c->hash.key);

#if USE_DELAY_POOLS
    if (CommQuotaQueue *q = c->quotaQueue) {
        q->clientInfo = NULL;
        delete q; // invalidates cbdata, cancelling any pending kicks
    }
#endif

    debugs(77, 9, "ClientInfo destructed, this=" << c);
    memFree(c, MEM_CLIENT_INFO);
}

void
clientdbFreeMemory(void)
{
    hashFreeItems(client_table, clientdbFreeItem);
    hashFreeMemory(client_table);
    client_table = NULL;
}

static void
clientdbScheduledGC(void *unused)
{
    cleanup_scheduled = 0;
    clientdbStartGC();
}

static void
clientdbGC(void *unused)
{
    static int bucket = 0;
    hash_link *link_next;

    link_next = hash_get_bucket(client_table, bucket++);

    while (link_next != NULL) {
        ClientInfo *c = (ClientInfo *)link_next;
        int age = squid_curtime - c->last_seen;
        link_next = link_next->next;

        if (c->n_established)
            continue;

        if (age < 24 * 3600 && c->Http.n_requests > 100)
            continue;

        if (age < 4 * 3600 && (c->Http.n_requests > 10 || c->Icp.n_requests > 10))
            continue;

        if (age < 5 * 60 && (c->Http.n_requests > 1 || c->Icp.n_requests > 1))
            continue;

        if (age < 60)
            continue;

        hash_remove_link(client_table, &c->hash);

        clientdbFreeItem(c);

        --statCounter.client_http.clients;

        ++cleanup_removed;
    }

    if (bucket < CLIENT_DB_HASH_SIZE)
        eventAdd("client_db garbage collector", clientdbGC, NULL, 0.15, 0);
    else {
        bucket = 0;
        cleanup_running = 0;
        max_clients = statCounter.client_http.clients * 3 / 2;

        if (!cleanup_scheduled) {
            cleanup_scheduled = 1;
            eventAdd("client_db garbage collector", clientdbScheduledGC, NULL, 6 * 3600, 0);
        }

        debugs(49, 2, "clientdbGC: Removed " << cleanup_removed << " entries");
    }
}

static void
clientdbStartGC(void)
{
    max_clients = statCounter.client_http.clients;
    cleanup_running = 1;
    cleanup_removed = 0;
    clientdbGC(NULL);
}

#if SQUID_SNMP

Ip::Address *
client_entry(Ip::Address *current)
{
    ClientInfo *c = NULL;
    char key[MAX_IPSTRLEN];

    if (current) {
        current->toStr(key,MAX_IPSTRLEN);
        hash_first(client_table);
        while ((c = (ClientInfo *) hash_next(client_table))) {
            if (!strcmp(key, hashKeyStr(&c->hash)))
                break;
        }

        c = (ClientInfo *) hash_next(client_table);
    } else {
        hash_first(client_table);
        c = (ClientInfo *) hash_next(client_table);
    }

    hash_last(client_table);

    if (c)
        return (&c->addr);
    else
        return (NULL);

}

variable_list *
snmp_meshCtblFn(variable_list * Var, snint * ErrP)
{
    char key[MAX_IPSTRLEN];
    ClientInfo *c = NULL;
    Ip::Address keyIp;

    *ErrP = SNMP_ERR_NOERROR;
    MemBuf tmp;
    debugs(49, 6, HERE << "Current : length=" << Var->name_length << ": " << snmpDebugOid(Var->name, Var->name_length, tmp));
    if (Var->name_length == 16) {
        oid2addr(&(Var->name[12]), keyIp, 4);
    } else if (Var->name_length == 28) {
        oid2addr(&(Var->name[12]), keyIp, 16);
    } else {
        *ErrP = SNMP_ERR_NOSUCHNAME;
        return NULL;
    }

    keyIp.toStr(key, sizeof(key));
    debugs(49, 5, HERE << "[" << key << "] requested!");
    c = (ClientInfo *) hash_lookup(client_table, key);

    if (c == NULL) {
        debugs(49, 5, HERE << "not found.");
        *ErrP = SNMP_ERR_NOSUCHNAME;
        return NULL;
    }

    variable_list *Answer = NULL;
    int aggr = 0;

    switch (Var->name[LEN_SQ_NET + 2]) {

    case MESH_CTBL_ADDR_TYPE: {
        int ival;
        ival = c->addr.isIPv4() ? INETADDRESSTYPE_IPV4 : INETADDRESSTYPE_IPV6 ;
        Answer = snmp_var_new_integer(Var->name, Var->name_length,
                                      ival, SMI_INTEGER);
    }
    break;

    case MESH_CTBL_ADDR: {
        Answer = snmp_var_new(Var->name, Var->name_length);
        // InetAddress doesn't have its own ASN.1 type,
        // like IpAddr does (SMI_IPADDRESS)
        // See: rfc4001.txt
        Answer->type = ASN_OCTET_STR;
        char client[MAX_IPSTRLEN];
        c->addr.toStr(client,MAX_IPSTRLEN);
        Answer->val_len = strlen(client);
        Answer->val.string =  (u_char *) xstrdup(client);
    }
    break;
    case MESH_CTBL_HTBYTES:
        Answer = snmp_var_new_integer(Var->name, Var->name_length,
                                      (snint) c->Http.kbytes_out.kb,
                                      SMI_COUNTER32);
        break;

    case MESH_CTBL_HTREQ:
        Answer = snmp_var_new_integer(Var->name, Var->name_length,
                                      (snint) c->Http.n_requests,
                                      SMI_COUNTER32);
        break;

    case MESH_CTBL_HTHITS:
        aggr = 0;

        for (LogTags l = LOG_TAG_NONE; l < LOG_TYPE_MAX; ++l) {
            if (logTypeIsATcpHit(l))
                aggr += c->Http.result_hist[l];
        }

        Answer = snmp_var_new_integer(Var->name, Var->name_length,
                                      (snint) aggr,
                                      SMI_COUNTER32);
        break;

    case MESH_CTBL_HTHITBYTES:
        Answer = snmp_var_new_integer(Var->name, Var->name_length,
                                      (snint) c->Http.hit_kbytes_out.kb,
                                      SMI_COUNTER32);
        break;

    case MESH_CTBL_ICPBYTES:
        Answer = snmp_var_new_integer(Var->name, Var->name_length,
                                      (snint) c->Icp.kbytes_out.kb,
                                      SMI_COUNTER32);
        break;

    case MESH_CTBL_ICPREQ:
        Answer = snmp_var_new_integer(Var->name, Var->name_length,
                                      (snint) c->Icp.n_requests,
                                      SMI_COUNTER32);
        break;

    case MESH_CTBL_ICPHITS:
        aggr = c->Icp.result_hist[LOG_UDP_HIT];
        Answer = snmp_var_new_integer(Var->name, Var->name_length,
                                      (snint) aggr,
                                      SMI_COUNTER32);
        break;

    case MESH_CTBL_ICPHITBYTES:
        Answer = snmp_var_new_integer(Var->name, Var->name_length,
                                      (snint) c->Icp.hit_kbytes_out.kb,
                                      SMI_COUNTER32);
        break;

    default:
        *ErrP = SNMP_ERR_NOSUCHNAME;
        debugs(49, 5, "snmp_meshCtblFn: illegal column.");
        break;
    }

    return Answer;
}

#endif /*SQUID_SNMP */
