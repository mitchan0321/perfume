#ifndef __BINBULK__
#define __BINBULK__

#include <wchar.h>
#include <t_gc.h>

#ifndef EOF
#define EOF	(-1)
#endif

#define DEFAULT_BINBULK_SIZE 512

typedef struct _binbulk {
    int length;
    int allocsize;
    int pos;
    unsigned char *data;
} BinBulk;

/* Bulk standard interface */
BinBulk*	new_binbulk();
int	binbulk_get_length(BinBulk *bulk);
int	binbulk_alloc_size(BinBulk *bulk, int size);
int	binbulk_realloc_size(BinBulk *bulk, int size);
int	binbulk_add_char(BinBulk *bulk, wchar_t c);
wchar_t	binbulk_get_char(BinBulk *bulk);
wchar_t	binbulk_set_char(BinBulk *bulk, wchar_t c);
int	binbulk_seek(BinBulk *bulk, int pos);
int	binbulk_get_position(BinBulk *bulk);
int	binbulk_get_capacity(BinBulk *bulk);
int	binbulk_truncate(BinBulk *bulk, int size);
int	binbulk_read(BinBulk *bulk, int fd);
int	binbulk_write(BinBulk *bulk, int fd, int from, int to);

#endif /* __BINBULK__ */
