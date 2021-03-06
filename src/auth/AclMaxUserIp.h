/*
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
 *
 * Copyright (c) 2003, Robert Collins <robertc@squid-cache.org>
 */

#ifndef SQUID_ACLMAXUSERIP_H
#define SQUID_ACLMAXUSERIP_H

#if USE_AUTH

#include "acl/Acl.h"
#include "acl/Checklist.h"
#include "auth/UserRequest.h"

/// \ingroup ACLAPI
class ACLMaxUserIP : public ACL
{
public:
    MEMPROXY_CLASS(ACLMaxUserIP);

    ACLMaxUserIP(char const *theClass);
    ACLMaxUserIP(ACLMaxUserIP const &old);
    ~ACLMaxUserIP();
    ACLMaxUserIP &operator =(ACLMaxUserIP const &);

    virtual ACL *clone() const;
    virtual char const *typeString() const;
    virtual void parse();
    virtual int match(ACLChecklist *cl);
    virtual wordlist *dump() const;
    virtual bool empty() const;
    virtual bool valid() const;
    virtual bool requiresRequest() const {return true;}

    int getMaximum() const {return maximum;}

    bool getStrict() const {return flags.isSet(ACL_F_STRICT);}

private:
    static Prototype RegistryProtoype;
    static ACLMaxUserIP RegistryEntry_;
    static ACLFlag SupportedFlags[];

    int match(Auth::UserRequest::Pointer auth_user_request, Ip::Address const &src_addr);
    char const *class_;
    int maximum;
};

MEMPROXY_CLASS_INLINE(ACLMaxUserIP);

#endif /* USE_AUTH */
#endif /* SQUID_ACLMAXUSERIP_H */
