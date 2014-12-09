/* $Id: array.h,v 1.2 2011/08/28 10:51:29 mit-sato Exp $ */

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
struct _toy_type* array_get(Array *array, int pos);
int		array_get_size(Array *array);

#endif /* __ARRAY__ */
