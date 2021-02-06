#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include "util.h"
#include "global.h"

int
read_size(int fd, char* buff, int size) {
    int sts;
    int pos = 0;
    int remain = size;

    do {
	sts = read(fd, &buff[pos], remain);
	if (-1 == sts) return -1;
	if ( 0 == sts) return -1;

	pos += sts;
	remain -= sts;

    } while (remain > 0);

    return 1;
}

int
write_size(int fd, char* buff, int size) {
    int sts;
    int pos = 0;
    int remain = size;

    do {
	sts = write(fd, &buff[pos], remain);
	if (-1 == sts) return -1;
	if ( 0 == sts) return -1;

	pos += sts;
	remain -= sts;

    } while (remain > 0);

    return 1;
}

wchar_t*
to_wchar(const char *src) {
    wchar_t *dest;
    int len, i;
    
    len = strlen(src);
    dest = GC_MALLOC((len+1)*sizeof(wchar_t));
    ALLOC_SAFE(dest);
    for (i=0; i<=len; i++) {
	dest[i] = ((wchar_t)src[i]) & 0xff;
    }

    return dest;
}

char*
to_char(const wchar_t *src) {
    char *dest;
    int len, i;
    
    len = wcslen(src);
    dest = GC_MALLOC(len+1);
    ALLOC_SAFE(dest);
    for (i=0; i<=len; i++) {
	dest[i] = (char)src[i];
    }

    return dest;
}

void
println(Toy_Interp *interp, wchar_t *msg) {
    Toy_Type *l;

    l = new_list(new_symbol(L"println"));
    list_append(l, new_string_str(msg));

    toy_call(interp, l);

    return;
}

char*
encode_dirent(Toy_Interp *interp, wchar_t *name, encoder_error_info **info) {
    Toy_Type *enc;
    int iencoder;
    encoder_error_info *error_info;
    Cell *raw;

    error_info = GC_MALLOC(sizeof(encoder_error_info));
    ALLOC_SAFE(error_info);
    *info = error_info;

    iencoder = NENCODE_RAW;
    enc = hash_get_t(interp->globals, const_DEFAULT_DIRENT_ENCODING);
    if (enc) {
	if (GET_TAG(enc) == SYMBOL) {
	    iencoder = get_encoding_index(cell_get_addr(enc->u.symbol.cell));
	    if (-1 == iencoder) {
		error_info->errorno = EENCODE_BADENCODING;
		error_info->pos = -1;
		error_info->message = L"Bad encoder specified.";
		return 0;
	    }
	} else {
	    error_info->errorno = EENCODE_BADENCODING;
	    error_info->pos = -1;
	    error_info->message = L"Bad encoder specified, need symbol.";
	    return 0;
	}
    }

    raw = encode_unicode_to_raw(new_cell(name), iencoder, error_info);
    if (NULL == raw) return 0;

    return to_char(cell_get_addr(raw));
}

Cell*
decode_dirent(Toy_Interp *interp, char *name, encoder_error_info **info) {
    Toy_Type *enc;
    int iencoder;
    encoder_error_info *error_info;
    Cell *unicode;

    error_info = GC_MALLOC(sizeof(encoder_error_info));
    ALLOC_SAFE(error_info);
    *info = error_info;

    iencoder = NENCODE_RAW;
    enc = hash_get_t(interp->globals, const_DEFAULT_DIRENT_ENCODING);
    if (enc) {
	if (GET_TAG(enc) == SYMBOL) {
	    iencoder = get_encoding_index(cell_get_addr(enc->u.symbol.cell));
	    if (-1 == iencoder) {
		error_info->errorno = EENCODE_BADENCODING;
		error_info->pos = -1;
		error_info->message = L"Bad encoder specified.";
		return 0;
	    }
	} else {
	    error_info->errorno = EENCODE_BADENCODING;
	    error_info->pos = -1;
	    error_info->message = L"Bad encoder specified, need symbol.";
	    return 0;
	}
    }

    unicode = decode_raw_to_unicode(new_cell(to_wchar(name)), iencoder, error_info);
    if (NULL == unicode) return 0;

    return unicode;
}

wchar_t*
decode_error(Toy_Interp *interp, char *str) {
    int iencoder;
    Cell *c;
    encoder_error_info *enc_error_info;
    Toy_Type *enc;
    
    enc = hash_get_t(interp->globals, const_DEFAULT_DIRENT_ENCODING);
    if (NULL == enc) {
        iencoder = get_encoding_index(DEFAULT_ERRSTR_ENCODING);
    } else {
        if (GET_TAG(enc) == SYMBOL) {
            iencoder = get_encoding_index(cell_get_addr(enc->u.symbol.cell));
        } else {
            iencoder = get_encoding_index(DEFAULT_ERRSTR_ENCODING);
        }
    }
    if (-1 == iencoder) {
        return to_wchar(str);
    }
    
    enc_error_info = GC_MALLOC(sizeof(encoder_error_info));
    ALLOC_SAFE(enc_error_info);
    c = decode_raw_to_unicode(new_cell(to_wchar(str)), iencoder, enc_error_info);
    if (c == NULL) {
        return to_wchar(str);
    }
    
    return cell_get_addr(c);
}
