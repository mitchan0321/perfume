#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include "types.h"
#include "cell.h"
#include "global.h"

static struct hash_bucket* hash_add_to_bucket(
    Hash *hash,
    struct hash_bucket *bucket,
    const unsigned int org_index,
    const wchar_t *key,
    const int key_len,
    const struct _toy_type *item,
    int bucket_size,
    int key_re_alloc
);
static Hash* hash_rehash(Hash *hash);

#define HASH_MAKE_KEY(k)	(hash_string_key(k))

/*
int hash_bsize[] = {
    7, 13, 31, 61, 127, 251, 509, 1021, 2039, 4093, 8191,
    16381, 32749, 65521, 131071, 262139, 524287, 1048573,
    2097151, 4194301, 8388607, 16777213, 33554467, 67108879,
//    134217757, 268435459, 536870923, 1073741827,
    0
};
*/

Hash*
new_hash() {
    Hash *h;

    h = GC_MALLOC(sizeof(Hash));
    ALLOC_SAFE(h);

    hash_clear(h);

    return h;
}

void
hash_clear(Hash* h) {
    h->bucket_size = HASH_INIT_BUCKET;
    h->bucket = NULL;
    h->items = 0;
    h->synonyms = 0;
}

Hash*
hash_set(Hash *hash, const wchar_t *key, const struct _toy_type *item) {
    unsigned int index;

    if (NULL == hash) return NULL;

    if (((hash->items)*2) > (hash->bucket_size)) {
	if (NULL == hash_rehash(hash)) {
	    return NULL;
	}
    }

    index = HASH_MAKE_KEY(key);

    if (NULL == hash_add_to_bucket(
            hash,
            hash->bucket,
            index,
            key,
            wcslen(key) + 1,
            item,
            hash->bucket_size,
            1
        ))
    {
	return NULL;
    }

    return hash;
}

Hash*
hash_set_t(Hash *hash, const struct _toy_type *key, const struct _toy_type *item) {
    int t;
    unsigned int idx;
    Cell *caddr;

    if (NULL == hash) return NULL;

    if (((hash->items)*2) > (hash->bucket_size)) {
	if (NULL == hash_rehash(hash)) {
	    return NULL;
	}
    }

    t = GET_TAG(key);
    if ((t == SYMBOL) || (t == REF)) {
	if (SYMBOL == t) {
	    idx =  key->u.symbol.hash_index;
	    caddr = key->u.symbol.cell;
	} else {
	    idx = key->u.ref.hash_index;
	    caddr = key->u.ref.cell;
	}

	if (NULL ==
	    hash_add_to_bucket(
                hash,
                hash->bucket,
                idx,
                cell_get_addr(caddr),
                cell_get_length(caddr) + 1,
                item,
                hash->bucket_size,
                1
            ))
        {
	    return NULL;
	}

	return hash;
    }

    assert(0);

    return NULL;
}

struct hash_bucket*
hash_add_to_bucket(
    Hash *hash,
    struct hash_bucket *bucket,
    const unsigned int org_index,
    const wchar_t *key,
    const int key_len,
    const struct _toy_type *item,
    int bucket_size,
    int key_re_alloc
) {
    wchar_t *dest_key;
    struct hash_bucket *b, *bs;
    unsigned int index;

    index = org_index % bucket_size;

    if (hash->bucket == NULL) {
	int bsize;
        struct hash_bucket *lbucket;

	bsize = sizeof(struct hash_bucket) * hash->bucket_size;
        lbucket = GC_MALLOC(bsize);
	ALLOC_SAFE(lbucket);
	memset(lbucket, 0, bsize);
	hash->bucket = lbucket;
        bucket = lbucket;
    }

    b = &(bucket[index]);
    if (NULL == b->key) {
	
	/* new item into new bucket */

	if (key_re_alloc) {
	    dest_key = (wchar_t*)GC_MALLOC(key_len*sizeof(wchar_t));
	    ALLOC_SAFE(dest_key);
	    wcsncpy(dest_key, key, key_len);
	    b->key = dest_key;
	} else {
	    b->key = (wchar_t*)key;
	}

	b->index = org_index;
	b->item = (struct _toy_type*)item;
	b->next = 0;

	hash->items++;

	return bucket;
    }

    while (1) {
	if (0 == wcscmp(b->key, key)) {
	    if (GET_TAG(b->item) == ALIAS) {

		/* set to alias slot */

		if (NULL == hash_set_t(b->item->u.alias.slot, b->item->u.alias.key, item)) {
		    return NULL;
		} else {
		    return bucket;
		}
	    } else {

		/* replace item */

		b->item = (struct _toy_type*)item;
		return bucket;
	    }
	}

        if (! b->next) break;
	b = b->next;
    }

    /* new item add to synonym */

    bs = GC_MALLOC(sizeof(struct hash_bucket));
    ALLOC_SAFE(bs);

    if (key_re_alloc) {
	dest_key = GC_MALLOC(key_len*sizeof(wchar_t));
	ALLOC_SAFE(dest_key);
	wcsncpy(dest_key, key, key_len);
	bs->key = dest_key;
    } else {
	bs->key = (wchar_t*)key;
    }

    bs->index = org_index;
    bs->item = (struct _toy_type*)item;
    bs->next = 0;
    b->next = bs;

    hash->items++;
    hash->synonyms++;

    return bucket;
}

static int next_size(int now_size);

static int next_size(int now_size) {
    int i;

    /*
    for (i=0; hash_bsize[i]!=0; i++) {
	if (now_size == hash_bsize[i]) {
	    i++;
	    return hash_bsize[i];
	}
    }
    */

    i = now_size * 2;
    if (i < 0) {
        return now_size;
    }
    return i;
}

Hash*
hash_rehash(Hash *hash) {
    int new_bucket_size;
    struct hash_bucket *b, *newb;
    int bsize, i;

    new_bucket_size = next_size(hash->bucket_size);
    if (new_bucket_size == 0) return hash;

    bsize = sizeof(struct hash_bucket) * new_bucket_size;
    newb = GC_MALLOC(bsize);
    ALLOC_SAFE(newb);
    memset(newb, 0, bsize);

    hash->items = 0;
    hash->synonyms = 0;

    for (i=0; i<(hash->bucket_size); i++) {
        b = &(hash->bucket[i]);
	if (NULL != b->key) {
	    while (b) {
		if (NULL ==
		    hash_add_to_bucket(
                        hash,
                        newb,
                        b->index,
                        b->key,
                        wcslen(b->key) + 1,
                        b->item,
                        new_bucket_size,
                        0
                    ))
                {
		    return NULL;
		}
		b = b->next;
	    }
	}
    }

    memset(hash->bucket, 0, sizeof(struct hash_bucket) * hash->bucket_size);
    hash->bucket_size = new_bucket_size;
    hash->bucket = newb;

    return hash;
}

struct _toy_type*
hash_get(Hash *hash, const wchar_t *key) {
    unsigned int index;
    struct hash_bucket *b;

    if (NULL == hash) return NULL;
    if (NULL == hash->bucket) return NULL;

    index = HASH_MAKE_KEY(key) % hash->bucket_size;

    b = &(hash->bucket[index]);
    if (NULL == b->key) return NULL;
    
    while (b) {
	if (0 == wcscmp(b->key, key)) {
	    if (GET_TAG(b->item) == ALIAS) {
		return hash_get_t(b->item->u.alias.slot, b->item->u.alias.key);
	    } else {
		return b->item;
	    }
	}
	b = b->next;
    }

    return NULL;
}

static struct _toy_type* hash_get_alias(Hash *hash, const wchar_t *key) ;

static struct _toy_type*
hash_get_alias(Hash *hash, const wchar_t *key) {
    unsigned int index;
    struct hash_bucket *b;

    if (NULL == hash) return NULL;
    if (NULL == hash->bucket) return NULL;

    index = HASH_MAKE_KEY(key) % hash->bucket_size;

    b = &(hash->bucket[index]);
    if (NULL == b->key) return NULL;
    
    while (b) {
	if (0 == wcscmp(b->key, key)) {
	    return b->item;
	}
	b = b->next;
    }

    return NULL;
}

struct _toy_type*
hash_get_t(Hash *hash, const struct _toy_type *key) {
    unsigned int index;
    struct hash_bucket *b;
    int t;
    Cell *caddr;

    if (NULL == hash) return NULL;
    if (NULL == hash->bucket) return NULL;

    t = GET_TAG(key);
    if ((t == SYMBOL) || (t == REF)) {
	if (SYMBOL == t) {
	    index = key->u.symbol.hash_index % hash->bucket_size;
	    caddr = key->u.symbol.cell;
	} else {
	    index = key->u.ref.hash_index % hash->bucket_size;
	    caddr = key->u.ref.cell;
	}

	b = &(hash->bucket[index]);
        if (NULL == b->key) return NULL;
        
	while (b) {
	    if (0 == wcscmp(b->key, cell_get_addr(caddr))) {
		if (GET_TAG(b->item) == ALIAS) {
		    return hash_get_t(b->item->u.alias.slot, b->item->u.alias.key);
		} else {
		    return b->item;
		}
	    }
	    b = b->next;
	}

	return NULL;
    }

    assert(0);

    return NULL;
}


static struct _toy_type* hash_get_and_unset_sub(Hash *hash, unsigned int index, const wchar_t *key);

struct _toy_type*
hash_get_and_unset(Hash *hash, const wchar_t *key) {
    unsigned int index;

    if (NULL == hash) return NULL;
    if (NULL == hash->bucket) return NULL;

    index = HASH_MAKE_KEY(key);
    return hash_get_and_unset_sub(hash, index, key);
}

struct _toy_type*
hash_get_and_unset_t(Hash *hash, const struct _toy_type *key) {
    unsigned int index;
    int t;
    Cell *caddr;

    if (NULL == hash) return NULL;
    if (NULL == hash->bucket) return NULL;

    t = GET_TAG(key);
    if ((t == SYMBOL) || (t == REF)) {
	if (SYMBOL == t) {
	    index = key->u.symbol.hash_index % hash->bucket_size;
	    caddr = key->u.symbol.cell;
	} else {
	    index = key->u.ref.hash_index % hash->bucket_size;
	    caddr = key->u.ref.cell;
	}

	return hash_get_and_unset_sub(hash, index, cell_get_addr(caddr));
    }

    assert(0);

    return NULL;
}

static struct _toy_type*
hash_get_and_unset_sub(Hash *hash, unsigned int org_index, const wchar_t *key) {
    struct hash_bucket *b, *bb;
    struct _toy_type *item;
    unsigned int index;

    index = org_index % hash->bucket_size;
    b = &(hash->bucket[index]);
    if (NULL == b->key) return NULL;
    
    bb = NULL;
    while (b) {
	if (0 == wcscmp(b->key, key)) {
            item = b->item;
            if (bb) {
                bb->next = b->next;
                b->key = NULL;
                b->index = 0;
                b->item = NULL;
                b->next = NULL;
            } else {
                if (b->next) {
                    struct hash_bucket *next;
                    next = b->next;
                    b->key = b->next->key;
                    b->index = b->next->index;
                    b->item = b->next->item;
                    b->next = b->next->next;
                    next->key = NULL;
                    next->index = 0;
                    next->item = NULL;
                    next->next = NULL;
                } else {
                    b->key = NULL;
                    b->index = 0;
                    b->item = NULL;
                    b->next = NULL;
                }
            }
            hash->items--;
            
	    if (GET_TAG(item) == ALIAS) {
		return hash_get_t(item->u.alias.slot, item->u.alias.key);
            } else {
                return item;
	    }
        }
        bb = b;
	b = b->next;
    }

    return NULL;
}

static int
hash_is_exists_sub(Hash *hash, unsigned int index, const wchar_t *key);

int
hash_is_exists(Hash *hash, const wchar_t *key) {
    unsigned int index;

    if (NULL == hash) return 0;
    if (NULL == hash->bucket) return 0;

    index = HASH_MAKE_KEY(key);
    return hash_is_exists_sub(hash, index, key);
}

int
hash_is_exists_t(Hash *hash, const struct _toy_type *key) {
    unsigned int index;
    int t;
    Cell *caddr;

    if (NULL == hash) return 0;
    if (NULL == hash->bucket) return 0;

    t = GET_TAG(key);
    if ((t == SYMBOL) || (t == REF)) {
	if (SYMBOL == t) {
	    index = key->u.symbol.hash_index % hash->bucket_size;
	    caddr = key->u.symbol.cell;
	} else {
	    index = key->u.ref.hash_index % hash->bucket_size;
	    caddr = key->u.ref.cell;
	}

	return hash_is_exists_sub(hash, index, cell_get_addr(caddr));
    }

    assert(0);

    return 0;
}

static int
hash_is_exists_sub(Hash *hash, unsigned int org_index, const wchar_t *key) {
    struct hash_bucket *b;
    unsigned int index;

    index = org_index % hash->bucket_size;
    b = &(hash->bucket[index]);
    if (NULL == b->key) return 0;
    
    while (b) {
	if (0 == wcscmp(b->key, key)) {
	    return 1;
	}
	b = b->next;
    }

    return 0;
}

Toy_Type*
hash_get_keys(Hash *hash) {
    Toy_Type *l, *sl;
    int i;
    struct hash_bucket *b;

    if (NULL == hash) return NULL;

    l = new_list(NULL);

    if (NULL == hash->bucket) return l;

    sl = l;

    for (i=0; i<(hash->bucket_size); i++) {
        b = &(hash->bucket[i]);
        if (NULL == b->key) continue;

        while (b) {
            sl = list_append(sl, new_symbol(b->key));
            b = b->next;
        }
    }

    return l;
}

Toy_Type*
hash_get_keys_str(Hash *hash) {
    Toy_Type *l, *sl;
    int i;
    struct hash_bucket *b;

    if (NULL == hash) return NULL;

    l = new_list(NULL);

    if (NULL == hash->bucket) return l;

    sl = l;

    for (i=0; i<(hash->bucket_size); i++) {
        b = &(hash->bucket[i]);
        if (NULL == b->key) continue;

        while (b) {
            sl = list_append(sl, new_string_str(b->key));
            b = b->next;
	}
    }

    return l;
}

Toy_Type*
hash_get_pairs(Hash *hash) {
    Toy_Type *l, *sl, *p;
    int i;
    struct hash_bucket *b;

    if (NULL == hash) return NULL;

    l = new_list(NULL);

    if (NULL == hash->bucket) return l;

    sl = l;

    for (i=0; i<(hash->bucket_size); i++) {
        b = &(hash->bucket[i]);
        if (NULL == b->key) continue;

        while (b) {
            if (GET_TAG(b->item) == ALIAS) {
                Toy_Type *v;
                v = hash_get_t(b->item->u.alias.slot, b->item->u.alias.key);
                if (NULL == v) {
                    p = new_cons(new_symbol(b->key), const_Nil);
                } else {
                    p = new_cons(new_symbol(b->key), v);
                }
            } else {
                p = new_cons(new_symbol(b->key), b->item);
            }
            sl = list_append(sl, p);
            b = b->next;
	}
    }

    return l;
}

Toy_Type*
hash_get_pairs_str(Hash *hash) {
    Toy_Type *l, *sl, *p;
    int i;
    struct hash_bucket *b;

    if (NULL == hash) return NULL;

    l = new_list(NULL);

    if (NULL == hash->bucket) return l;

    sl = l;

    for (i=0; i<(hash->bucket_size); i++) {
        b = &(hash->bucket[i]);
        if (NULL == b->key) continue;

        while (b) {
            if (GET_TAG(b->item) == ALIAS) {
                Toy_Type *v;
                v = hash_get_t(b->item->u.alias.slot, b->item->u.alias.key);
                if (NULL == v) {
                    p = new_cons(new_string_str(b->key), const_Nil);
                } else {
                    p = new_cons(new_string_str(b->key), v);
                }
            } else {
                p = new_cons(new_string_str(b->key), b->item);
            }
            sl = list_append(sl, p);
            b = b->next;
        }
    }

    return l;
}

int
hash_link(Hash *hash, const wchar_t *key,
	  Hash *to_hash, const wchar_t *to_key)
{
    Toy_Type *o;
    Toy_Type *e;

    if (NULL == hash) return 0;
    if (1 == hash_is_exists(hash, key)) return 0;

    e = hash_get_alias(to_hash, to_key);
    if ((NULL != e) && (GET_TAG(e) == ALIAS)) return 0;

    o = new_alias(to_hash, new_symbol((wchar_t*)to_key));

    if (NULL == hash_set(hash, key, o)) {
	return 0;
    } else {
	return 1;
    }
}

int
hash_link_t(Hash *hash, const struct _toy_type *key,
	    Hash *to_hash, const struct _toy_type *to_key)
{
    if (GET_TAG(key) != SYMBOL) return 0;
    if (GET_TAG(to_key) != SYMBOL) return 0;

    return hash_link(hash, cell_get_addr(key->u.symbol.cell),
		     to_hash, cell_get_addr(to_key->u.symbol.cell));
}

void
hash_debug_dump(Hash *hash) {
    int i;
    struct hash_bucket *b;

    if (NULL == hash) {
	printf("hash not created.\n");
	return;
    }

    printf("\n");
    printf("hash items: %d\n", hash->items);
    printf("hash bucket size: %d\n", hash->bucket_size);
    printf("hash synonyms: %d\n", hash->synonyms);

    for (i=0; i<(hash->bucket_size); i++) {
	if (NULL == hash->bucket[i].key) {
	    wprintf(L"bucket[%d] is not used\n", i);
	} else {
	    wprintf(L"bucket[%d] dump:\n", i);
	    b = &(hash->bucket[i]);
	    while (b) {
		wprintf(L"  key: \"%ls\"\n", b->key);
		b = b->next;
	    }
	}
    }

    return;
}

/*
 * this function is part of the Tcl lang suite
 */
unsigned int
hash_string_key(const wchar_t *key) {
    register const wchar_t *string = key;
    register unsigned int result;
    register int c;

    /*
     * I tried a zillion different hash functions and asked many other people
     * for advice. Many people had their own favorite functions, all
     * different, but no-one had much idea why they were good ones. I chose
     * the one below (multiply by 9 and add new character) because of the
     * following reasons:
     *
     * 1. Multiplying by 10 is perfect for keys that are decimal strings, and
     *	  multiplying by 9 is just about as good.
     * 2. Times-9 is (shift-left-3) plus (old). This means that each
     *	  character's bits hang around in the low-order bits of the hash value
     *	  for ever, plus they spread fairly rapidly up to the high-order bits
     *	  to fill out the hash value. This seems works well both for decimal
     *	  and non-decimal strings, but isn't strong against maliciously-chosen
     *	  keys.
     */

    result = 0;

    for (c=*string++ ; c ; c=*string++) {
	result += (result<<3) + c;
    }
    return result;
}
