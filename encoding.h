#ifndef __ENCODING__
#define __ENCODING__

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "cell.h"

/*
 * Indicate the maximum encoding name index; range is 0 to ENCODING_NAME_MAX.
 * Now ready encodings are RAW, UTF-8, UTF-8F, EUC-JP, Shift-JIS, UTF-16LE, UTF-16LEF,
 * UTF-16BE and UTF-16BE/F.
 */
#define ENCODING_NAME_MAX		(10)

/* encoding index */
#define NENCODE_RAW			(0)
#define NENCODE_UTF8			(1)
#define NENCODE_UTF8F			(2)
#define NENCODE_EUCJP			(3)
#define NENCODE_EUCJPF			(4)
#define NENCODE_SJIS			(5)
#define NENCODE_SJISF			(6)
#define NENCODE_UTF16LE			(7)
#define NENCODE_UTF16LEF		(8)
#define NENCODE_UTF16BE			(9)
#define NENCODE_UTF16BEF		(10)

/* encoding symbol */
#define SENCODE_RAW			(L"RAW")
#define SENCODE_UTF8			(L"UTF-8")
#define SENCODE_UTF8F			(L"UTF-8/F")
#define SENCODE_EUCJP			(L"EUC-JP")
#define SENCODE_EUCJPF			(L"EUC-JP/F")
#define SENCODE_SJIS			(L"Shift-JIS")
#define SENCODE_SJISF			(L"Shift-JIS/F")
#define SENCODE_UTF16LE			(L"UTF-16LE")
#define SENCODE_UTF16LEF		(L"UTF-16LE/F")
#define SENCODE_UTF16BE 		(L"UTF-16BE")
#define SENCODE_UTF16BEF 		(L"UTF-16BE/F")

/* encoder/decoder error code */
#define EENCODE_BADENCODING		(1)
#define EENCODE_LESSLENGTH		(2)
#define EENCODE_BAD2NDBYTE		(3)
#define EENCODE_OUTOFRANGEUNICODE	(4)
#define EENCODE_BADSEQUENCE	        (5)
#define EENCODE_BADSEQUENCEE	        (6)
#define EENCODE_BADSEQUENCES	        (7)
#define EENCODE_BADSEQUENCEU16	        (8)

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
int	 get_encoding_file_boundary(int enc);
int	 is_encoding_char_equal(int enc, int dest, int *src_array);
int	 is_encoding_char_eof(int enc, int *src_array);

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
Cell*utf16le_decoder(Cell *raw, encoder_error_info *error_info);
Cell*utf16lef_decoder(Cell *raw, encoder_error_info *error_info);
Cell*utf16le_encoder(Cell *unicode, encoder_error_info *error_info);
Cell*utf16be_decoder(Cell *raw, encoder_error_info *error_info);
Cell*utf16bef_decoder(Cell *raw, encoder_error_info *error_info);
Cell*utf16be_encoder(Cell *unicode, encoder_error_info *error_info);
int char_equal_single_byte(int dest, int *src_array);
int char_equal_u16le(int dest, int *src_array);
int char_equal_u16be(int dest, int *src_array);

#endif /* __ENCODING__ */
