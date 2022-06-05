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
cell_clone(Cell *p) {
    Cell *c;
    
    if (NULL == p) return new_cell(L"");

    c = (Cell*)GC_MALLOC(sizeof(Cell));
    ALLOC_SAFE(c);
    memcpy(c, p, sizeof(Cell));
    c->data = (wchar_t*)GC_MALLOC(p->allocsize*sizeof(wchar_t));
    ALLOC_SAFE(c->data);
    memcpy(c->data, p->data, ((p->allocsize > p->length+1) ? p->length+1 : p->allocsize)*sizeof(wchar_t));
    
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
cell_add_cell(Cell *p, Cell *src) {
    int len;
    
    if (NULL == p) return NULL;
    if (NULL == src) return p;

    len = src->length+1;
    if ((len + p->length) > p->allocsize) {
	if (NULL == cell_realloc(p, len + p->length))
	    return NULL;
    }
    
    memcpy(&p->data[p->length], src->data, len*sizeof(wchar_t));
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
    if (end <= start) return d;
    if (start >= l) return d;

    if (end > l) {
	end = l;
    }

    for (i=start; i<end; i++) {
	cell_add_char(d, p[i]);
    }

    return d;
}

int
cell_cmp(Cell *src, Cell *dest) {
    int l;
    int result;
    
    l = (src->length > dest->length) ? dest->length : src->length; 
    result = wmemcmp(src->data, dest->data, l);
    if (result) return result;
    
    if (src->length == dest->length) return 0;
    if (src->length > dest->length) return 1;
    return -1;
}
