/* $Id: types.c,v 1.41 2012/03/06 06:09:29 mit-sato Exp $ */

#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <setjmp.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "toy.h"
#include "interp.h"
#include "types.h"
#include "error.h"
#include "global.h"
#include "cstack.h"

Toy_Type*
new_nil() {
    Toy_Type *o;
    o = GC_MALLOC_ATOMIC(sizeof(Toy_Type));
    ALLOC_SAFE(o);

    o->tag = NIL;
    return o;
}

Toy_Type*
new_symbol(wchar_t *symbol) {
    Toy_Type *o;
    wchar_t *p;
    int l;

    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);

    o->tag = SYMBOL;
    o->u.symbol.cell = new_cell(symbol);
    o->u.symbol.hash_index = hash_string_key(symbol);

    p = cell_get_addr(o->u.symbol.cell);
    l = cell_get_length(o->u.symbol.cell);
    if (p[l-1] == L':') {
	o->tag |= TAG_NAMED_MASK;
    }
    if (p[0] == L':') {
	o->tag |= TAG_SWITCH_MASK;
    }

    return o;
}

Toy_Type*
new_ref(wchar_t *ref) {
    Toy_Type *o;
    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);

    o->tag = REF;
    o->u.ref.cell = new_cell(ref);
    o->u.ref.hash_index = hash_string_key(ref);
    return o;
}

Toy_Type*
new_integer(mpz_t integer) {
    Toy_Type *o;

    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);

    o->tag = INTEGER;
    mpz_init(o->u.biginteger);
    mpz_set(o->u.biginteger, integer);

    return o;
}

Toy_Type*
new_integer_si(long int integer) {
    mpz_t s;

    mpz_init(s);
    mpz_set_si(s, integer);
    return new_integer(s);
}

Toy_Type*
new_integer_ullsi(unsigned long long int integer) {
    mpz_t s;

    mpz_init(s);
    mpz_set_si(s, (long int)(integer));
    return new_integer(s);
}

Toy_Type*
new_integer_d(double val) {
    mpz_t s;

    mpz_init(s);
    mpz_set_d(s, val);
    return new_integer(s);
}

wchar_t*
integer_to_str(Toy_Type *val) {
    char *cbuff;
    int size;

    if (GET_TAG(val) != INTEGER) return NULL;
    size = mpz_sizeinbase(val->u.biginteger, 10);
    if (mpz_cmp_si(val->u.biginteger, 0) < 0) {
	size ++;
    }
    cbuff = GC_MALLOC(size + 1);
    ALLOC_SAFE(cbuff);
    mpz_get_str(cbuff, 10, val->u.biginteger);
    cbuff[size] = 0;

    return to_wchar(cbuff);
}

Toy_Type*
new_real(double real) {
    Toy_Type *o;
    o = GC_MALLOC_ATOMIC(sizeof(Toy_Type));
    ALLOC_SAFE(o);

    o->tag = REAL;
    o->u.real = real;
    return o;
}

Toy_Type*
new_string_str(wchar_t *string) {
    Toy_Type *o;
    Cell *c;
    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);
    c = new_cell(string);
    if (NULL == c) return NULL;

    o->tag = STRING;
    o->u.string = c;
    return o;
}

Toy_Type*
new_string_cell(Cell *string) {
    Toy_Type *o;
    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);

    o->tag = STRING;
    o->u.string = string;
    return o;
}

Toy_Type*
new_script(Toy_Type *statement_list) {
    Toy_Type *o;
    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);

    o->tag = SCRIPT;
    o->u.statement_list = statement_list;
    return o;
}

Toy_Type*
new_statement(Toy_Type *item_list, int line) {
    Toy_Type *o;
    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);

    o->tag = STATEMENT;
    o->u.statement.item_list = item_list;
    o->u.statement.trace_info = GC_MALLOC(sizeof(struct _toy_func_trace_info));
    ALLOC_SAFE(o->u.statement.trace_info);
    o->u.statement.trace_info->line = line;
    o->u.statement.trace_info->object_name = NULL;
    o->u.statement.trace_info->func_name = NULL;
    o->u.statement.trace_info->statement = NULL;
    SET_PARAMNO(o, TAG_MAX_PARAMNO);
    return o;
}

Toy_Type*
new_block(Toy_Type *block_body) {
    Toy_Type *o;
    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);

    o->tag = BLOCK;
    o->u.block_body = block_body;
    return o;
}

Toy_Type*
new_eval(Toy_Type *eval_body) {
    Toy_Type *o;
    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);

    o->tag = EVAL;
    o->u.eval_body = eval_body;
    return o;
}

Toy_Type*
new_native(Toy_Type* (*cfunc)(Toy_Interp*, Toy_Type*, struct _hash*, int arglen),
	   Toy_Type *argspec_list) {

    Toy_Type *o;
    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);

    o->tag = NATIVE;
    o->u.native.cfunc = cfunc;
    o->u.native.argspec_list = argspec_list;
    return o;
}

Toy_Type*
new_object(wchar_t *name, struct _hash* slots, Toy_Type *delegate_list) {
    Toy_Type *o;
    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);

    o->tag = OBJECT;
    o->u.object.slots = slots;
    o->u.object.delegate_list = delegate_list;
    hash_set_t(slots, const_atname, new_symbol(name));
    return o;
}

Toy_Type*
new_exception(wchar_t *code, wchar_t *desc, Toy_Interp *interp) {
    Toy_Type *o;

    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);

    o->tag = EXCEPTION;
    o->u.exception.code = new_cell(code);
    o->u.exception.msg_list = new_list(new_string_str(desc));

    if (interp) {
	Cell *stack;

	stack = new_cell(to_string(o));
	cell_add_str(stack, L"\n");
	get_stack_trace(interp, stack);

	list_append(o->u.exception.msg_list, new_string_cell(stack));

    } else {

	list_append(o->u.exception.msg_list, new_string_str(L""));
    }

    return o;
}

Toy_Type*
new_closure(Toy_Type *block_body, Toy_Env* env, int script_id) {
    Toy_Type *o;
    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);

    o->tag = CLOSURE;
    SET_SCRIPT_ID(block_body, script_id);
    o->u.closure.block_body = block_body;
    o->u.closure.env = env;
    return o;
}

Toy_Type*
new_func(Toy_Type *argspec_list, int posarglen, Array *posargarray, Hash *namedarg, Toy_Type *closure) {
    struct _toy_argspec *argspec;

    argspec = GC_MALLOC(sizeof(Toy_Argspec));
    ALLOC_SAFE(argspec);

    argspec->list = argspec_list;
    argspec->posarg_len = posarglen;
    argspec->posarg_array = posargarray;
    argspec->namedarg = namedarg;

    Toy_Type *o;
    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);

    o->tag = FUNC;
    o->u.func.argspec = argspec;
    o->u.func.closure = closure;

    if (((argspec->posarg_len >= 0) && (argspec->posarg_len < TAG_MAX_PARAMNO))
	&& (hash_get_length(namedarg) == 0)) {
	CLEAR_PARAMNO(o);
	SET_PARAMNO(o, argspec->posarg_len);
    } else {
	SET_PARAMNO(o, TAG_MAX_PARAMNO);
    }

    return o;
}

Toy_Type*
new_control(int code, Toy_Type *ret_value) {
    Toy_Type *o;
    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);

    o->tag = CONTROL;
    o->u.control.code = code;
    o->u.control.ret_value = ret_value;
    return o;
}

Toy_Type*
new_container(void *container) {
    Toy_Type *o;
    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);

    o->tag = CONTAINER;
    o->u.container = container;
    return o;
}

Toy_Type*
new_getmacro(Toy_Type *obj, Toy_Type *para) {
    Toy_Type *o;
    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);

    o->tag = GETMACRO;
    o->u.getmacro.obj = obj;
    o->u.getmacro.para = para;
    return o;
}

Toy_Type*
new_initmacro(Toy_Type *class, Toy_Type *param) {
    Toy_Type *o;
    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);

    o->tag = INITMACRO;
    o->u.initmacro.class = class;
    o->u.initmacro.param = param;
    return o;
}

Toy_Type*
new_alias(struct _hash *slot, Toy_Type *key) {
    Toy_Type *o;
    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);

    o->tag = ALIAS;
    o->u.alias.slot = slot;
    o->u.alias.key = key;
    return o;
}

Toy_Type*
new_rquote(wchar_t *string) {
    Toy_Type *o;
    Cell *c;
    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);
    c = new_cell(string);
    if (NULL == c) return NULL;

    o->tag = RQUOTE;
    o->u.string = c;
    return o;
}

Toy_Type*
new_bind(Toy_Type *bind_var) {
    Toy_Type *o;
    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);
    
    o->tag = BIND;
    o->u.bind_var = bind_var;
    return o;
}

Toy_Type*
new_dict(struct _hash *dict) {
    Toy_Type *o;
    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);
    
    o->tag = DICT;
    SET_NOPRINTABLE(o);
    o->u.dict = dict;
    return o;
}

Toy_Type*
new_vector(struct _array *vector) {
    Toy_Type *o;
    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);
    
    o->tag = VECTOR;
    SET_NOPRINTABLE(o);
    o->u.vector = vector;
    return o;
}

static void
coroutine_handl(void *context) {
    Toy_Type *co;
    Toy_Type *result = const_Nil;
    sigjmp_buf jmp_env;
    int id;

    co = (Toy_Type*)context;

    if (0 == sigsetjmp(jmp_env, 1)) {
	cstack_set_jmpbuff(co->u.coroutine->interp->cstack_id, &jmp_env);
	
	result = toy_eval_script(co->u.coroutine->interp,
				 co->u.coroutine->script->u.closure.block_body);

	if (GET_TAG(result) == CONTROL) {
	    if ((result->u.control.code == CTRL_RETURN) || 
		(result->u.control.code == CTRL_BREAK)) {
		result = result->u.control.ret_value;
	    } else {
		result = const_Nil;
	    }
	}

    } else {
	/* +++ */
	fwprintf(stderr, L"coroutine_handl: detect SOVF\n");
	co->u.coroutine->interp->co_parent->co_value =
	    new_exception(TE_STACKOVERFLOW, L"C stack overflow.", co->u.coroutine->interp);
    }

    id = co->u.coroutine->interp->cstack_id;
    co->u.coroutine->interp->cstack_id = 0;
    co->u.coroutine->interp->co_parent->co_value = result;
    co->u.coroutine->state = CO_STS_DONE;
    cstack_release(id);
 
    co_exit();
}

void
coro_finalizer(void *obj, void *client_data) {
    Toy_Type *o;
    
    o = (Toy_Type*)obj;
    if (NULL == o) return;
    if (NULL == o->u.coroutine) return;

    if (0 != o->u.coroutine->coro_id) {
	co_delete(o->u.coroutine->coro_id);
	o->u.coroutine->coro_id = 0;
    }
    if (NULL != o->u.coroutine->interp) {
	cstack_release_clear(o->u.coroutine->interp->cstack_id);
	o->u.coroutine->interp = NULL;
    }

    return;
}

Toy_Type*
new_coroutine(Toy_Interp *interp, Toy_Type* script) {
    Toy_Type *o;
    int cstack_id;

    o = GC_MALLOC(sizeof(Toy_Type));
    ALLOC_SAFE(o);

    o->tag = COROUTINE;
    o->u.coroutine = GC_MALLOC(sizeof(Toy_Coroutine));
    ALLOC_SAFE(o->u.coroutine);

    o->u.coroutine->interp = new_interp(0, CO_STACKSIZE, interp, 0, 0, 0);
    ALLOC_SAFE(o->u.coroutine->interp);
    o->u.coroutine->script = script;

    if (GET_TAG(script) == CLOSURE) {
	cstack_id = cstack_get(to_string(script->u.closure.block_body));
    } else {
	cstack_id = cstack_get(L"");
    }
    
    if (-1 == cstack_id) {
	return new_exception(TE_NOSLOT, L"No stack slot space.", interp);
    }

    o->u.coroutine->interp->cstack_id = cstack_id;
    o->u.coroutine->interp->cstack = cstack_get_start_addr(cstack_id);
    o->u.coroutine->interp->cstack_size = cstack_get_size(cstack_id);
    o->u.coroutine->interp->co_parent = interp;
    o->u.coroutine->coro_id = co_create(coroutine_handl,
					(void*)o,
					o->u.coroutine->interp->cstack,
					o->u.coroutine->interp->cstack_size);
    o->u.coroutine->interp->coroid = o->u.coroutine->coro_id;
    o->u.coroutine->state = CO_STS_INIT;

    GC_register_finalizer_ignore_self((void*)o,
				      coro_finalizer,
				      (void*)interp,
				      NULL,
				      NULL);

    if (NULL == o->u.coroutine->coro_id) {
	return new_exception(TE_NOSLOT, L"Can\'t create co-routine.", interp);
    }

    return o;
}

Toy_Type*
toy_clone(Toy_Type *obj) {
    Toy_Type *dest;

    switch (GET_TAG(obj)) {
    case INTEGER:
	dest = GC_MALLOC(sizeof(Toy_Type));
	ALLOC_SAFE(dest);
	dest->tag = obj->tag;
	mpz_init(dest->u.biginteger);
	mpz_set(dest->u.biginteger, obj->u.biginteger);
	break;
    case STRING:
	dest = GC_MALLOC(sizeof(Toy_Type));
	ALLOC_SAFE(dest);
	dest->tag = obj->tag;
	dest->u.string = new_cell(cell_get_addr(obj->u.string));
	break;
    default:
	dest = obj;
    }
    return dest;
}

static wchar_t *toy_type_char[] = {
    L"NIL",
    L"SYMBOL",
    L"REF",
    L"LIST",
    L"INTEGER",
    L"REAL",
    L"STRING",
    L"SCRIPT",
    L"STATEMENT",
    L"BLOCK",
    L"EVAL",
    L"NATIVE",
    L"OBJECT",
    L"EXCEPTION",
    L"CLOSURE",
    L"FUNC",
    L"CONTROL",
    L"CONTAINER",
    L"GETMACRO",
    L"ALIAS",
    L"RQUOTE",
    L"INITMACRO",
    L"BIND",
    L"DICT",
    L"VECTOR",
    L"COROUTINE",
};

wchar_t*
toy_get_type_str(Toy_Type *obj) {
    if (NULL == obj) return L"(null address)";
    if (IS_TOY_OBJECT(obj)) {
	return toy_type_char[GET_TAG(obj)];
    } else {
	return L"(unknown)";
    }
}

wchar_t*
to_string(Toy_Type *obj) {
    if (NULL == obj) return L"(null address)";
    if (! IS_TOY_OBJECT(obj)) return L"(unknown)";

    switch (GET_TAG(obj)) {
    case NIL:
	return S_NIL;

    case SYMBOL:
	return cell_get_addr(obj->u.symbol.cell);

    case REF:
    {
	Cell *c;
	c = new_cell(L"$");
	cell_add_str(c, cell_get_addr(obj->u.ref.cell));
	return cell_get_addr(c);
    }

    case INTEGER:
    {
	return integer_to_str(obj);
    }

    case REAL:
    {
	wchar_t *buff;
	Cell *c;

	buff = GC_MALLOC(32*sizeof(wchar_t));
	ALLOC_SAFE(buff);
	if ((fabs(obj->u.real) > 1.0E+9) || (fabs(obj->u.real) < 1.0E-2)) {
	    if (obj->u.real == 0.0) {
		swprintf(buff, 32, L"%-.15f", obj->u.real);
	    } else {
		swprintf(buff, 32, L"%.15E", obj->u.real);
	    }
	} else {
	    if (fabs(obj->u.real) < 10.0) {
		swprintf(buff, 32, L"%-.15f", obj->u.real);
	    } else if (fabs(obj->u.real) < 100.0) {
		swprintf(buff, 32, L"%-.14f", obj->u.real);
	    } else if (fabs(obj->u.real) < 1000.0) {
		swprintf(buff, 32, L"%-.13f", obj->u.real);
	    } else if (fabs(obj->u.real) < 10000.0) {
		swprintf(buff, 32, L"%-.12f", obj->u.real);
	    } else if (fabs(obj->u.real) < 100000.0) {
		swprintf(buff, 32, L"%-.11f", obj->u.real);
	    } else if (fabs(obj->u.real) < 1000000.0) {
		swprintf(buff, 32, L"%-.10f", obj->u.real);
	    } else if (fabs(obj->u.real) < 10000000.0) {
		swprintf(buff, 32, L"%-.9f", obj->u.real);
	    } else if (fabs(obj->u.real) < 100000000.0) {
		swprintf(buff, 32, L"%-.8f", obj->u.real);
	    } else if (fabs(obj->u.real) < 1000000000.0) {
		swprintf(buff, 32, L"%-.7f", obj->u.real);
	    } else {
		swprintf(buff, 32, L"%-.6f", obj->u.real);
	    }
	}
	c = new_cell(buff);
	return cell_get_addr(c);
    }

    case STRING:
    {
	return cell_get_addr(obj->u.string);
    }
	
    case SCRIPT:
    {
	Toy_Type *l;
	Cell *c;

	l = obj->u.statement_list;
	if (IS_LIST_NULL(l)) {
	    return L"";
	}

	c = new_cell(L"");
	do {
	    cell_add_str(c, to_print(list_get_item(l)));
	    l = list_next(l);
	} while (l);

	return cell_get_addr(c);
    }

    case STATEMENT:
    {
	Toy_Type *l;
	Cell *c;

	l = obj->u.statement.item_list;
	if (IS_LIST_NULL(l)) {
	    return L"";
	}

	c = new_cell(L"");
	do {
	    cell_add_str(c, to_print(list_get_item(l)));
	    if (list_length(l) > 1) cell_add_char(c, L' ');
	    l = list_next(l);
	} while (l);

	cell_add_char(c, L';');
	return cell_get_addr(c);
    }

    case LIST:
    {
	Cell *c;

	if (IS_LIST_NULL(obj)) {
	    return L"()";
	}

	c = new_cell(L"(");
	do {
	    if (GET_TAG(obj) != LIST) {
		cell_add_str(c, L". ");
		cell_add_str(c, to_print(obj));
		break;
	    } else {
		if (IS_LIST_NULL(obj)) {
		    cell_add_str(c, L". ()");
		} else {
		    cell_add_str(c, to_print(list_get_item(obj)));
		}
		if (obj->u.list.nextp) cell_add_char(c, L' ');
	    }

	    obj = list_next(obj);
	} while (obj);

	cell_add_str(c, L")");
	return cell_get_addr(c);
    }
	
    case BLOCK:
    {
	Toy_Type *s;
	Cell *c;

	s = obj->u.block_body;
	c = new_cell(L"{");
	cell_add_str(c, to_print(s));
	cell_add_str(c, L"}");
	return cell_get_addr(c);
    }

    case EVAL:
    {
	Toy_Type *s;
	Cell *c;

	s = obj->u.eval_body;
	c = new_cell(L"[");
	cell_add_str(c, to_print(s));
	cell_add_str(c, L"]");
	return cell_get_addr(c);
    }

    case NATIVE:
    {
	Cell *c;
	
	if (obj->u.native.argspec_list == NULL) {
	    return L"() <NATIVE>";
	}
	c = new_cell(NULL);
	cell_add_str(c, to_print(obj->u.native.argspec_list));
	cell_add_str(c, L" <NATIVE>");
	return cell_get_addr(c);
    }

    case EXCEPTION:
    {
	Cell *c;
	c = new_cell(L"<");
	cell_add_str(c, cell_get_addr(obj->u.exception.code));
	cell_add_str(c, L"># ");
	cell_add_str(c, to_string(list_get_item(obj->u.exception.msg_list)));
	return cell_get_addr(c);
    }

    case FUNC:
    {
	Toy_Type *l;
	Cell *c;

	c = new_cell(L"(");
	l = obj->u.func.argspec->list;
	while (! IS_LIST_NULL(l)) {
	    if (list_get_item(l) == NIL) break;
	    cell_add_str(c, to_print(list_get_item(l)));
	    if (list_length(l) > 1) cell_add_char(c, L' ');
	    l = list_next(l);
	}
	cell_add_str(c, L") ");
	cell_add_str(c, to_print(obj->u.func.closure));
	return cell_get_addr(c);
    }

    case CONTROL:
    {
	wchar_t *p;
	Cell *c;
	switch (obj->u.control.code) {
	case CTRL_RETURN:
	    p = L"<CONTROL-return># ";
	    break;
	case CTRL_CONTINUE:
	    p = L"<CONTROL-continue># ";
	    break;
	case CTRL_BREAK:
	    p = L"<CONTROL-break># ";
	    break;
	case CTRL_REDO:
	    p = L"<CONTROL-redo># ";
	    break;
	case CTRL_RETRY:
	    p = L"<CONTROL-retry># ";
	    break;
	case CTRL_GOTO:
	    p = L"<CONTROL-goto># ";
	    break;
	default:
	    p = L"<CONTROL-unknown># ";
	}
	c = new_cell(p);
	cell_add_str(c, to_string(obj->u.control.ret_value));
	return cell_get_addr(c);
    }

    case OBJECT:
    {
	Cell *c;
	Hash *h;
	Toy_Type *name;

	h = obj->u.object.slots;
	c = new_cell(L"(object)");
	name = hash_get_t(h, const_atname);
	if (name) {
	    cell_add_str(c, to_string(name));
	}
	return cell_get_addr(c);
    }

    case CLOSURE:
    {
	Toy_Type *s;
	Cell *c;

	s = obj->u.closure.block_body;
	c = new_cell(L"{");
	cell_add_str(c, to_print(s));
	cell_add_str(c, L"}");
	return cell_get_addr(c);
    }

    case CONTAINER:
	return L"<CONTAINER>";

    case GETMACRO:
    {
	Cell *c;

	c = new_cell(L"");
	cell_add_str(c, to_string(obj->u.getmacro.obj));
	cell_add_str(c, L",");
	cell_add_str(c, to_string(obj->u.getmacro.para));
	return cell_get_addr(c);
    }

    case RQUOTE:
    {
	return cell_get_addr(obj->u.rquote);
    }

    case INITMACRO:
    {
	Cell *c;

	c = new_cell(L"`");
	cell_add_str(c, to_print(obj->u.initmacro.class));
	if (to_print(obj->u.initmacro.param)) {
	    cell_add_str(c, L" ");
	    cell_add_str(c, to_print(obj->u.initmacro.param));
	}
	return cell_get_addr(c);
    }

    case BIND:
    {
	Cell *c;
	Toy_Type *l;
	l = obj->u.bind_var;
	c = new_cell(L"| ");
	while (! IS_LIST_NULL(l)) {
	    if (list_get_item(l) == NIL) break;
	    cell_add_str(c, to_print(list_get_item(l)));
	    if (list_length(l) > 1) cell_add_char(c, L' ');
	    l = list_next(l);
	}
	cell_add_str(c, L" |");
	return cell_get_addr(c);
    }

    case DICT:
	return L"<DICT>";

    case VECTOR:
	return L"<VECTOR>";

    case COROUTINE:
	return L"<COROUTINE>";

    default:
	return L"(unknown)";
    }
}

wchar_t*
to_print(Toy_Type *obj) {
    if (NULL == obj) return L"(null address)";
    if (! IS_TOY_OBJECT(obj)) return L"(unknown)";

    switch (GET_TAG(obj)) {
    case NIL:
	return S_NIL;

    case SYMBOL:
	return cell_get_addr(obj->u.symbol.cell);

    case REF:
    {
	Cell *c;
	c = new_cell(L"$");
	cell_add_str(c, cell_get_addr(obj->u.ref.cell));
	return cell_get_addr(c);
    }

    case INTEGER:
    {
	return integer_to_str(obj);
    }

    case REAL:
    {
	wchar_t *buff;
	Cell *c;

	buff = GC_MALLOC(32*sizeof(wchar_t));
	ALLOC_SAFE(buff);
	if ((fabs(obj->u.real) > 1.0E+9) || (fabs(obj->u.real) < 1.0E-2)) {
	    if (obj->u.real == 0.0) {
		swprintf(buff, 32, L"%-.15f", obj->u.real);
	    } else {
		swprintf(buff, 32, L"%.15E", obj->u.real);
	    }
	} else {
	    if (fabs(obj->u.real) < 10.0) {
		swprintf(buff, 32, L"%-.15f", obj->u.real);
	    } else if (fabs(obj->u.real) < 100.0) {
		swprintf(buff, 32, L"%-.14f", obj->u.real);
	    } else if (fabs(obj->u.real) < 1000.0) {
		swprintf(buff, 32, L"%-.13f", obj->u.real);
	    } else if (fabs(obj->u.real) < 10000.0) {
		swprintf(buff, 32, L"%-.12f", obj->u.real);
	    } else if (fabs(obj->u.real) < 100000.0) {
		swprintf(buff, 32, L"%-.11f", obj->u.real);
	    } else if (fabs(obj->u.real) < 1000000.0) {
		swprintf(buff, 32, L"%-.10f", obj->u.real);
	    } else if (fabs(obj->u.real) < 10000000.0) {
		swprintf(buff, 32, L"%-.9f", obj->u.real);
	    } else if (fabs(obj->u.real) < 100000000.0) {
		swprintf(buff, 32, L"%-.8f", obj->u.real);
	    } else if (fabs(obj->u.real) < 1000000000.0) {
		swprintf(buff, 32, L"%-.7f", obj->u.real);
	    } else {
		swprintf(buff, 32, L"%-.6f", obj->u.real);
	    }
	}
	c = new_cell(buff);
	return cell_get_addr(c);
    }

    case STRING:
    {
	Cell *c;
	wchar_t *p;
	c = new_cell(L"\"");
	p = cell_get_addr(obj->u.string);
	while (*p) {
	    switch (*p) {
	    case L'\t':
		cell_add_str(c, L"\\t");
		break;
	    case L'\r':
		cell_add_str(c, L"\\r");
		break;
	    case L'\n':
		cell_add_str(c, L"\\n");
		break;
		    
	    default:
		cell_add_char(c, *p);
	    }
	    p++;
	}
	cell_add_char(c, L'"');
	return cell_get_addr(c);
    }
	
    case SCRIPT:
    {
	Toy_Type *l;
	Cell *c;

	l = obj->u.statement_list;
	if (IS_LIST_NULL(l)) {
	    return L"";
	}

	c = new_cell(L"");
	do {
	    cell_add_str(c, to_print(list_get_item(l)));
	    l = list_next(l);
	} while (l);

	return cell_get_addr(c);
    }

    case STATEMENT:
    {
	Toy_Type *l;
	Cell *c;

	l = obj->u.statement.item_list;
	if (IS_LIST_NULL(l)) {
	    return L"";
	}

	c = new_cell(L"");
	do {
	    cell_add_str(c, to_print(list_get_item(l)));
	    if (list_length(l) > 1) cell_add_char(c, L' ');
	    l = list_next(l);
	} while (l);

	cell_add_char(c, L';');
	return cell_get_addr(c);
    }

    case LIST:
    {
	Cell *c;

	if (IS_LIST_NULL(obj)) {
	    return L"()";
	}

	c = new_cell(L"(");
	do {
	    if (GET_TAG(obj) != LIST) {
		cell_add_str(c, L". ");
		cell_add_str(c, to_print(obj));
		break;
	    } else {
		if (IS_LIST_NULL(obj)) {
		    cell_add_str(c, L". ()");
		} else {
		    cell_add_str(c, to_print(list_get_item(obj)));
		}
		if (obj->u.list.nextp) cell_add_char(c, L' ');
	    }

	    obj = list_next(obj);
	} while (obj);

	cell_add_str(c, L")");
	return cell_get_addr(c);
    }
	
    case BLOCK:
    {
	Toy_Type *s;
	Cell *c;

	s = obj->u.block_body;
	c = new_cell(L"{");
	cell_add_str(c, to_print(s));
	cell_add_str(c, L"}");
	return cell_get_addr(c);
    }

    case EVAL:
    {
	Toy_Type *s;
	Cell *c;

	s = obj->u.eval_body;
	c = new_cell(L"[");
	cell_add_str(c, to_print(s));
	cell_add_str(c, L"]");
	return cell_get_addr(c);
    }

    case NATIVE:
    {
	Cell *c;
	
	if (obj->u.native.argspec_list == NULL) {
	    return L"() <NATIVE>";
	}
	c = new_cell(NULL);
	cell_add_str(c, to_print(obj->u.native.argspec_list));
	cell_add_str(c, L" <NATIVE>");
	return cell_get_addr(c);
    }

    case EXCEPTION:
    {
	Cell *c;
	c = new_cell(L"<");
	cell_add_str(c, cell_get_addr(obj->u.exception.code));
	cell_add_str(c, L"># ");
	cell_add_str(c, to_print(list_get_item(obj->u.exception.msg_list)));
	return cell_get_addr(c);
    }

    case FUNC:
    {
	Toy_Type *l;
	Cell *c;

	c = new_cell(L"(");
	l = obj->u.func.argspec->list;
	while (! IS_LIST_NULL(l)) {
	    if (list_get_item(l) == NIL) break;
	    cell_add_str(c, to_print(list_get_item(l)));
	    if (list_length(l) > 1) cell_add_char(c, L' ');
	    l = list_next(l);
	}
	cell_add_str(c, L") ");
	cell_add_str(c, to_print(obj->u.func.closure));
	return cell_get_addr(c);
    }

    case CONTROL:
    {
	wchar_t *p;
	Cell *c;
	switch (obj->u.control.code) {
	case CTRL_RETURN:
	    p = L"<CONTROL-return># ";
	    break;
	case CTRL_CONTINUE:
	    p = L"<CONTROL-continue># ";
	    break;
	case CTRL_BREAK:
	    p = L"<CONTROL-break># ";
	    break;
	case CTRL_REDO:
	    p = L"<CONTROL-redo># ";
	    break;
	case CTRL_RETRY:
	    p = L"<CONTROL-retry># ";
	    break;
	case CTRL_GOTO:
	    p = L"<CONTROL-goto># ";
	    break;
	default:
	    p = L"<CONTROL-unknown># ";
	}
	c = new_cell(p);
	cell_add_str(c, to_print(obj->u.control.ret_value));
	return cell_get_addr(c);
    }

    case OBJECT:
    {
	Cell *c;
	Hash *h;
	Toy_Type *name;

	h = obj->u.object.slots;
	c = new_cell(L"(object)");
	name = hash_get_t(h, const_atname);
	if (name) {
	    cell_add_str(c, to_print(name));
	}
	return cell_get_addr(c);
    }

    case CLOSURE:
    {
	Toy_Type *s;
	Cell *c;

	s = obj->u.closure.block_body;
	c = new_cell(L"{");
	cell_add_str(c, to_print(s));
	cell_add_str(c, L"}");
	return cell_get_addr(c);
    }

    case CONTAINER:
	return L"<CONTAINER>";

    case GETMACRO:
    {
	Cell *c;

	c = new_cell(L"");
	cell_add_str(c, to_print(obj->u.getmacro.obj));
	cell_add_str(c, L",");
	cell_add_str(c, to_print(obj->u.getmacro.para));
	return cell_get_addr(c);
    }

    case RQUOTE:
    {
	Cell *c;
	wchar_t *p;
	c = new_cell(L"\'");
	p = cell_get_addr(obj->u.rquote);
	while (*p) {
	    switch (*p) {
	    case L'\\':
		cell_add_str(c, L"\\\\");
		break;
	    case L'\'':
		cell_add_str(c, L"\\\'");
		break;
	    default:
		cell_add_char(c, *p);
	    }
	    p++;
	}
	cell_add_char(c, L'\'');
	return cell_get_addr(c);
    }

    case INITMACRO:
    {
	Cell *c;

	c = new_cell(L"`");
	cell_add_str(c, to_print(obj->u.initmacro.class));
	if (obj->u.initmacro.param) {
	    cell_add_str(c, L" ");
	    cell_add_str(c, to_print(obj->u.initmacro.param));
	}
	return cell_get_addr(c);
    }

    case BIND:
	return to_string(obj);

    case DICT:
	return L"<DICT>";

    case VECTOR:
	return L"<VECTOR>";

    case COROUTINE:
	return L"<COROUTINE>";

    default:
	return L"(unknown)";
    }
}
