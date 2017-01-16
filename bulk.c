/* $Id: bulk.c,v 1.3 2009/07/20 14:32:04 mit-sato Exp $ */

#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "bulk.h"
#include "toy.h"

static int read_size(int fd, char* buff, int size);

Bulk*
new_bulk() {
    Bulk *bulk;

    bulk = GC_MALLOC(sizeof(Bulk));
    ALLOC_SAFE(bulk);

    memset(bulk, 0, sizeof(Bulk));
    bulk->line = 1;

    return bulk;
}

int
bulk_load_file(Bulk *bulk, const char *file) {
    int fd;
    struct stat statbuff;
    int size;
    char *readbuff;

    if (NULL == bulk) return 0;

    if (-1 == (fd = open(file, O_RDONLY))) return 0;

    if (-1 == fstat(fd, &statbuff)) goto error;
    size = statbuff.st_size;

    bulk->length = size;
    bulk->allocsize = (size+1)*sizeof(wchar_t);
    bulk->pos = 0;
    bulk->line = 1;

    readbuff = GC_MALLOC_ATOMIC(size+1);
    ALLOC_SAFE(readbuff);

    if (-1 == read_size(fd, readbuff, size)) goto error;
    bulk->data = to_wchar(readbuff);

    close(fd);
    return 1;

error:
    close(fd);
    bulk->length = 0;
    bulk->allocsize = 0;
    bulk->pos = 0;
    bulk->line = 1;
    bulk->data = 0;

    return 0;
}

static int
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
bulk_set_string(Bulk *bulk, const wchar_t *str) {
    int len;

    if (NULL == bulk) return 0;
    if (NULL == str) return 0;

    len = wcslen(str);
    bulk->data = GC_MALLOC_ATOMIC((len+1)*sizeof(wchar_t));
    ALLOC_SAFE(bulk->data);

    bulk->length = len;
    bulk->allocsize = (len+1)*sizeof(wchar_t);
    bulk->pos = 0;
    bulk->line = 1;

    memcpy(bulk->data, str, len*sizeof(wchar_t));

    return 1;
}

int
bulk_rewind(Bulk *bulk) {
    if (NULL == bulk) return 0;
    if (NULL == bulk->data) return 0;

    bulk->pos = 0;
    bulk->line = 1;

    return 1;
}

int
bulk_getchar(Bulk *bulk) {
    int c;

    if (NULL == bulk) return EOF;
    if (EOF == bulk_is_eof(bulk)) return EOF;

    c = bulk->data[bulk->pos];
    if (L'\n' == c) {
	bulk->line++;
    }
    bulk->pos++;

    return c;
}

int
bulk_ungetchar(Bulk *bulk) {

    if (NULL == bulk) return EOF;
    if (bulk->pos > 0) bulk->pos--;
    if (L'\n' == bulk->data[bulk->pos]) {
	bulk->line--;
    }

    return 1;
}

int
bulk_is_eof(Bulk *bulk) {
    if (NULL == bulk) return EOF;
    if (NULL == bulk->data) return EOF;

    if ((bulk->pos) >= (bulk->length)) return EOF;

    return 1;
}

int
bulk_get_length(Bulk *bulk) {
    if (NULL == bulk) return EOF;

    return bulk->length;
}

int
bulk_get_position(Bulk *bulk) {
    if (NULL == bulk) return EOF;

    return bulk->pos;
}

int
bulk_set_position(Bulk *bulk, int pos) {
    if (NULL == bulk) return 0;

    if (EOF == bulk_is_eof(bulk)) return EOF;

    if (pos >= bulk->length) {
	bulk->pos = bulk->length;
	return EOF;
    }

    bulk->pos = pos;

    return 1;
}

wchar_t*
bulk_get_addr(Bulk *bulk) {
    if (NULL == bulk) return 0;

    return bulk->data;
}

int
bulk_get_line(Bulk *bulk) {
    if (NULL == bulk) return 0;

    return bulk->line;
}
