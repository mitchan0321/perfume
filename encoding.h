#ifndef __ENCODING__
#define __ENCODING__

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

wchar_t*	get_encoding_name(int enc_idx);
int		get_encoding_index(wchar_t *enc_name);

#endif /* __ENCODING__ */
