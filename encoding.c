#include "encoding.h"

static wchar_t *ENCODING_NAME_DEFS[] = {
    L"RAW",		// index: 0 ... RAW encoding (no encoding data stream)
    L"UTF-8",		// index: 1 ... UTF-8 encoding
    L"EUC-JP",		// index: 2 ... EUC-JP encoding (not yet)
    L"SJIS",		// index: 3 ... Shift-JIS encoding (not yet)
    L"ISO-2022-JP",	// index: 4 ... ISO-2022-JP(JIS) encoding (not yet)
};

Cell*raw_decoder(Cell *raw, encoder_error_info *error_info);
Cell*raw_encoder(Cell *unicode, encoder_error_info *error_info);
Cell*utf8_decoder(Cell *raw, encoder_error_info *error_info);
Cell*utf8_encoder(Cell *unicode, encoder_error_info *error_info);

typedef struct _encoder_methods {
    Cell*(*raw_to_unicode)(Cell *raw, encoder_error_info *error_info);
    Cell*(*unicode_to_raw)(Cell *raw, encoder_error_info *error_info);
} encoder_methods;

static encoder_methods Encoder_methods[] = {
    {raw_decoder, raw_encoder},		// NENCODE_RAW
    {utf8_decoder, utf8_encoder},	// NENCODE_UTF8
};

wchar_t*
get_encoding_name(int enc_idx) {
    /* if return 0, enc_idex range over */
    if ((enc_idx < 0) ||
	(enc_idx > ENCODING_NAME_MAX)) return 0;

    return ENCODING_NAME_DEFS[enc_idx];
}

int
get_encoding_index(wchar_t *enc_name) {
    int i;
    for (i=0; i<= ENCODING_NAME_MAX; i++) {
	if (wcscmp(ENCODING_NAME_DEFS[i], enc_name) == 0) return i;
    }

    /* if return -1, invalid string enc_name */
    return -1;
}

Cell*
decode_raw_to_unicode(Cell *raw, int enc, encoder_error_info *error_info) {
    if ((enc < 0) ||
	(enc > ENCODING_NAME_MAX)) {
	if (error_info) {
	    error_info->errorno = EENCODE_BADENCODING;
	    error_info->pos = 0;
	    error_info->message = L"Bad encoder index";
	}
	return NULL;
    }

    return Encoder_methods[enc].raw_to_unicode(raw, error_info);
}

Cell*
encode_unicode_to_raw(Cell *unicode, int enc, encoder_error_info *error_info) {
    if ((enc < 0) ||
	(enc > ENCODING_NAME_MAX)) {
	if (error_info) {
	    error_info->errorno = EENCODE_BADENCODING;
	    error_info->pos = 0;
	    error_info->message = L"Bad encoder index";
	}
	return NULL;
    }

    return Encoder_methods[enc].unicode_to_raw(unicode, error_info);
}

Cell*
raw_decoder(Cell *raw, encoder_error_info *error_info) {
    return raw;
}

Cell*
raw_encoder(Cell *unicode, encoder_error_info *error_info) {
    return unicode;
}

Cell*
utf8_decoder(Cell *raw, encoder_error_info *error_info) {
    return raw;
}

Cell*
utf8_encoder(Cell *unicode, encoder_error_info *error_info) {
    return unicode;
}
