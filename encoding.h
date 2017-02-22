#ifndef __ENCODING__
#define __ENCODING__

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "cell.h"

#define ENCODING_NAME_MAX		(1) 	// now ready encodings are RAW and UTF-8.
						// (index range is 0 to ENCODING_NAME_MAX)

/* encoding index */
#define NENCODE_RAW			(0)
#define NENCODE_UTF8			(1)
#define NENCODE_EUCJP			(2)	// not yet
#define NENCODE_SJIS			(3)	// not yet
#define NENCODE_ISO2022JP		(4)	// not yet

/* encoder/decoder error code */
#define EENCODE_BADENCODING		(1)
#define EENCODE_LESSLENGTH		(2)
#define EENCODE_BAD2NDBYTE		(3)
#define EENCODE_OUTOFRANGEUNICODE	(4)

typedef struct _encoder_error_info {
    int errorno;
    int pos;
    wchar_t *message;
} encoder_error_info;

wchar_t*	get_encoding_name(int enc_idx);
int		get_encoding_index(wchar_t *enc_name);
Cell*		decode_raw_to_unicode(Cell *raw, int enc, encoder_error_info *error_info);
Cell*		encode_unicode_to_raw(Cell *unicode, int enc, encoder_error_info *error_info);

#endif /* __ENCODING__ */
