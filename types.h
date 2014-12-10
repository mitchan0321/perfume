/* $Id: types.h,v 1.39 2012/03/06 06:09:30 mit-sato Exp $ */

#ifndef __TYPES__
#define __TYPES__

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <sys/types.h>
#include <pcl.h>
#include <gmp.h>
#include "cell.h"
#include "hash.h"
#include "array.h"
#include "bulk.h"
#include "t_gc.h"

#define    NIL			0	/* nil */
#define    SYMBOL  		1	/* symbol */
#define	   REF			2	/* reference (e.g. $XX) */
#define    LIST	  		3	/* list (e.g. (...)) */
#define    INTEGER  		4	/* 64bit signed integer */
#define    REAL	  		5	/* 64bit real */
#define    STRING		6	/* string (e.g. "...") */
#define    SCRIPT		7	/* script is some statements */
#define    STATEMENT		8	/* statement (e.g. ...;) */
#define    BLOCK		9	/* block (e.g. {...} */
#define    EVAL	  		10	/* eval (e.g. [...] */
#define    NATIVE		11	/* C native code (e.g. <NATIVE>) */
#define    OBJECT		12	/* a object */
#define    EXCEPTION		13	/* throwable object */
#define    CLOSURE		14	/* closure */
#define    FUNC	  		15	/* function object */
#define	   CONTROL		16	/* control object (e.g. return, break, or continue) */
#define    CONTAINER		17	/* C object container */
#define    GETMACRO		18	/* GET Macro */
#define    ALIAS		19	/* variable alias */
#define    RQUOTE		20	/* reguler expresion string (e.g. '...') */
#define	   INITMACRO		21	/* INIT Macro */
#define	   BIND			22	/* bind var list (e.g. | ... |) */
#define	   DICT			23	/* dictionary (as primitive hash) */
#define	   VECTOR		24	/* vector (as primitive array) */
#define	   COROUTINE		25	/* coroutine */
#define    __TYPE_LAST__	26


#define S_NIL	"nil"

#define IS_LIST_NULL(l)	((l==NULL)?1:((NULL==l->u.list.item)&&(NULL==l->u.list.nextp)))

#define TAG_MASK		(0x000000ff)
#define GET_TAG(p)		((p==NULL)?-1:(TAG_MASK & (p->tag)))

#define TAG_NAMED_MASK		(0x00008000)
#define TAG_SWITCH_MASK		(0x00004000)
#define IS_NAMED_SYM(p)		((p==NULL)?0:(TAG_NAMED_MASK & (p->tag)))
#define IS_SWITCH_SYM(p)	((p==NULL)?0:(TAG_SWITCH_MASK & (p->tag)))

#define SET_SCRIPT_ID(p,i)	(p->tag|=((i<<16)&0xffff0000))
#define GET_SCRIPT_ID(p)	(int)((p->tag&0xffff0000)>>16)

#define TAG_LAZY_MASK		(0x00002000)
#define SET_LAZY(p)		(p->tag|=TAG_LAZY_MASK)
#define CLEAR_LAZY(p)		(p->tag&=(~TAG_LAZY_MASK))
#define IS_LAZY(p)		(p->tag&TAG_LAZY_MASK)

#define TAG_NOPRINTABLE		(0x00000200)
#define SET_NOPRINTABLE(p)	(p->tag|=TAG_NOPRINTABLE)
#define IS_NOPRINTABLE(p)	(p->tag&TAG_NOPRINTABLE)

#define TAG_PARAMNO_MASK	(0x00001C00)
#define TAG_MAX_PARAMNO		(7)
#define SET_PARAMNO(p,i)	(p->tag|=((i<<10)&TAG_PARAMNO_MASK))
#define GET_PARAMNO(p)		(int)((p->tag&TAG_PARAMNO_MASK)>>10)
#define CLEAR_PARAMNO(p)	(p->tag&=(~TAG_PARAMNO_MASK))

#define IS_TOY_OBJECT(p)	((GET_TAG(p)>=NIL)&&(GET_TAG(p)<__TYPE_LAST__))
#define SELF_HASH(i)		(i->obj_stack[i->cur_obj_stack]->cur_object_slot)
#define SELF(i)			(i->obj_stack[i->cur_obj_stack]->self)
#define SELF_OBJ(i)		(i->obj_stack[i->cur_obj_stack]->cur_object)
#define GET_FUNC_ENV(p,i)	((((p->cur_func_stack)-i)<0) ? NULL : p->func_stack[(p->cur_func_stack)-i])

typedef struct _toy_func_trace_info {
    int line;
    struct _toy_type *object_name;
    struct _toy_type *func_name;
    struct _toy_type *statement;
} Toy_Func_Trace_Info;

typedef struct _toy_func_env {
    Hash *localvar;
    struct _toy_func_env *upstack;
    struct _toy_type *tobe_bind_val;
    struct _toy_func_trace_info *trace_info;
    int script_id;
} Toy_Func_Env;

typedef struct _toy_obj_env {
    struct _hash *cur_object_slot;
    struct _toy_type *cur_object;
    struct _toy_type *self;
} Toy_Obj_Env;

typedef struct _toy_env {
    Toy_Func_Env *func_env;
    Toy_Obj_Env *object_env;
    struct _toy_type *tobe_bind_val;
} Toy_Env;

struct _toy_argspec {
    struct _toy_type *list;
    int posarg_len;
    Array *posarg_array;
    Hash *namedarg;
} Toy_Argspec;

typedef struct _toy_interp {
    /* interpriter name */
    char *name;

    /* stack size */
    int stack_size;

    /* some stacks */
    int cur_func_stack;
    Toy_Func_Env **func_stack;

    int cur_obj_stack;
    Toy_Obj_Env **obj_stack;

    /* world */
    struct _hash *funcs;
    struct _hash *classes;
    struct _hash *globals;
    struct _hash *scripts;

    /* trace info */
    struct _toy_func_trace_info *trace_info;
    int script_id;

#ifdef HAS_GCACHE
    /* global cache */
    struct _hash *gcache;
    int cache_hit;
    int cache_missing;
#endif /* HAS_GCACHE */

    /* trace flag */
    int trace;
    int trace_fd;

    /* coroutine */
    int cstack_id;
    void *cstack;
    int cstack_size;
    coroutine_t coroid;
    struct _toy_interp *co_parent;
    struct _toy_type *co_value;
    int co_calling;

} Toy_Interp;

#if 0
typedef struct _callcc {
    Toy_Interp *interp;
    int *cstack;
    size_t cstack_size;
    struct _toy_type *value;
    jmp_buf jmpbuf;
} CallCC;
#endif

typedef struct _toy_coroutine {
    Toy_Interp *interp;
    struct _toy_type *script;
    int state;
    coroutine_t coro_id;
} Toy_Coroutine;

/*
 * TAG format:
 *       FEDCBA9876543210
 *      +----------------+
 *   +2 |   script id    |
 *      +----------------+
 *   +0 |@#$***! |  TYPE |
 *      +----------------+
 *
 *	@: named arg flag
 *	#: switch arg flag
 *	$: lazy flag
 *	***:
 *	   # of position parameters (0-6),
 *	   if less than 7 and only position parameters.
 *	!: no printable
 */
typedef struct _toy_type {
    u_int32_t tag;

    union _u {
	/* NIL */

	/* SYMBOL */
	struct _symbol {
	    Cell *cell;
	    unsigned int hash_index;
	} symbol;

	/* REF */
	struct _ref {
	    Cell *cell;
	    unsigned int hash_index;
	} ref;

	/* LIST */
	struct _list {
	    struct _toy_type *item;
	    struct _toy_type *nextp;
	} list;

	/* INTEGER */
	mpz_t biginteger;

	/* REAL */
	double real;

	/* STRING */
	Cell *string;

	/* SCRIPT */
	struct _toy_type *statement_list;
	
	/* STATEMENT */
	struct _statement {
	    struct _toy_type *item_list;
	    struct _toy_func_trace_info *trace_info;
	} statement;
	
	/* BLOCK */
	struct _toy_type *block_body;		/* point to _toy_type->script */

	/* EVAL */
	struct _toy_type *eval_body;		/* point to _toy_type->script */

	/* NATIVE */
	struct _native {
	    struct _toy_type* (*cfunc)(
		Toy_Interp* interp,
		struct _toy_type* posargs,	/* pint to _toy_type->list */
		struct _hash* namedargs,
		int arglen
		);
	    struct _toy_type *argspec_list;
	} native;

	/* OBJECT */
	struct _object {
	    struct _hash *slots;
	    struct _toy_type *delegate_list;
	} object;

	/* EXCEPTION */
	struct _exception {
	    Cell *code;
	    struct _toy_type *msg_list;
	} exception;

	/* CLOSURE */
	struct _closure {
	    struct _toy_type *block_body;	/* point to _toy_type->script */
	    Toy_Env *env;
	} closure;

	/* FUNC */
	struct _func {
	    struct _toy_argspec *argspec;
	    struct _toy_type *closure;
	} func;

	struct _control {
	    u_int32_t code;
	    struct _toy_type *ret_value;
	} control;

	/* CONTAINER */
	void *container;

	/* GET Macro */
	struct _getmacro {
	    struct _toy_type *obj;
	    struct _toy_type *para;
	} getmacro;

	/* INIT Macro */
	struct _initmacro {
	    struct _toy_type *class;
	    struct _toy_type *param;
	} initmacro;

	/* ALIAS */
	struct _alias {
	    struct _hash *slot;
	    struct _toy_type *key;
	} alias;

	/* RQUOTE */
	Cell *rquote;

#if 0
	/* CALL/CC */
	CallCC *callcc_buff;
#endif

	/* BIND */
	struct _toy_type *bind_var;

	/* DICT */
	struct _hash *dict;

	/* VECTOR */
	struct _array *vector;

	/* COROUTINE */
	Toy_Coroutine *coroutine;

    } u;
} Toy_Type;

typedef struct _toy_script {
    int id;
    Toy_Type *path;
    Bulk *src;
    Toy_Type *script;
} Toy_Script;


Toy_Type*	new_nil();
Toy_Type*	new_symbol(char *atom);
Toy_Type*	new_ref(char *ref);
Toy_Type*	new_list(Toy_Type *item);
Toy_Type*	new_cons(Toy_Type *car, Toy_Type *cdr);
Toy_Type*	list_append(Toy_Type *list, Toy_Type *item);
Toy_Type*	list_next(Toy_Type *list);
Toy_Type*	list_get_item(Toy_Type *list);
int		list_length(Toy_Type *list);
Toy_Type*	new_integer(mpz_t biginteger);
Toy_Type*	new_integer_si(int integer);
Toy_Type*	new_integer_d(double val);
char*		integer_to_str(Toy_Type *val);
Toy_Type*	new_real(double real);
Toy_Type*	new_string_str(char *string);
Toy_Type*	new_string_cell(Cell *string);
Toy_Type*	new_script(Toy_Type *statement_list);
Toy_Type*	new_statement(Toy_Type *item_list, int line);
Toy_Type*	new_block(Toy_Type *block_body);
Toy_Type*	new_eval(Toy_Type *eval_body);
Toy_Type*	new_native(Toy_Type* (*cfunc)(Toy_Interp*, Toy_Type*, struct _hash*, int arglen),
			   Toy_Type *argspec_list);
Toy_Type*	new_object(char* name, struct _hash* slots, Toy_Type *delegate_list);
Toy_Type*	new_exception(char *code, char *desc, Toy_Interp* interp);
Toy_Type*	new_closure(Toy_Type *block_body, Toy_Env* env, int script_id);
Toy_Type*	new_func(Toy_Type *argspec_list, int posarglen, Array *posargarray, Hash *namedarg,
			 Toy_Type *closure);
Toy_Type*	new_control(int code, Toy_Type *ret_value);
Toy_Type*	new_container(void *container);
Toy_Type*	new_getmacro(Toy_Type *obj, Toy_Type *para);
Toy_Type*	new_initmacro(Toy_Type *obj, Toy_Type *param);
Toy_Type*	new_alias(struct _hash *slot, Toy_Type *key);
Toy_Type*	new_rquote(char *string);
Toy_Type*	new_callcc();
Toy_Type*	new_bind(Toy_Type *bind_var);
Toy_Type*	new_dict(struct _hash *dict);
Toy_Type*	new_vector(struct _array *vector);
Toy_Type*	new_coroutine(Toy_Interp*, Toy_Type*);

Toy_Type*	toy_clone(Toy_Type *obj);
char*		toy_get_type_str(Toy_Type *obj);
char*		to_string(Toy_Type *obj);
char*		to_print(Toy_Type *obj);

#define	list_next(l)		(((l)==NULL)?NULL:((GET_TAG((l))==LIST)?(l)->u.list.nextp:NULL))
#define list_get_item(l)	(((l)==NULL)?NULL:((GET_TAG((l))==LIST)?(l)->u.list.item:NULL))
#define list_set_car(l,v)	(((l)==NULL)?NULL:((GET_TAG((l))==LIST)?(l)->u.list.item=v:NULL))
#define list_set_cdr(l,v)	(((l)==NULL)?NULL:((GET_TAG((l))==LIST)?(l)->u.list.nextp=v:NULL))

#endif /* __TYPES__ */
