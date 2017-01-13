#include <toy.h>
#include <util.h>
#include <string.h>

wchar_t*
to_wchar(const char *src) {
    wchar_t *dest;
    int len, i;
    
    len = strlen(src);
    dest = GC_MALLOC((len+1)*sizeof(wchar_t));
    ALLOC_SAFE(dest);
    for (i=0; i<=len; i++) {
	dest[i] = (wchar_t)src[i];
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
