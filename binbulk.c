#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "binbulk.h"
#include "cell.h"
#include "encoding.h"
#include "toy.h"

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
