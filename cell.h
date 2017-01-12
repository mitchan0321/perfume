/* $Id: cell.h,v 1.4 2009/03/23 14:03:37 mit-sato Exp $ */

#ifndef __CELL__
#define __CELL__

#include <t_gc.h>

#define CELL_INIT_ALLOC	(16)

typedef struct _cell {
    int length;
    int allocsize;
    char *data;
} Cell;

Cell*	new_cell(const char *src);
Cell*	cell_add_str(Cell *p, const char *src);
Cell*	cell_add_char(Cell *p, const char src);
char*	cell_get_addr(Cell *p);
int	cell_get_length(Cell *p);
int     cell_eq_str(Cell *s, char *d);
Cell*	cell_sub(Cell *c, int start, int end);

#define cell_get_addr(p)	(p->data)
#define cell_get_length(p)	(p->length)

#endif /* __CELL__ */
