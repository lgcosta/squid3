/*
 * STUB file for the pconn.cc API
 */
#include "squid.h"
#include "comm/Connection.h"
#include "pconn.h"

#define STUB_API "pconn.cc"
#include "tests/STUB.h"

IdleConnList::IdleConnList(const char *key, PconnPool *parent) STUB
IdleConnList::~IdleConnList() STUB
void IdleConnList::push(const Comm::ConnectionPointer &conn) STUB
Comm::ConnectionPointer IdleConnList::findUseable(const Comm::ConnectionPointer &key) STUB_RETVAL(Comm::ConnectionPointer())
void IdleConnList::clearHandlers(const Comm::ConnectionPointer &conn) STUB
PconnPool::PconnPool(const char *) STUB
PconnPool::~PconnPool() STUB
void PconnPool::moduleInit() STUB
void PconnPool::push(const Comm::ConnectionPointer &serverConn, const char *domain) STUB
Comm::ConnectionPointer PconnPool::pop(const Comm::ConnectionPointer &destLink, const char *domain, bool retriable) STUB_RETVAL(Comm::ConnectionPointer())
void PconnPool::count(int uses) STUB
void PconnPool::noteUses(int) STUB
void PconnPool::dumpHist(StoreEntry *e) const STUB
void PconnPool::dumpHash(StoreEntry *e) const STUB
void PconnPool::unlinkList(IdleConnList *list) STUB
PconnModule * PconnModule::GetInstance() STUB_RETVAL(NULL)
void PconnModule::DumpWrapper(StoreEntry *e) STUB
PconnModule::PconnModule() STUB
void PconnModule::registerWithCacheManager(void) STUB
void PconnModule::add(PconnPool *) STUB
void PconnModule::dump(StoreEntry *) STUB
