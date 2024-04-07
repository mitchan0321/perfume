#include "toy.h"
#include "encoding.h"

static wchar_t *ENCODING_NAME_DEFS[] = {
    SENCODE_RAW,	// index: 0 ... RAW encoding (no encoding, byte data stream)
    SENCODE_UTF8,	// index: 1 ... UTF-8 encoding
    SENCODE_UTF8F,	// index: 2 ... UTF-8/F encoding
    SENCODE_EUCJP,	// index: 3 ... EUC-JP encoding
    SENCODE_EUCJPF,	// index: 4 ... EUC-JP/F encoding
    SENCODE_SJIS,	// index: 5 ... Shift-JIS encoding
    SENCODE_SJISF,	// index: 6 ... Shift-JIS/F encoding
    SENCODE_UTF16LE,	// index: 7 ... UTF-16LE encoding
    SENCODE_UTF16LEF,	// index: 8 ... UTF-16LE/F encoding
    SENCODE_UTF16BE,	// index: 9 ... UTF-16BE encoding
    SENCODE_UTF16BEF,	// index: 10 ... UTF-16BE/F encoding
    0
};

typedef struct _encoder_methods {
    Cell*(*raw_to_unicode)(Cell *raw, encoder_error_info *error_info);
    Cell*(*unicode_to_raw)(Cell *raw, encoder_error_info *error_info);
} encoder_methods;

static encoder_methods Encoder_methods[] = {
    {raw_decoder, raw_encoder},		// NENCODE_RAW
    {utf8_decoder, utf8_encoder},	// NENCODE_UTF8
    {utf8f_decoder, utf8_encoder},	// NENCODE_UTF8F
    {eucjp_decoder, eucjp_encoder},	// NENCODE_EUCJP
    {eucjpf_decoder, eucjp_encoder},	// NENCODE_EUCJPF
    {sjis_decoder, sjis_encoder},	// NENCODE_SJIS
    {sjisf_decoder, sjis_encoder},	// NENCODE_SJISF
    {utf16le_decoder, utf16le_encoder},	// NENCODE_UTF16LE
    {utf16lef_decoder, utf16le_encoder},// NENCODE_UTF16LEF
    {utf16be_decoder, utf16be_encoder},	// NENCODE_UTF16BE
    {utf16bef_decoder, utf16be_encoder},// NENCODE_UTF16BEF
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
#define HAS_ERROR(c, e, l) 					\
    if (-1 == c) {						\
	if (e) {						\
	    e->errorno = EENCODE_LESSLENGTH;			\
	    e->pos = i;						\
	    e->message = L"Less length UTF-8 sequence";		\
	}							\
	goto l;							\
    }								\
    if ((c & 0XC0) != 0X80) {					\
	if (e) {						\
	    e->errorno = EENCODE_BAD2NDBYTE;			\
	    e->pos = i;						\
	    e->message = L"Bad UTF-8 bit pattern 2nd byte";	\
	}							\
	goto l;							\
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
	    HAS_ERROR(c2, error_info, error);
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
	    HAS_ERROR(c2, error_info, error);
	    c2 = c2 & (~0xC0);
	    i++;

	    c3 = RETR_CHAR(p, i, len);
	    HAS_ERROR(c3, error_info, error);
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
	    HAS_ERROR(c2, error_info, error);
	    c2 = c2 & (~0xC0);
	    i++;

	    c3 = RETR_CHAR(p, i, len);
	    HAS_ERROR(c3, error_info, error);
	    c3 = c3 & (~0xC0);
	    i++;

	    c4 = RETR_CHAR(p, i, len);
	    HAS_ERROR(c4, error_info, error);
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

#define HAS_ERROR_FAKE(c, s) 					\
    s = 0;                                                      \
    if (-1 == c) {						\
        s = 1;                                                  \
    }								\
    if ((c & 0XC0) != 0X80) {					\
        s = 1;                                                  \
    }

#define HAS_ERROR_SET(e)     					\
    e->errorno = EENCODE_BADSEQUENCE;    			\
    e->pos = i;				        		\
    e->message = L"Bad UTF-8 sequence";

Cell*
utf8f_decoder(Cell *raw, encoder_error_info *error_info) {
    wchar_t *p;
    int i, len;
    Cell *result;
    wchar_t c1, c2, c3, c4;
    int errsts;
    
    error_info->errorno = 0;
    error_info->pos = 0;
    error_info->message = L"";

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
	    HAS_ERROR_FAKE(c2, errsts);
            if (errsts == 0) {
                c2 = c2 & (~0xC0);
                cell_add_char(result, ((c1 << 6) | c2));
            } else {
                cell_add_char(result, UTF8_FAKE_CHAR);
                HAS_ERROR_SET(error_info);
            }
            i++;
	    
	} else if ((p[i] & 0xF0) == 0xE0) {
	    /*
	     * 3 bytes UTF-8 sequence: 1110 xxxx, 10yy yyyy, 10yy yyyy
	     * 0x800 - 0xffff (16 bits)
	     */
	    c1 = (p[i] & (~0xF0));
	    i++;

	    c2 = RETR_CHAR(p, i , len);
	    HAS_ERROR_FAKE(c2, errsts);
            if (errsts == 0) {
                c2 = c2 & (~0xC0);
                i++;
                
                c3 = RETR_CHAR(p, i, len);
                HAS_ERROR_FAKE(c3, errsts);
                if (errsts == 0) {
                    c3 = c3 & (~0xC0);
                    cell_add_char(result, ((c1 << 12) | (c2 << 6) | c3));
                } else {
                    cell_add_char(result, UTF8_FAKE_CHAR);
                    HAS_ERROR_SET(error_info);
                }
                i++;
            } else {
                cell_add_char(result, UTF8_FAKE_CHAR);
                HAS_ERROR_SET(error_info);
                i++;
            }

	} else if ((p[i] & 0xF8) == 0xF0) {
	    /*
	     * 4 bytes UTF-8 sequence: 1111 0xxx, 10yy yyyy, 10yy yyyy, 10yy yyyy
	     * 0x10000 - 0x1fffff (21 bits)
	     */
	    c1 = (p[i] & (~0xF8));
	    i++;

	    c2 = RETR_CHAR(p, i, len);
	    HAS_ERROR_FAKE(c2, errsts);
            if (errsts == 0) {
                c2 = c2 & (~0xC0);
                i++;
                c3 = RETR_CHAR(p, i, len);
                HAS_ERROR_FAKE(c3, errsts);
                if (errsts == 0) {
                    c3 = c3 & (~0xC0);
                    i++;
                    c4 = RETR_CHAR(p, i, len);
                    HAS_ERROR_FAKE(c4, errsts);
                    if (errsts == 0) {
                        c4 = c4 & (~0xC0);
                        i++;
                        cell_add_char(result, ((c1 << 18) | (c2 << 12) | (c3 << 6) | c4));
                    } else {
                        cell_add_char(result, UTF8_FAKE_CHAR);
                        HAS_ERROR_SET(error_info);
                        i++;
                    }
                } else {
                    cell_add_char(result, UTF8_FAKE_CHAR);
                    HAS_ERROR_SET(error_info);
                    i++;
                }
            } else {
                cell_add_char(result, UTF8_FAKE_CHAR);
                HAS_ERROR_SET(error_info);
                i++;
            }
	    
	} else {
            cell_add_char(result, UTF8_FAKE_CHAR);
            HAS_ERROR_SET(error_info);
            i++;
	}
    }

    return result;
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
 * EUC-JP decoder/encoder.
 */

#define HAS_ERROR_SETE(e)     					\
    e->errorno = EENCODE_BADSEQUENCEE;    			\
    e->pos = i;				        		\
    e->message = L"Bad EUC-JP sequence";

#define HAS_ERROR_SETE_LESS(e) 					\
    e->errorno = EENCODE_LESSLENGTH;    			\
    e->pos = i;				        		\
    e->message = L"Less length EUC-JP sequence";

Cell*
eucjp_decoder(Cell *raw, encoder_error_info *error_info) {
    Cell *result;
    wchar_t *p, c, c2, cr;
    int len, i;
    wchar_t *buff;

    JISENCODER_INIT();

    error_info->errorno = 0;
    error_info->pos = 0;
    error_info->message = L"";

    p = cell_get_addr(raw);
    len = cell_get_length(raw);
    result = new_cell(L"");

    for (i=0; i<len; ) {
	c = p[i];
	if ((c >= 0) && (c <= 0x7f)) {
	    /* ascii */
	    cell_add_char(result, c);
	    i++;

	} else if ((c >= 0xa1) && (c <= 0xfe)) {
	    i++;
	    if (i >= len) {
		/* lost data euc_jp 2nd byte, broken file? */
		cell_add_char(result, c);
                HAS_ERROR_SETE_LESS(error_info);
		goto error;
	    }

	    c2 = p[i];
	    if ((c2 >= 0xa1) && (c2 <= 0xfe)) {
		cr = JISX0208_to_Unicode[(((c & 0x7f) << 8) | (c2 & 0x7f)) & 0xffff];
		if (cr == 0) {
		    /* not JISX0208 character */
		    cell_add_char(result, c);
                    cell_add_char(result, c2);
                    HAS_ERROR_SETE(error_info);
                    goto error;
		} else {
		    /* valid JISX0208 character */
		    cell_add_char(result, cr);
		}
	    } else {
		/* less data euc_jp 2nd byte */
		cell_add_char(result, c);
		cell_add_char(result, c2);
                HAS_ERROR_SETE(error_info);
                goto error;
	    }
	    i++;

	} else if (c == 0x8E) {
	    /* JISX0201 character prefix found */
	    i++;
	    if (i >= len) {
		/* lost data euc_jp JISX0201 2nd byte, broken file? */
		cell_add_char(result, c);
                HAS_ERROR_SETE(error_info);
                goto error;
	    }

	    c2 = p[i];
	    if ((c2 >= 0xA1) && (c2 <= 0xDF)) {
		/* JISX0201 character found */
		cell_add_char(result, 0xFF61 - 0xA1 + c2);
	    } else {
		/* not JISX0201 character */
		cell_add_char(result, c);
		cell_add_char(result, c2);
                HAS_ERROR_SETE(error_info);
                goto error;
	    }
	    i++;
	    
	} else {
	    /* not JISX0208 and not JISX0201 character */
	    cell_add_char(result, c);
	    i++;
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
eucjpf_decoder(Cell *raw, encoder_error_info *error_info) {
    Cell *result;
    wchar_t *p, c, c2, cr;
    int len, i;

    JISENCODER_INIT();

    error_info->errorno = 0;
    error_info->pos = 0;
    error_info->message = L"";

    p = cell_get_addr(raw);
    len = cell_get_length(raw);
    result = new_cell(L"");

    for (i=0; i<len; ) {
	c = p[i];
	if ((c >= 0) && (c <= 0x7f)) {
	    /* ascii */
	    cell_add_char(result, c);
	    i++;

	} else if ((c >= 0xa1) && (c <= 0xfe)) {
	    i++;
	    if (i >= len) {
		/* lost data euc_jp 2nd byte, broken file? */
		cell_add_char(result, c);
                HAS_ERROR_SETE_LESS(error_info);
                break;
	    }

	    c2 = p[i];
	    if ((c2 >= 0xa1) && (c2 <= 0xfe)) {
		cr = JISX0208_to_Unicode[(((c & 0x7f) << 8) | (c2 & 0x7f)) & 0xffff];
		if (cr == 0) {
		    /* not JISX0208 character */
		    cell_add_char(result, c);
                    cell_add_char(result, c2);
                    HAS_ERROR_SETE(error_info);
		} else {
		    /* valid JISX0208 character */
		    cell_add_char(result, cr);
		}
	    } else {
		/* less data euc_jp 2nd byte */
		cell_add_char(result, c);
		cell_add_char(result, c2);
                HAS_ERROR_SETE(error_info);
	    }
	    i++;

	} else if (c == 0x8E) {
	    /* JISX0201 character prefix found */
	    i++;
	    if (i >= len) {
		/* lost data euc_jp JISX0201 2nd byte, broken file? */
		cell_add_char(result, c);
                HAS_ERROR_SETE(error_info);
		break;
	    }

	    c2 = p[i];
	    if ((c2 >= 0xA1) && (c2 <= 0xDF)) {
		/* JISX0201 character found */
		cell_add_char(result, 0xFF61 - 0xA1 + c2);
	    } else {
		/* not JISX0201 character */
		cell_add_char(result, c);
		cell_add_char(result, c2);
                HAS_ERROR_SETE(error_info);
	    }
	    i++;
	    
	} else {
	    /* not JISX0208 and not JISX0201 character */
	    cell_add_char(result, c);
	    i++;
	}
    }
    
    return result;
}

Cell*
eucjp_encoder(Cell *unicode, encoder_error_info *error_info) {
    Cell *result;
    wchar_t *p, c, cr;
    int len, i;

    JISENCODER_INIT();

    p = cell_get_addr(unicode);
    len = cell_get_length(unicode);
    result = new_cell(L"");

    for (i=0; i<len; i++) {
	c = p[i];
	
	if ((c >= 0x00) && (c <= 0x7f)) {
	    /* ascii */
	    cell_add_char(result, c);

	} else if ((c > 0xffff) || (c < 0)) {
	    /* out range JISX0208 character, can't convert */
	    cell_add_char(result, L'?');

	} else if ((c >= 0xFF61) && (c <= 0xFF9F)) {
	    /* JISX0201 character */
	    cell_add_char(result, 0x8E);
	    cell_add_char(result, c - (0xFF61 - 0xA1));
	    
	} else {
	    cr = Unicode_to_JISX0208[c & 0xffff];
	    if (0 == cr) {
		/* not JISX0208 character, output throw */
		cell_add_char(result, (c >> 8) & 0xff);
		cell_add_char(result, (c     ) & 0xff);
		
	    } else {
		/* JISX0208 character, convert to euc_jp */
		cell_add_char(result, ((cr >> 8) & 0xff) | 0x80);
		cell_add_char(result, ((cr     ) & 0xff) | 0x80);
	    }
	}
    }
    
    return result;
}

/* 
 * Shift-JIS decoder/encoder.
 */
wchar_t
jis2sjis(wchar_t src) {
    wchar_t x, y, x1, x2, x3, Xres, y1, Yres;

    x = (src >> 8) & 0xff; // source hi-byte
    y = (src     ) & 0xff; // source low-byte

    x1 = x - 0x21;
    x2 = x1 >> 1;
    x3 = x1 & 0x01;
    if (x2 <= 0x1e) {
	Xres = x2 + 0x81;
    } else {
	Xres = x2 + 0xc1;
    }

    if (x3 == 1) {
	Yres = y + 0x7e;
    } else {
	y1 = y + 0x1f;
	if (y1 <= 0x7e) {
	    Yres = y1;
	} else {
	    Yres = y1 + 0x01;
	}
    }

    return (Xres << 8) | Yres;
}

wchar_t
sjis2jis(wchar_t src) {
    wchar_t x, y, x1, x2, x3, Xres, y1, Yres;

    x = (src >> 8) & 0xff; // source hi-byte
    y = (src     ) & 0xff; // source low-byte

    if (y >= 0x9f) {
	Yres = y - 0x7e;
	x3 = 1;
    } else {
	if (y >=0x80) {
	    y1 = y - 0x01;
	} else {
	    y1 = y;
	}
	Yres = y1 - 0x1f;
	x3 = 0;
    }

    if (x >= 0xe0) {
	x2 = x - 0xc1;
    } else {
	x2 = x - 0x81;
    }
    x1 = (x2 << 1) + x3;
    Xres = x1 + 0x21;

    return (Xres << 8) | Yres;    
}

#define HAS_ERROR_SETS(e)     					\
    e->errorno = EENCODE_BADSEQUENCES;    			\
    e->pos = i;				        		\
    e->message = L"Bad Shift-JIS sequence";

#define HAS_ERROR_SETS_LESS(e) 					\
    e->errorno = EENCODE_LESSLENGTH;    			\
    e->pos = i;				        		\
    e->message = L"Less length Shift-JIS sequence";

Cell*
sjisf_decoder(Cell *raw, encoder_error_info *error_info) {
    Cell *result;
    wchar_t *p, c, c2, cr;
    int len, i;

    JISENCODER_INIT();

    error_info->errorno = 0;
    error_info->pos = 0;
    error_info->message = L"";

    p = cell_get_addr(raw);
    len = cell_get_length(raw);
    result = new_cell(L"");

    for (i=0; i<len; ) {
	c = p[i];
	if ((c >= 0) && (c <= 0x7f)) {
	    /* ascii */
	    cell_add_char(result, c);
	    i++;

	} else if ((c >= 0xA1) && (c <= 0xDF)) {
	    /* JISX0201 character found */
	    cell_add_char(result, 0xFF61 - 0xA1 + c);
	    i++;
	    
	} else if (((c >= 0x81) && (c <= 0x9f)) || ((c >= 0xe0) && (c <= 0xff))) {
	    i++;
	    if (i >= len) {
		/* lost data Shift-JIS 2nd byte, broken file? */
		cell_add_char(result, c);
                HAS_ERROR_SETS_LESS(error_info);
		break;
	    }

	    c2 = p[i];
	    if (((c2 >= 0x40) && (c2 <= 0x7e)) || ((c2 >= 0x80) && (c2 <= 0xfc))) {
		cr = JISX0208_to_Unicode[sjis2jis(((c << 8) | c2) & 0xffff)];
		if (cr == 0) {
		    /* not JISX0208 character */
		    cell_add_char(result, c);
		    cell_add_char(result, c2);
                    HAS_ERROR_SETS(error_info);
		} else {
		    /* valid JISX0208 character */
		    cell_add_char(result, cr);
		}
	    } else {
		/* less data Shift-JIS 2nd byte */
		cell_add_char(result, c);
		cell_add_char(result, c2);
                HAS_ERROR_SETS(error_info);
	    }
	    i++;

	} else {
	    /* not JISX0208 and not JISX0201 character */
	    cell_add_char(result, c);
	    i++;
	}
    }
    
    return result;
}

Cell*
sjis_decoder(Cell *raw, encoder_error_info *error_info) {
    Cell *result;
    wchar_t *p, c, c2, cr;
    int len, i;
    wchar_t *buff;

    JISENCODER_INIT();

    error_info->errorno = 0;
    error_info->pos = 0;
    error_info->message = L"";

    p = cell_get_addr(raw);
    len = cell_get_length(raw);
    result = new_cell(L"");

    for (i=0; i<len; ) {
	c = p[i];
	if ((c >= 0) && (c <= 0x7f)) {
	    /* ascii */
	    cell_add_char(result, c);
	    i++;

	} else if ((c >= 0xA1) && (c <= 0xDF)) {
	    /* JISX0201 character found */
	    cell_add_char(result, 0xFF61 - 0xA1 + c);
	    i++;
	    
	} else if (((c >= 0x81) && (c <= 0x9f)) || ((c >= 0xe0) && (c <= 0xff))) {
	    i++;
	    if (i >= len) {
		/* lost data Shift-JIS 2nd byte, broken file? */
		cell_add_char(result, c);
                HAS_ERROR_SETS_LESS(error_info);
		goto error;
	    }

	    c2 = p[i];
	    if (((c2 >= 0x40) && (c2 <= 0x7e)) || ((c2 >= 0x80) && (c2 <= 0xfc))) {
		cr = JISX0208_to_Unicode[sjis2jis(((c << 8) | c2) & 0xffff)];
		if (cr == 0) {
		    /* not JISX0208 character */
		    cell_add_char(result, c);
		    cell_add_char(result, c2);
                    HAS_ERROR_SETS(error_info);
                    goto error;
		} else {
		    /* valid JISX0208 character */
		    cell_add_char(result, cr);
		}
	    } else {
		/* less data Shift-JIS 2nd byte */
		cell_add_char(result, c);
		cell_add_char(result, c2);
                HAS_ERROR_SETS(error_info);
		goto error;
	    }
	    i++;

	} else {
	    /* not JISX0208 and not JISX0201 character */
	    cell_add_char(result, c);
	    i++;
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
sjis_encoder(Cell *unicode, encoder_error_info *error_info) {
    Cell *result;
    wchar_t *p, c, cr;
    int len, i;

    JISENCODER_INIT();

    p = cell_get_addr(unicode);
    len = cell_get_length(unicode);
    result = new_cell(L"");

    for (i=0; i<len; i++) {
	c = p[i];
	
	if ((c >= 0x00) && (c <= 0x7f)) {
	    /* ascii */
	    cell_add_char(result, c);

	} else if ((c > 0xffff) || (c < 0)) {
	    /* out range JISX0208 character, can't convert */
	    cell_add_char(result, L'?');

	} else if ((c >= 0xFF61) && (c <= 0xFF9F)) {
	    /* JISX0201 character */
	    cell_add_char(result, c - (0xFF61 - 0xA1));
	    
	} else {
	    cr = Unicode_to_JISX0208[c & 0xffff];
	    if (0 == cr) {
		/* not JISX0208 character, output throw */
		cell_add_char(result, (c >> 8) & 0xff);
		cell_add_char(result, (c     ) & 0xff);
		
	    } else {
		/* JISX0208 character, convert to Shift-JIS */
		cr = jis2sjis(cr);
		cell_add_char(result, ((cr >> 8) & 0xff));
		cell_add_char(result, ((cr     ) & 0xff));
	    }
	}
    }
    
    return result;
}

/* 
 * UTF-16LE decoder/encoder.
 */
#define HAS_ERROR_SETU16LE(e)    				\
    e->errorno = EENCODE_BADSEQUENCEU16;    			\
    e->pos = i;				        		\
    e->message = L"Bad UTF-16LE sequence";

#define HAS_ERROR_SETU16LE_LESS(e) 				\
    e->errorno = EENCODE_LESSLENGTH;    			\
    e->pos = i;				        		\
    e->message = L"Less length UTF-16LE sequence";

Cell*
utf16lef_decoder(Cell *raw, encoder_error_info *error_info) {
    Cell *result;
    wchar_t *p, c, c2, cr, cr2;
    int len, i;

    error_info->errorno = 0;
    error_info->pos = 0;
    error_info->message = L"";

    p = cell_get_addr(raw);
    len = cell_get_length(raw);
    result = new_cell(L"");

    for (i=0; i<len; i++) {
	c = p[i];
        i++;
        if (i >= len) {
            /* lost data UTF16LE 2nd byte, broken file? */
            cell_add_char(result, c);
            HAS_ERROR_SETU16LE_LESS(error_info);
            break;
        }
        c2 = p[i];
        cr = (c2 << 8) | (c & 0xff);
        
        if ((cr >= 0xdc00) && (cr <= 0xdfff)) {
            /* bad UTF16LE sequence */
            cell_add_char(result, c);
            cell_add_char(result, c2);
            HAS_ERROR_SETU16LE(error_info);
            
        } else if ((cr >= 0xd800) && (cr <= 0xdbff)) {
            /* UTF16LE sequence 1st surrogte found */
            i++;
            if (i >= len) {
                /* lost data UTF16LE 2nd suroogate 1st byte, broken file? */
                cell_add_char(result, c);
                cell_add_char(result, c2);
                HAS_ERROR_SETU16LE_LESS(error_info);
                break;
            }
            c = p[i];

            i++;
            if (i >= len) {
                /* lost data UTF16LE 2nd surrogate 2nd byte, broken file? */
                cell_add_char(result, cr);
                cell_add_char(result, c);
                HAS_ERROR_SETU16LE_LESS(error_info);
                break;
            }
            c2 = p[i];
            cr2 = (c2 << 8) | (c & 0xff);

            if (! ((cr2 >= 0xdc00) && (cr2 <= 0xdfff))) {
                /* bad UTF16LE sequence */
                cell_add_char(result, cr);
                cell_add_char(result, cr2);
                HAS_ERROR_SETU16LE(error_info);

            } else {
                /* success retrebe 2nd surrogate pair */
                cell_add_char(result, (((cr - 0xd800) << 10) + 0x10000 + (cr2 - 0xdc00)));
            }

        } else {
            /* valid, non surrogate pair character */
            cell_add_char(result, cr);
        }
    }
    
    return result;
}

Cell*
utf16le_decoder(Cell *raw, encoder_error_info *error_info) {
    Cell *result;
    wchar_t *p, c, c2, cr, cr2;
    int len, i;
    wchar_t *buff;

    error_info->errorno = 0;
    error_info->pos = 0;
    error_info->message = L"";

    p = cell_get_addr(raw);
    len = cell_get_length(raw);
    result = new_cell(L"");

    for (i=0; i<len; i++) {
	c = p[i];
        i++;
        if (i >= len) {
            /* lost data UTF16LE 2nd byte, broken file? */
            cell_add_char(result, c);
            HAS_ERROR_SETU16LE_LESS(error_info);
            goto error;
        }
        c2 = p[i];
        cr = (c2 << 8) | (c & 0xff);
        
        if ((cr >= 0xdc00) && (cr <= 0xdfff)) {
            /* bad UTF16LE sequence */
            cell_add_char(result, c);
            cell_add_char(result, c2);
            HAS_ERROR_SETU16LE(error_info);
            goto error;
            
        } else if ((cr >= 0xd800) && (cr <= 0xdbff)) {
            /* UTF16LE sequence 1st surrogte found */
            i++;
            if (i >= len) {
                /* lost data UTF16LE 2nd suroogate 1st byte, broken file? */
                cell_add_char(result, c);
                cell_add_char(result, c2);
                HAS_ERROR_SETU16LE_LESS(error_info);
                goto error;
            }
            c = p[i];

            i++;
            if (i >= len) {
                /* lost data UTF16LE 2nd surrogate 2nd byte, broken file? */
                cell_add_char(result, cr);
                cell_add_char(result, c);
                HAS_ERROR_SETU16LE_LESS(error_info);
                goto error;
            }
            c2 = p[i];
            cr2 = (c2 << 8) | (c & 0xff);

            if (! ((cr2 >= 0xdc00) && (cr2 <= 0xdfff))) {
                /* bad UTF16LE sequence */
                cell_add_char(result, cr);
                cell_add_char(result, cr2);
                HAS_ERROR_SETU16LE(error_info);
                goto error;

            } else {
                /* success retrebe 2nd surrogate pair */
                cell_add_char(result, (((cr - 0xd800) << 10) + 0x10000 + (cr2 - 0xdc00)));
            }

        } else {
            /* valid, non surrogate pair character */
            cell_add_char(result, cr);
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
utf16le_encoder(Cell *unicode, encoder_error_info *error_info) {
    Cell *result;
    wchar_t *p, c;
    int len, i;

    p = cell_get_addr(unicode);
    len = cell_get_length(unicode);
    result = new_cell(L"");

    for (i=0; i<len; i++) {
        c = p[i];
        if (p[i] < 0x10000) {
            /* output BMP code area */
            cell_add_char(result, (c  & 0xff));
            cell_add_char(result, ((c >> 8)  & 0xff));

        } else {
            /* output 1st surrogate word */
            cell_add_char(result, (((c - 0x10000) >> 10) + 0xd800) & 0xff);
            cell_add_char(result, ((((c - 0x10000) >> 10) + 0xd800) >> 8) & 0xff);
            cell_add_char(result, (((c - 0x10000) & 0x3ff) + 0xdc00) & 0xff);
            cell_add_char(result, (((((c - 0x10000) & 0x3ff) + 0xdc00) >> 8)) & 0xff);
        }
    }
    
    return result;
}

/* 
 * UTF-16BE decoder/encoder.
 */
#define HAS_ERROR_SETU16BE(e)    				\
    e->errorno = EENCODE_BADSEQUENCEU16;    			\
    e->pos = i;				        		\
    e->message = L"Bad UTF-16BE sequence";

#define HAS_ERROR_SETU16BE_LESS(e) 				\
    e->errorno = EENCODE_LESSLENGTH;    			\
    e->pos = i;				        		\
    e->message = L"Less length UTF-16BE sequence";

Cell*
utf16bef_decoder(Cell *raw, encoder_error_info *error_info) {
    Cell *result;
    wchar_t *p, c, c2, cr, cr2;
    int len, i;

    error_info->errorno = 0;
    error_info->pos = 0;
    error_info->message = L"";

    p = cell_get_addr(raw);
    len = cell_get_length(raw);
    result = new_cell(L"");

    for (i=0; i<len; i++) {
	c = p[i];
        i++;
        if (i >= len) {
            /* lost data UTF16BE 2nd byte, broken file? */
            cell_add_char(result, c);
            HAS_ERROR_SETU16BE_LESS(error_info);
            break;
        }
        c2 = p[i];
        cr = (c << 8) | (c2 & 0xff);
        
        if ((cr >= 0xdc00) && (cr <= 0xdfff)) {
            /* bad UTF16BE sequence */
            cell_add_char(result, c);
            cell_add_char(result, c2);
            HAS_ERROR_SETU16BE(error_info);
            
        } else if ((cr >= 0xd800) && (cr <= 0xdbff)) {
            /* UTF16BE sequence 1st surrogte found */
            i++;
            if (i >= len) {
                /* lost data UTF16BE 2nd suroogate 1st byte, broken file? */
                cell_add_char(result, c);
                cell_add_char(result, c2);
                HAS_ERROR_SETU16BE_LESS(error_info);
                break;
            }
            c = p[i];

            i++;
            if (i >= len) {
                /* lost data UTF16BE 2nd surrogate 2nd byte, broken file? */
                cell_add_char(result, cr);
                cell_add_char(result, c);
                HAS_ERROR_SETU16BE_LESS(error_info);
                break;
            }
            c2 = p[i];
            cr2 = (c << 8) | (c2 & 0xff);

            if (! ((cr2 >= 0xdc00) && (cr2 <= 0xdfff))) {
                /* bad UTF16BE sequence */
                cell_add_char(result, cr);
                cell_add_char(result, cr2);
                HAS_ERROR_SETU16BE(error_info);

            } else {
                /* success retrebe 2nd surrogate pair */
                cell_add_char(result, (((cr - 0xd800) << 10) + 0x10000 + (cr2 - 0xdc00)));
            }

        } else {
            /* valid, non surrogate pair character */
            cell_add_char(result, cr);
        }
    }
    
    return result;
}

Cell*
utf16be_decoder(Cell *raw, encoder_error_info *error_info) {
    Cell *result;
    wchar_t *p, c, c2, cr, cr2;
    int len, i;
    wchar_t *buff;

    error_info->errorno = 0;
    error_info->pos = 0;
    error_info->message = L"";

    p = cell_get_addr(raw);
    len = cell_get_length(raw);
    result = new_cell(L"");

    for (i=0; i<len; i++) {
	c = p[i];
        i++;
        if (i >= len) {
            /* lost data UTF16BE 2nd byte, broken file? */
            cell_add_char(result, c);
            HAS_ERROR_SETU16BE_LESS(error_info);
            goto error;
        }
        c2 = p[i];
        cr = (c << 8) | (c2 & 0xff);
        
        if ((cr >= 0xdc00) && (cr <= 0xdfff)) {
            /* bad UTF16BE sequence */
            cell_add_char(result, c);
            cell_add_char(result, c2);
            HAS_ERROR_SETU16BE(error_info);
            goto error;
            
        } else if ((cr >= 0xd800) && (cr <= 0xdbff)) {
            /* UTF16BE sequence 1st surrogte found */
            i++;
            if (i >= len) {
                /* lost data UTF16BE 2nd suroogate 1st byte, broken file? */
                cell_add_char(result, c);
                cell_add_char(result, c2);
                HAS_ERROR_SETU16BE_LESS(error_info);
                goto error;
            }
            c = p[i];

            i++;
            if (i >= len) {
                /* lost data UTF16BE 2nd surrogate 2nd byte, broken file? */
                cell_add_char(result, cr);
                cell_add_char(result, c);
                HAS_ERROR_SETU16BE_LESS(error_info);
                goto error;
            }
            c2 = p[i];
            cr2 = (c << 8) | (c2 & 0xff);

            if (! ((cr2 >= 0xdc00) && (cr2 <= 0xdfff))) {
                /* bad UTF16BE sequence */
                cell_add_char(result, cr);
                cell_add_char(result, cr2);
                HAS_ERROR_SETU16BE(error_info);
                goto error;

            } else {
                /* success retrebe 2nd surrogate pair */
                cell_add_char(result, (((cr - 0xd800) << 10) + 0x10000 + (cr2 - 0xdc00)));
            }

        } else {
            /* valid, non surrogate pair character */
            cell_add_char(result, cr);
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
utf16be_encoder(Cell *unicode, encoder_error_info *error_info) {
    Cell *result;
    wchar_t *p, c;
    int len, i;

    p = cell_get_addr(unicode);
    len = cell_get_length(unicode);
    result = new_cell(L"");

    for (i=0; i<len; i++) {
        c = p[i];
        if (p[i] < 0x10000) {
            /* output BMP code area */
            cell_add_char(result, ((c >> 8)  & 0xff));
            cell_add_char(result, (c  & 0xff));

        } else {
            /* output 1st surrogate word */
            cell_add_char(result, ((((c - 0x10000) >> 10) + 0xd800) >> 8) & 0xff);
            cell_add_char(result, (((c - 0x10000) >> 10) + 0xd800) & 0xff);
            cell_add_char(result, (((((c - 0x10000) & 0x3ff) + 0xdc00) >> 8)) & 0xff);
            cell_add_char(result, ((((c - 0x10000) & 0x3ff) + 0xdc00) & 0xff));
        }
    }
    
    return result;
}
