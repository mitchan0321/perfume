#ifndef __CELL__
#define __CELL__

#include <wchar.h>
#include <t_gc.h>

#define CELL_INIT_ALLOC	(16)

typedef struct _cell {
    int length;
    int allocsize;
    wchar_t *data;
} Cell;

Cell*	new_cell(const wchar_t *src);
Cell*	cell_clone(Cell *p);
Cell*	cell_add_str(Cell *p, const wchar_t *src);
Cell*	cell_add_cell(Cell *p, Cell *src);
Cell*	cell_add_char(Cell *p, const wchar_t src);
wchar_t* cell_get_addr(Cell *p);
int	cell_get_length(Cell *p);
int     cell_eq_str(Cell *s, wchar_t *d);
Cell*	cell_sub(Cell *c, int start, int end);
int     cell_cmp(Cell *src, Cell *dest);

#define cell_get_addr(p)	(p->data)
#define cell_get_length(p)	(p->length)

#endif /* __CELL__ */
