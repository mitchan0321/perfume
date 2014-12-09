/* $Id: t_gc.h,v 1.7 2011/09/04 13:00:54 mit-sato Exp $ */

#ifndef __T_GC__
#define __T_GC__

#ifdef PROF
#	define GC_INIT()		0
#	define GC_MALLOC(s)		malloc(s)
#	define GC_MALLOC_ATOMIC(s)	malloc(s)
#else
#	include <gc.h>
#endif /* PROF */

#endif /* __T_GC__ */
