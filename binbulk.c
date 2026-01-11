#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "cell.h"
#include "encoding.h"
#include "toy.h"
#include "binbulk.h"

BinBulk*
new_binbulk() {
    BinBulk *bulk;

    bulk = GC_MALLOC(sizeof(BinBulk));
    ALLOC_SAFE(bulk);

    memset(bulk, 0, sizeof(BinBulk));

    binbulk_alloc_size(bulk, DEFAULT_BINBULK_SIZE);
    bulk->length = 0;
    bulk->pos = 0;

    return bulk;
}

int
binbulk_get_length(BinBulk *bulk) {
    if (NULL == bulk) return EOF;

    return bulk->length;
}

int
binbulk_alloc_size(BinBulk *bulk, int size) {
    bulk->length = size;
    bulk->allocsize = size;
    bulk->data = GC_MALLOC(size * sizeof(unsigned char));
    ALLOC_SAFE(bulk->data);
    memset(bulk->data, 0, size * sizeof(unsigned char));
    bulk->pos = 0;

    return 1;
}

int
binbulk_realloc_size(BinBulk *bulk, int size) {
    unsigned char *p;

    p = GC_REALLOC(bulk->data, size * sizeof(unsigned char));
    ALLOC_SAFE(p);
    bulk->data = p;
    bulk->allocsize = size;
    
    return 1;
}

int
binbulk_add_char(BinBulk *bulk, wchar_t c) {
    // set seek position to eof
    bulk->pos = bulk->length;
    
    if (bulk->allocsize <= (bulk->pos + 1)) {
        binbulk_realloc_size(bulk, bulk->allocsize * 2);
    }
    bulk->data[bulk->pos] = (unsigned char)(c & 0xff);
    bulk->pos ++;
    bulk->length ++;

    return 1;
}

wchar_t
binbulk_get_char(BinBulk *bulk) {
    if (bulk->pos < bulk->length) {
        wchar_t c;
        c = (wchar_t)bulk->data[bulk->pos];
        bulk->pos ++;
        return c;
    }

    return -1;
}

int
binbulk_is_eof(BinBulk *bulk) {
    if (bulk->pos >= bulk->length) return 1;
    return 0;
}

wchar_t
binbulk_set_char(BinBulk *bulk, wchar_t c) {
    if (bulk->pos < bulk->length) {
        bulk->data[bulk->pos] = (unsigned char)(c & 0xff);
        bulk->pos ++;
        return c;
    }

    return -1;
}

int
binbulk_seek(BinBulk *bulk, int pos) {
    if ((pos <= bulk->length) || (pos == 0)) {
        int orig_pos;
        orig_pos = bulk->pos;
        bulk->pos = pos;
        return orig_pos;
    }

    return -1;
}

int
binbulk_get_position(BinBulk *bulk) {
    return bulk->pos;
}

int
binbulk_get_capacity(BinBulk *bulk) {
    return bulk->allocsize;
}

int
binbulk_truncate(BinBulk *bulk, int size) {
    if (size <= bulk->length) {
        bulk->length = size;
        return size;
    }

    return -1;
}

int
binbulk_read(BinBulk *bulk, int fd) {
    struct stat statbuff;
    int size;

    if (-1 == fstat(fd, &statbuff)) {
        return -1;
    }

    if (S_ISREG(statbuff.st_mode)) {
        size = statbuff.st_size;
        binbulk_realloc_size(bulk, size);
        errno = 0;
        if (-1 == read_size(fd, (char*)bulk->data, size)) {
            return -1;
        }
        bulk->pos = 0;
        bulk->length = size;
        return size;
    }

    return -1;
}

int
binbulk_write(BinBulk *bulk, int fd, int from, int to) {
    struct stat statbuff;
    int size;

    if (-1 == fstat(fd, &statbuff)) {
        return -1;
    }

    if (S_ISREG(statbuff.st_mode)) {
        if (from < 0) return -1;
        if (from > bulk->length) return -1;
        if (to < 0) return -1;
        if (to > bulk->length) return -1;
                                      
        if (0 == to) to = bulk->length;
        size = to - from;
        if (size < 0) return -1;
        if (size > bulk->length) return -1;

        errno = 0;
        if (-1 == write_size(fd, (char*)&(bulk->data[from]), size)) {
            return -1;
        }
        return size;
    }

    return -1;
}

static const wchar_t int_to_base64_char[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

Cell*
binbulk_base64_encode(BinBulk *bulk, int count_bytes) {
    int b64_code[4];
    int i;
    Cell *result;
    int c1, c2, c3;
    
    result = new_cell(NULL);

    for (i=0; i<(count_bytes/3); i++) {
        c1 = binbulk_get_char(bulk);
        c2 = binbulk_get_char(bulk);
        c3 = binbulk_get_char(bulk);

        b64_code[0] = b64_code[1] = b64_code[2] = b64_code[3] = 64;

        if (c1 >= 0) {
            b64_code[0] = (c1 >> 2);
            b64_code[1] = ((c1 & 0x03) << 4);
            if (c2 >= 0) {
                b64_code[1] |= ((c2 >> 4) & 0x0f);
                b64_code[2] = ((c2 & 0x0f) << 2);
                if(c3 >= 0) {
                    b64_code[2] |= ((c3 >> 6) & 0x03);
                    b64_code[3] = (c3 & 0x3f);
                }
            }
        } else {
            break;
        }

        cell_add_char(result, int_to_base64_char[b64_code[0]]);
        cell_add_char(result, int_to_base64_char[b64_code[1]]);
        cell_add_char(result, int_to_base64_char[b64_code[2]]);
        cell_add_char(result, int_to_base64_char[b64_code[3]]);

        if ((c1 == -1) || (c2 == -1) || (c3 == -1)) break;
    }

    return result;
}

static const int base64_char_to_int[] = {
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,  //   0 -  15
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,  //  16 -  31
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, 62, -2, -2, -2, 63,  //  32 -  47
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -2, -2, -2, -1, -2, -2,  //  48 -  63
    -2,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,  //  64 -  79
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -2, -2, -2, -2, -2,  //  80 -  95
    -2, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,  //  96 - 111
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -2, -2, -2, -2, -2,  // 112 - 127
};

int
binbulk_base64_decode(BinBulk *bulk, Cell *b64) {
    wchar_t *p;
    wchar_t b64_char[4];
    int c1, c2, c3;
    int pad;

    p = cell_get_addr(b64);
    for (;;) {
        if (! *p) return 1;
        if (*p > 127) return 0;
        b64_char[0] = base64_char_to_int[*p];
        if (b64_char[0] == -2) return 0;
        p++;
        
        if (! *p) return 0;
        if (*p > 127) return 0;
        b64_char[1] = base64_char_to_int[*p];
        if (b64_char[1] == -2) return 0;
        p++;
        
        if (! *p) return 0;
        if (*p > 127) return 0;
        b64_char[2] = base64_char_to_int[*p];
        if (b64_char[2] == -2) return 0;
        p++;
        
        if (! *p) return 0;
        if (*p > 127) return 0;
        b64_char[3] = base64_char_to_int[*p];
        if (b64_char[3] == -2) return 0;
        p++;

        c1 = c2 = c3 = 0;
        pad = 0;
        if (b64_char[0] >= 0) {
            c1 = (b64_char[0] << 2);
            if (b64_char[1] >= 0) {
                c1 |= ((b64_char[1] >> 4) & 0x03);
                c2 = ((b64_char[1] & 0x0f) << 4);
                if (b64_char[2] >= 0) {
                    pad = 1;
                    c2 |= ((b64_char[2] >> 2) & 0x0f);
                    c3 = ((b64_char[2] & 0x03) << 6);
                    if (b64_char[3] >= 0) {
                        pad = 2;
                        c3 |= b64_char[3];
                    }
                }
            }
        }

        if (pad >= 0) binbulk_add_char(bulk, c1);
        if (pad >= 1) binbulk_add_char(bulk, c2);
        if (pad >= 2) binbulk_add_char(bulk, c3);
    }

    return 1;
}
