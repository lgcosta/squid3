#ifndef SQUID_ACLSSL_ERROR_H
#define SQUID_ACLSSL_ERROR_H
#include "acl/Strategised.h"
#include "acl/Strategy.h"
#include "ssl/support.h"

class ACLSslErrorStrategy : public ACLStrategy<const Ssl::CertErrors *>
{

public:
    virtual int match (ACLData<MatchType> * &, ACLFilledChecklist *, ACLFlags &);
    static ACLSslErrorStrategy *Instance();
    /* Not implemented to prevent copies of the instance. */
    /* Not private to prevent brain dead g+++ warnings about
     * private constructors with no friends */
    ACLSslErrorStrategy(ACLSslErrorStrategy const &);

private:
    static ACLSslErrorStrategy Instance_;
    ACLSslErrorStrategy() {}

    ACLSslErrorStrategy&operator=(ACLSslErrorStrategy const &);
};

class ACLSslError
{

private:
    static ACL::Prototype RegistryProtoype;
    static ACLStrategised<const Ssl::CertErrors *> RegistryEntry_;
};

#endif /* SQUID_ACLSSL_ERROR_H */