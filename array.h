#include "types.h"

#ifndef __ARRAY__
#define __ARRAY__

#define ARRAY_INIT_SIZE	(256)

typedef struct _array {
    int alloc_size;
    int cur_size;
    struct _toy_type *array;
} Array;

Array*		new_array();
Array*		array_append(Array *array, struct _toy_type *item);
Array*		array_set(Array *array, struct _toy_type *item, int pos);
struct _toy_type*
                array_get(Array *array, int pos);
int		array_get_size(Array *array);
int		array_swap(Array *array, int pos1, int pos2);
Array*		array_resize(Array *array, int new_size);
Array*          array_insert(Array *array, int pos, struct _toy_type *item);
struct _toy_type*
                array_delete(Array *array, int pos);

#endif /* __ARRAY__ */
