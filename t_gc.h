/* $Id: t_gc.h,v 1.7 2011/09/04 13:00:54 mit-sato Exp $ */

#ifndef __T_GC__
#define __T_GC__

#ifdef PROF
#	define GC_INIT()					0
#	define GC_MALLOC(s)					malloc(s)
#	define GC_MALLOC_ATOMIC(s)				malloc(s)
#	define GC_REALLOC(d,s)					realloc(d,s)
#	define GC_register_finalizer_ignore_self(o,f,c,x,y)	0
#	define GC_add_roots(s,e)				0
#	define GC_remove_roots(d1,d2)				0
#else
#	include <gc.h>
#endif /* PROF */

#endif /* __T_GC__ */
