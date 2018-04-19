/* $Id: hash.h,v 1.7 2009/08/27 12:21:03 mit-sato Exp $ */

#ifndef __HASH__
#define __HASH__

#include <wchar.h>
#include <t_gc.h>
#include "types.h"

#define HASH_INIT_BUCKET	(13)


struct hash_bucket {
    wchar_t *key;
    unsigned int index;
    struct _toy_type *item;
    struct hash_bucket *next;
};

typedef struct _hash {
    int bucket_size;
    struct hash_bucket **bucket;
    int items;
    int synonyms;
} Hash;

unsigned int		hash_string_key(const wchar_t *key);
Hash*			new_hash();
void			hash_clear(Hash* h);
Hash*			hash_set(Hash *hash, const wchar_t *key, const struct _toy_type *item);
Hash*			hash_set_t(Hash *hash, const struct _toy_type *key, const struct _toy_type *item);
struct _toy_type*	hash_get(Hash *hash, const wchar_t *key);
struct _toy_type*	hash_get_t(Hash *hash, const struct _toy_type *key);
struct _toy_type*	hash_get_and_unset(Hash *hash, const wchar_t *key);
struct _toy_type*	hash_get_and_unset_t(Hash *hash, const struct _toy_type *key);
Hash*			hash_unset(Hash *hash, const wchar_t *key);
Hash*			hash_unset_t(Hash *hash, const struct _toy_type *key);
int			hash_is_exists(Hash *hash, const wchar_t *key);
int			hash_is_exists_t(Hash *hash, const struct _toy_type *key);
int			hash_link(Hash *hash, const wchar_t *key,
				  Hash *to_hash, const wchar_t *to_key);
int			hash_link_t(Hash *hash, const struct _toy_type *key,
				    Hash *to_hash, const struct _toy_type *to_key);
struct _toy_type*	hash_get_keys(Hash *hash);
struct _toy_type*	hash_get_keys_str(Hash *hash);
struct _toy_type*	hash_get_pairs(Hash *hash);
struct _toy_type*	hash_get_pairs_str(Hash *hash);
void			hash_debug_dump(Hash *hash);

/* for search object method cache */
struct _toy_type*	hash_get_method_cache(Hash* hash,
					      const struct _toy_type *object,
					      const struct _toy_type *key);
Hash*			hash_set_method_cache(Hash* hash,
					      const struct _toy_type *object,
					      const struct _toy_type *key,
					      struct _toy_type *method);

#define hash_get_length(x)	((x==NULL)?0:x->items)
#define hash_get_synonyms(x)	((x==NULL)?0:x->synonyms)

#endif /* __HASH__ */
