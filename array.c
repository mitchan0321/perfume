#include <string.h>
#include "types.h"
#include "array.h"
#include "toy.h"

static Array* array_realloc(Array*);
static Array* array_realloc_size(Array*, int);

Array*
new_array() {
    Array *array;

    array = GC_MALLOC(sizeof(Array));
    ALLOC_SAFE(array);

    array->array = GC_MALLOC(sizeof(struct _toy_type) * ARRAY_INIT_SIZE);
    ALLOC_SAFE(array->array);
    memset(array->array, 0, sizeof(struct _toy_type) * ARRAY_INIT_SIZE);

    array->alloc_size = ARRAY_INIT_SIZE;
    array->cur_size = 0;

    return array;
}

Array*
array_append(Array *array, struct _toy_type *item) {
    if (NULL == array) return NULL;

    if ((array->cur_size + 1) > array->alloc_size) {
	if (NULL == array_realloc(array)) return NULL;
    }

    array->array[array->cur_size] = *item;
    array->cur_size++;

    return array;
}

Array*
array_set(Array *array, struct _toy_type *item, int pos) {
    if (NULL == array) return NULL;

    if ((pos < 0) || (pos >= array->cur_size)) return NULL;

    array->array[pos] = *item;

    return array;
}

struct _toy_type*
array_get(Array *array, int pos) {
    struct _toy_type *item;

    if (NULL == array) return NULL;

    if ((pos < 0) || (pos >= array->cur_size)) return NULL;

    item = GC_MALLOC(sizeof(struct _toy_type));
    ALLOC_SAFE(item);
    *item = array->array[pos];

    return item;
}

int
array_get_size(Array *array) {
    if (NULL == array) return -1;

    return array->cur_size;
}

int
array_swap(Array *array, int pos1, int pos2) {
    struct _toy_type t;

    if (NULL == array) return 0;

    if ((pos1 < 0) || (pos1 >= array->cur_size)) return 0;
    if ((pos2 < 0) || (pos2 >= array->cur_size)) return 0;

    t = array->array[pos1];
    array->array[pos1] = array->array[pos2];
    array->array[pos2] = t;

    return 1;
}

Array*
array_resize(Array *array, int new_size) {
    if (NULL == array) return NULL;

    if (new_size < 0) return NULL;
    return array_realloc_size(array, new_size);
}


static Array*
array_realloc(Array* array) {
    struct _toy_type *orig_array, *new_array;
    int alloc_size;
    int i;

    alloc_size = array->alloc_size * 2;
    orig_array = array->array;

    new_array = GC_MALLOC(sizeof(struct _toy_type) * alloc_size);
    ALLOC_SAFE(new_array);
    memset(new_array, 0, sizeof(struct _toy_type) * alloc_size);

    for (i=0; i<array->cur_size; i++) {
	new_array[i] = orig_array[i];
    }

    array->alloc_size = alloc_size;
    array->array = new_array;

    return array;
}

static Array*
array_realloc_size(Array* array, int size) {
    struct _toy_type *new_t;
    int i;

    if (size < 0) return NULL;
    
    if (size <= array->alloc_size) {
	array->cur_size = size;
	for (i=size; i<array->alloc_size; i++) {
	    memset(&array->array[i], 0, sizeof(struct _toy_type));
	}
	return array;
    }

    new_t = GC_MALLOC(sizeof(struct _toy_type) * ((size>0) ? size : ARRAY_INIT_SIZE));
    ALLOC_SAFE(new_t);
    memset(new_t, 0, sizeof(struct _toy_type) * ((size>0) ? size : ARRAY_INIT_SIZE));

    for (i=0; i<size; i++) {
	if (i >= array->cur_size) break;
	new_t[i] = array->array[i];
    }
    
    array->array = new_t;
    array->cur_size = size;
    array->alloc_size = ((size>0) ? size : ARRAY_INIT_SIZE);

    return array;
}

Array*
array_insert(Array *array, int pos, struct _toy_type *item) {
    int i;
    
    if (NULL == array) return NULL;
    if (pos < 0) return NULL;
    
    if (pos >= array->cur_size) {
        return array_append(array, item);
    }

    if ((array->cur_size + 1) > array->alloc_size) {
	if (NULL == array_realloc(array)) return NULL;
    }
    
    array->cur_size ++;
    for (i=array->cur_size - 1; i>pos; i--) {
        array->array[i] = array->array[i-1];
    }
    array->array[pos] = *item;
    
    return array;
}

struct _toy_type*
array_delete(Array *array, int pos) {
    int i;
    struct _toy_type *item;
    
    if (NULL == array) return NULL;
    if (pos < 0) return NULL;
    if (pos >= array->cur_size) return NULL;

    item = GC_MALLOC(sizeof(struct _toy_type));
    ALLOC_SAFE(item);
    *item = array->array[pos];

    array->cur_size --;
    for (i=pos; i<array->cur_size; i++) {
        array->array[i] = array->array[i+1];
    }
    
    return item;
}
