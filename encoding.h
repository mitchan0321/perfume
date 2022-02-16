#ifndef __ENCODING__
#define __ENCODING__

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "cell.h"

/*
 * Indicate the maximum encoding name index; range is 0 to ENCODING_NAME_MAX.
 * Now ready encodings are RAW, UTF-8, EUC-JP and Shift-JIS.
 */
#define ENCODING_NAME_MAX		(4)

/* encoding index */
#define NENCODE_RAW			(0)
#define NENCODE_UTF8			(1)
#define NENCODE_UTF8F			(2)
#define NENCODE_EUCJP			(3)
#define NENCODE_SJIS			(4)

/* encoder/decoder error code */
#define EENCODE_BADENCODING		(1)
#define EENCODE_LESSLENGTH		(2)
#define EENCODE_BAD2NDBYTE		(3)
#define EENCODE_OUTOFRANGEUNICODE	(4)
#define EENCODE_BADSEQUENCE	        (5)

typedef struct _encoder_error_info {
    int errorno;
    int pos;
    wchar_t *message;
} encoder_error_info;

#define UTF8_FAKE_CHAR                  (0x2592)

wchar_t* get_encoding_name(int enc_idx);
int	 get_encoding_index(wchar_t *enc_name);
Cell*	 decode_raw_to_unicode(Cell *raw, int enc, encoder_error_info *error_info);
Cell*	 encode_unicode_to_raw(Cell *unicode, int enc, encoder_error_info *error_info);

/* for JIS converter */
extern wchar_t Unicode_to_JISX0208[65536];
extern wchar_t JISX0208_to_Unicode[65536];
extern int jisencoder_setup_done;
void JisEncoder_Setup();
#define JISENCODER_INIT()	if (jisencoder_setup_done == 0) {JisEncoder_Setup();}

#endif /* __ENCODING__ */
