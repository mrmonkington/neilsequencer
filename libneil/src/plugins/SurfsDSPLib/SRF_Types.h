#ifndef	SRF_TYPES_H__
#define	SRF_TYPES_H__

#if defined(POSIX)
#include <sys/types.h>
#endif

namespace SurfDSPLib
{

#if !defined(POSIX)
typedef	unsigned char		u_char;
typedef	unsigned short		u_short;
typedef	unsigned long		u_long;
#endif

#if defined(__GNUC__)
typedef	unsigned long long	u_llong;
typedef	signed long long	llong;
#else
typedef	unsigned __int64	u_llong;
typedef	signed __int64		llong;
#endif

};

#endif
