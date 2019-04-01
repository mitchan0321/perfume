/* $Id: array.c,v 1.4 2011/08/28 10:51:29 mit-sato Exp $ */

#include <string.h>
#include "types.h"
#include "array.h"
#include "toy.h"

static Array* array_realloc(Array*);

Array*
new_array() {
    Array *array;

    array = GC_MALLOC(sizeof(Array));
    ALLOC_SAFE(array);

    array->array = GC_MALLOC(sizeof(Toy_Type) * ARRAY_INIT_SIZE);
    ALLOC_SAFE(array->array);
    memset(array->array, 0, sizeof(Toy_Type) * ARRAY_INIT_SIZE);

    array->alloc_size = ARRAY_INIT_SIZE;
    array->cur_size = 0;

    return array;
}

Array*
array_append(Array *array, Toy_Type *item) {
    if (NULL == array) return NULL;

    if ((array->cur_size + 1) > array->alloc_size) {
	if (NULL == array_realloc(array)) return NULL;
    }

    array->array[array->cur_size] = *item;
    array->cur_size++;

    return array;
}

Array*
array_set(Array *array, Toy_Type *item, int pos) {
    if (NULL == array) return NULL;

    if ((pos < 0) || (pos >= array->cur_size)) return NULL;

    array->array[pos] = *item;

    return array;
}

struct _toy_type*
array_get(Array *array, int pos) {
    if (NULL == array) return NULL;

    if ((pos < 0) || (pos >= array->cur_size)) return NULL;

    return &array->array[pos];
}

int
array_get_size(Array *array) {
    if (NULL == array) return -1;

    return array->cur_size;
}

int
array_swap(Array *array, int pos1, int pos2) {
    Toy_Type t;

    if (NULL == array) return 0;

    if ((pos1 < 0) || (pos1 >= array->cur_size)) return 0;
    if ((pos2 < 0) || (pos2 >= array->cur_size)) return 0;

    t = array->array[pos1];
    array->array[pos1] = array->array[pos2];
    array->array[pos2] = t;

    return 1;
}


static Array*
array_realloc(Array* array) {
    Toy_Type *orig_array, *new_array;
    int alloc_size;
    int i;

    alloc_size = array->alloc_size * 2;
    orig_array = array->array;

    new_array = GC_MALLOC(sizeof(Toy_Type) * alloc_size);
    ALLOC_SAFE(new_array);
    memset(new_array, 0, sizeof(Toy_Type) * alloc_size);

    for (i=0; i<array->cur_size; i++) {
	new_array[i] = orig_array[i];
    }

    array->alloc_size = alloc_size;
    array->array = new_array;

    return array;
}
