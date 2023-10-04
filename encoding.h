#ifndef __ENCODING__
#define __ENCODING__

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "cell.h"

/*
 * Indicate the maximum encoding name index; range is 0 to ENCODING_NAME_MAX.
 * Now ready encodings are RAW, UTF-8, UTF-8F, EUC-JP and Shift-JIS.
 */
#define ENCODING_NAME_MAX		(6)

/* encoding index */
#define NENCODE_RAW			(0)
#define NENCODE_UTF8			(1)
#define NENCODE_UTF8F			(2)
#define NENCODE_EUCJP			(3)
#define NENCODE_EUCJPF			(4)
#define NENCODE_SJIS			(5)
#define NENCODE_SJISF			(6)

/* encoding symbol */
#define SENCODE_RAW			(L"RAW")
#define SENCODE_UTF8			(L"UTF-8")
#define SENCODE_UTF8F			(L"UTF-8F")
#define SENCODE_EUCJP			(L"EUC-JP")
#define SENCODE_EUCJPF			(L"EUC-JPF")
#define SENCODE_SJIS			(L"Shift-JIS")
#define SENCODE_SJISF			(L"Shift-JISF")

/* encoder/decoder error code */
#define EENCODE_BADENCODING		(1)
#define EENCODE_LESSLENGTH		(2)
#define EENCODE_BAD2NDBYTE		(3)
#define EENCODE_OUTOFRANGEUNICODE	(4)
#define EENCODE_BADSEQUENCE	        (5)
#define EENCODE_BADSEQUENCEE	        (6)
#define EENCODE_BADSEQUENCES	        (7)

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

/* encoder functions */
Cell*raw_decoder(Cell *raw, encoder_error_info *error_info);
Cell*raw_encoder(Cell *unicode, encoder_error_info *error_info);
Cell*utf8_decoder(Cell *raw, encoder_error_info *error_info);
Cell*utf8f_decoder(Cell *raw, encoder_error_info *error_info);
Cell*utf8_encoder(Cell *unicode, encoder_error_info *error_info);
Cell*eucjp_decoder(Cell *raw, encoder_error_info *error_info);
Cell*eucjpf_decoder(Cell *raw, encoder_error_info *error_info);
Cell*eucjp_encoder(Cell *unicode, encoder_error_info *error_info);
Cell*sjis_decoder(Cell *raw, encoder_error_info *error_info);
Cell*sjisf_decoder(Cell *raw, encoder_error_info *error_info);
Cell*sjis_encoder(Cell *unicode, encoder_error_info *error_info);

#endif /* __ENCODING__ */
