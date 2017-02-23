#include "toy.h"
#include "encoding.h"

static wchar_t *ENCODING_NAME_DEFS[] = {
    L"RAW",		// index: 0 ... RAW encoding (no encoding, byte data stream)
    L"UTF-8",		// index: 1 ... UTF-8 encoding
    L"EUC-JP",		// index: 2 ... EUC-JP encoding
    L"Shift-JIS",	// index: 3 ... Shift-JIS encoding (not yet)
};

Cell*raw_decoder(Cell *raw, encoder_error_info *error_info);
Cell*raw_encoder(Cell *unicode, encoder_error_info *error_info);
Cell*utf8_decoder(Cell *raw, encoder_error_info *error_info);
Cell*utf8_encoder(Cell *unicode, encoder_error_info *error_info);
Cell*eucjp_decoder(Cell *raw, encoder_error_info *error_info);
Cell*eucjp_encoder(Cell *unicode, encoder_error_info *error_info);

typedef struct _encoder_methods {
    Cell*(*raw_to_unicode)(Cell *raw, encoder_error_info *error_info);
    Cell*(*unicode_to_raw)(Cell *raw, encoder_error_info *error_info);
} encoder_methods;

static encoder_methods Encoder_methods[] = {
    {raw_decoder, raw_encoder},		// NENCODE_RAW
    {utf8_decoder, utf8_encoder},	// NENCODE_UTF8
    {eucjp_decoder, eucjp_encoder},	// NENCODE_EUCJP
    {0, 0}
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
	    error_info->message = L"Bad encoder index.";
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
	    error_info->message = L"Bad encoder index.";
	}
	return NULL;
    }

    return Encoder_methods[enc].unicode_to_raw(unicode, error_info);
}

/* 
 * RAW decoder/encoder, do nathing.
 */
Cell*
raw_decoder(Cell *raw, encoder_error_info *error_info) {
    return raw;
}

Cell*
raw_encoder(Cell *unicode, encoder_error_info *error_info) {
    return unicode;
}

/* 
 * UTF-8 decoder/encoder.
 */
#define RETR_CHAR(p, i, len)	((i<len)?p[i]:-1)
#define HAS_ERROR(c, e) 					\
    if (-1 == c) {						\
	if (e) {						\
	    e->errorno = EENCODE_LESSLENGTH;			\
	    e->pos = i;						\
	    e->message = L"Less length UTF-8 sequence";		\
	}							\
	goto error;						\
    }								\
    if ((c & 0XC0) != 0X80) {					\
	if (e) {						\
	    e->errorno = EENCODE_BAD2NDBYTE;			\
	    e->pos = i;						\
	    e->message = L"Bad UTF-8 bit pattern 2nd byte";	\
	}							\
	goto error;						\
    }

Cell*
utf8_decoder(Cell *raw, encoder_error_info *error_info) {
    wchar_t *p;
    int i, len;
    Cell *result;
    wchar_t c1, c2, c3, c4;
    wchar_t *buff;
    
    p = cell_get_addr(raw);
    len = cell_get_length(raw);
    result = new_cell(L"");

    for (i=0; i<len; ) {
	if ((p[i] & 0x80) == 0) {
	    /*
	     * 1 byte ASCII
	     * 0x00 - 0x7f (7 bits)
	     */
	    cell_add_char(result, p[i]);
	    i++;
	    
	} else if ((p[i] & 0xE0) == 0xC0) {
	    /*
	     * 2 bytes UTF-8 sequence: 110x xxxx, 10yy yyyy
	     * 0x80 - 0x7ff (11 bits)
	     */
	    c1 = (p[i] & (~0xE0));
	    i++;
	    
	    c2 = RETR_CHAR(p, i, len);
	    HAS_ERROR(c2, error_info);
	    c2 = c2 & (~0xC0);
	    i++;

	    cell_add_char(result, ((c1 << 6) | c2));
	    
	} else if ((p[i] & 0xF0) == 0xE0) {
	    /*
	     * 3 bytes UTF-8 sequence: 1110 xxxx, 10yy yyyy, 10yy yyyy
	     * 0x800 - 0xffff (16 bits)
	     */
	    c1 = (p[i] & (~0xF0));
	    i++;

	    c2 = RETR_CHAR(p, i , len);
	    HAS_ERROR(c2, error_info);
	    c2 = c2 & (~0xC0);
	    i++;

	    c3 = RETR_CHAR(p, i, len);
	    HAS_ERROR(c3, error_info);
	    c3 = c3 & (~0xC0);
	    i++;

	    cell_add_char(result, ((c1 << 12) | (c2 << 6) | c3));

	} else if ((p[i] & 0xF8) == 0xF0) {
	    /*
	     * 4 bytes UTF-8 sequence: 1111 0xxx, 10yy yyyy, 10yy yyyy, 10yy yyyy
	     * 0x10000 - 0x1fffff (21 bits)
	     */
	    c1 = (p[i] & (~0xF8));
	    i++;

	    c2 = RETR_CHAR(p, i, len);
	    HAS_ERROR(c2, error_info);
	    c2 = c2 & (~0xC0);
	    i++;

	    c3 = RETR_CHAR(p, i, len);
	    HAS_ERROR(c3, error_info);
	    c3 = c3 & (~0xC0);
	    i++;

	    c4 = RETR_CHAR(p, i, len);
	    HAS_ERROR(c4, error_info);
	    c4 = c4 & (~0xC0);
	    i++;
	    
	    cell_add_char(result, ((c1 << 18) | (c2 << 12) | (c3 << 6) | c4));
	    
	} else {
	    
	    if (error_info) {
		buff = GC_MALLOC(256*sizeof(wchar_t));
		ALLOC_SAFE(buff);
		error_info->errorno = EENCODE_OUTOFRANGEUNICODE;
		error_info->pos = i;
		swprintf(buff, 256, L"Out of range UNICODE codepoint, at %d bytes.", i);
		error_info->message = buff;
	    }

	    return NULL;
	}
    }

    return result;

error:
    if (error_info) {
	buff = GC_MALLOC(256*sizeof(wchar_t));
	ALLOC_SAFE(buff);
	swprintf(buff, 256, L"%ls, at %d bytes.", error_info->message, error_info->pos);
	error_info->message = buff;
    }
	    
    return NULL;
}

Cell*
utf8_encoder(Cell *unicode, encoder_error_info *error_info) {
    Cell *result;
    wchar_t *p, c;
    int len, i;
    wchar_t *buff;
    
    p = cell_get_addr(unicode);
    len = cell_get_length(unicode);
    result = new_cell(L"");

    for (i=0; i<len; i++) {
	c = p[i];
	if ((c >= 0) &&  (c <= 0x7f)) {
	    /*
	     * 1 byte ASCII
	     * 0x00 - 0x7f (7 bits)
	     */
	    cell_add_char(result, c);

	} else if ((c >= 0x80) && (c <= 0x7ff)) {
	    /*
	     * 2 bytes UTF-8 sequence: 110x xxxx, 10yy yyyy
	     * 0x80 - 0x7ff (11 bits)
	     */
	    cell_add_char(result, 0xC0 | ((c >> 6) & 0x1F));
	    cell_add_char(result, 0x80 | (c & 0x3F));
	    
	} else if ((c >= 0x800) && (c <= 0xffff)) {
	    /*
	     * 3 bytes UTF-8 sequence: 1110 xxxx, 10yy yyyy, 10yy yyyy
	     * 0x800 - 0xffff (16 bits)
	     */
	    cell_add_char(result, 0xE0 | ((c >> 12) & 0x0F));
	    cell_add_char(result, 0x80 | ((c >> 6) & 0x3F));
	    cell_add_char(result, 0x80 | (c & 0x3F));
	    
	} else if ((c >= 0x10000) && (c <= 0x1fffff)) {
	    /*
	     * 4 bytes UTF-8 sequence: 1111 0xxx, 10yy yyyy, 10yy yyyy, 10yy yyyy
	     * 0x10000 - 0x1fffff (21 bits)
	     */
	    cell_add_char(result, 0xF0 | ((c >> 18) & 0x07));
	    cell_add_char(result, 0x80 | ((c >> 12) & 0x3F));
	    cell_add_char(result, 0x80 | ((c >> 6) & 0x3F));
	    cell_add_char(result, 0x80 | (c & 0x3F));
	    
	} else {
	    if (error_info) {
		buff = GC_MALLOC(256*sizeof(wchar_t));
		ALLOC_SAFE(buff);
		error_info->errorno = EENCODE_OUTOFRANGEUNICODE;
		error_info->pos = i;
		swprintf(buff, 256, L"Out of range UNICODE codepoint, at %d characters.", i);
		error_info->message = buff;
	    }
	    
	    return NULL;
	}
    }

    return result;
}

/* 
 * Shift-JIS decoder/encoder.
 */
Cell*
eucjp_decoder(Cell *raw, encoder_error_info *error_info) {
    Cell *result;
    wchar_t *p, c, c2, cr;
    int len, i;

    JISENCODER_INIT();

    p = cell_get_addr(raw);
    len = cell_get_length(raw);
    result = new_cell(L"");

    for (i=0; i<len; ) {
	c = p[i];
	if (c <= 0x7f) {
	    cell_add_char(result, c);
	    i++;
	} else if ((c >= 0xa1) && (c <= 0xfe)) {
	    i++;
	    if (i >= len) {
		/* lost data euc_jp 2nd byte, broken file? */
		cell_add_char(result, c);
		break;
	    }

	    c2 = p[i];
	    if ((c2 >= 0xa1) && (c2 <= 0xfe)) {
		cr = JIS0208_to_Unicode[(((c & 0x7f) << 8) | (c2 & 0x7f)) & 0xffff];
		if (cr == 0) {
		    /* not JISX0208 character */
		    cell_add_char(result, c);
		    cell_add_char(result, c2);
		} else {
		    /* valid JISX0208 character */
		    cell_add_char(result, cr);
		}
	    } else {
		/* less data euc_jp 2nd byte */
		cell_add_char(result, c2);
	    }
	    i++;

	} else {
	    /* not JISX0208 character */
	    cell_add_char(result, c);
	    i++;
	}
    }
    
    return result;
}

Cell*
eucjp_encoder(Cell *unicode, encoder_error_info *error_info) {
    JISENCODER_INIT();
    return unicode;
}
