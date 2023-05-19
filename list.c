#include <string.h>
#include "types.h"
#include "toy.h"

Toy_Type*
new_list(Toy_Type *item) {
    Toy_Type *list;

    list = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(list);
    memset((void*)list, 0, sizeof(Toy_Type));

    list->tag = LIST;
    list->u.list.nextp = NULL;
    list->u.list.item = item;

    return list;
}

Toy_Type*
new_cons(Toy_Type *car, Toy_Type *cdr) {
    Toy_Type *list;

    list = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(list);
    memset((void*)list, 0, sizeof(Toy_Type));

    list->tag = LIST;
    list->u.list.item = car;
    list->u.list.nextp = cdr;

    return list;
}

Toy_Type*
list_append(Toy_Type *list, Toy_Type *item) {
    if (list == NULL) return NULL;

    if (IS_LIST_NULL(list)) {
	list->u.list.item = item;
	return list;
    }

    if (NULL == list->u.list.nextp) {
	list->u.list.nextp = new_list(item);
	return list->u.list.nextp;
    }

    while (list->u.list.nextp) {
	if (GET_TAG(list->u.list.nextp) != LIST) {
	    list->u.list.nextp = new_list(item);
	    return list->u.list.nextp;
	}
	list = list->u.list.nextp;
    }
    list->u.list.nextp = new_list(item);

    return list->u.list.nextp;
}

/*
Toy_Type*
list_next(Toy_Type *list) {
    if (list == NULL) return NULL;

    return list->u.list.nextp;
}

Toy_Type*
list_get_item(Toy_Type *list) {
    if (list == NULL) return 0;

    return list->u.list.item;
}
*/

int
list_length(Toy_Type *list) {
    int length = 1;

    if (NULL == list) return 0;

    if (IS_LIST_NULL(list)) return 0;

    while (list->u.list.nextp) {
	list = list->u.list.nextp;
	length++;
	if (GET_TAG(list) != LIST) return length;
    }

    return length;
}

