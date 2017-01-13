#ifndef __UTIL__
#define __UTIL__

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <t_gc.h>

wchar_t	*to_wchar(const char *src);
char	*to_char(const wchar_t *src);


#endif /* __UTIL__ */
