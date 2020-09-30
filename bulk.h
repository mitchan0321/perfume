#ifndef __BULK__
#define __BULK__

#include <wchar.h>
#include <t_gc.h>

#ifndef EOF
#define EOF	(-1)
#endif

#define IS_NEWLINE(x)	((x == L'\n') || (x == L'\r'))

typedef struct _bulk {
    int length;
    int allocsize;
    int pos;
    int line;
    wchar_t *data;
} Bulk;

Bulk*	new_bulk();
int	bulk_load_file(Bulk *bulk, const char *file, int encoder);
int	bulk_set_string(Bulk *bulk, const wchar_t *str);
int	bulk_rewind(Bulk *bulk);
int	bulk_getchar(Bulk *bulk);
int	bulk_ungetchar(Bulk *bulk);
int	bulk_is_eof(Bulk *bulk);
int	bulk_get_length(Bulk *bulk);
int	bulk_get_position(Bulk *bulk);
int	bulk_set_position(Bulk *bulk, int pos);
int	bulk_get_line(Bulk *bulk);
wchar_t*bulk_get_addr(Bulk *bulk);

#endif /* __BULK__ */
