#include "squid.h"

#define STUB_API "external_acl.cc"
#include "tests/STUB.h"

#include "ExternalACL.h"
#include "ExternalACLEntry.h"

void parse_externalAclHelper(external_acl ** ) STUB
void dump_externalAclHelper(StoreEntry *, const char *, const external_acl *) STUB
void free_externalAclHelper(external_acl **) STUB
void ACLExternal::parse() STUB
bool ACLExternal::valid () const STUB_RETVAL(false)
bool ACLExternal::empty () const STUB_RETVAL(false)
int ACLExternal::match(ACLChecklist *) STUB_RETVAL(0)
wordlist * ACLExternal::dump() const STUB_RETVAL(NULL)
void ACLExternal::ExternalAclLookup(ACLChecklist *, ACLExternal *) STUB
void ExternalACLLookup::Start(ACLChecklist *, external_acl_data *, bool) STUB
void externalAclInit(void) STUB_NOP
void externalAclShutdown(void) STUB_NOP
ExternalACLLookup * ExternalACLLookup::Instance() STUB_RETVAL(NULL)
void ExternalACLLookup::checkForAsync(ACLChecklist *) const STUB
void ExternalACLLookup::LookupDone(void *, void *) STUB
