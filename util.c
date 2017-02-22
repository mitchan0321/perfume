#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "toy.h"
#include "util.h"
#include "string.h"

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
