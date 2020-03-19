#include <string.h>
#include "cell.h"
#include "toy.h"

static int cell_get_alloc_size(int init_s, int dest_s);
static Cell* cell_realloc(Cell *p, int dest_s);

Cell*
new_cell(const wchar_t *src) {
    Cell *c;
    int alen, len;

    if (NULL == src) return new_cell(L"");
    
    c = (Cell*)GC_MALLOC(sizeof(Cell));
    ALLOC_SAFE(c);

    len = wcslen(src)+1;
    alen = cell_get_alloc_size(CELL_INIT_ALLOC, len);
    c->length = len-1;
    c->allocsize = alen;
    c->data = (wchar_t*)GC_MALLOC_ATOMIC(alen*sizeof(wchar_t));
    ALLOC_SAFE(c->data);
    wcsncpy(c->data, src, len);

    return c;
}

Cell*
cell_add_str(Cell *p, const wchar_t *src) {
    int len;

    if (NULL == p) return NULL;

    len = wcslen(src)+1;
    if ((len + p->length) > p->allocsize) {
	if (NULL == cell_realloc(p, len + p->length))
	    return NULL;
    }

    wcsncpy(&p->data[p->length], src, len);
    p->length = p->length + len-1;

    return p;
}

Cell*
cell_add_char(Cell *p, const wchar_t src) {
    int len;

    if (NULL == p) return NULL;

    len = 2;
    if ((len + p->length) > p->allocsize) {
	if (NULL == cell_realloc(p, len + p->length))
	    return NULL;
    }

    p->data[p->length] = src;
    p->data[p->length+1] = 0;
    p->length = p->length + len-1;

    return p;
}

int
cell_eq_str(Cell *s, wchar_t *d) {
    return wcscmp(cell_get_addr(s), d);
}

static int
cell_get_alloc_size(int init_s, int dest_s) {
    int i = init_s;

    while (i < dest_s) {
	i = i*2;
    }
    return i;
}

static Cell*
cell_realloc(Cell *p, int dest_s) {
    int i;
    wchar_t *data;

    i = cell_get_alloc_size(p->allocsize, dest_s);
    data = GC_REALLOC(p->data, i*sizeof(wchar_t));
    ALLOC_SAFE(data);

    p->data = data;
    p->allocsize = i;

    return p;
}

Cell*
cell_sub(Cell *c, int start, int end) {
    Cell *d;
    int i, l;
    const wchar_t *p;

    d = new_cell(L"");
    if (NULL == d) return d;

    l = c->length;
    p = c->data;

    if (end < 0) return d;
    if (start < 0) start = 0;
/*    if (end == 0) end = l; */
    if (end <= start) return d;
    if (start >= l) return d;

    if (end >= l) {
	return cell_add_str(d, &p[start]);
    }

    for (i=start; i<end; i++) {
	cell_add_char(d, p[i]);
    }

    return d;
}
