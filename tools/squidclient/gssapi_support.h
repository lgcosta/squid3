#ifndef _SQUID_TOOLS_SQUIDCLIENT_GSSAPI_H
#define _SQUID_TOOLS_SQUIDCLIENT_GSSAPI_H

#if HAVE_GSSAPI

#if HAVE_GSSAPI_GSSAPI_H
#include <gssapi/gssapi.h>
#elif HAVE_GSSAPI_H
#include <gssapi.h>
#endif /* HAVE_GSSAPI_GSSAPI_H/HAVE_GSSAPI_H */

#if !HAVE_HEIMDAL_KERBEROS
#if HAVE_GSSAPI_GSSAPI_KRB5_H
#include <gssapi/gssapi_krb5.h>
#endif
#if HAVE_GSSAPI_GSSAPI_GENERIC_H
#include <gssapi/gssapi_generic.h>
#endif
#if HAVE_GSSAPI_GSSAPI_EXT_H
#include <gssapi/gssapi_ext.h>
#endif
#endif

#ifndef gss_nt_service_name
#define gss_nt_service_name GSS_C_NT_HOSTBASED_SERVICE
#endif

bool check_gss_err(OM_uint32 major_status, OM_uint32 minor_status, const char *function);
char *GSSAPI_token(const char *server);

#endif /* HAVE_GSSAPI */
#endif /* _SQUID_TOOLS_SQUIDCLIENT_GSSAPI_H */
