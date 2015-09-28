/* $Id: eval.c,v 1.57 2012/03/06 06:09:28 mit-sato Exp $ */

#include <toy.h>
#include <assert.h>
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "types.h"
#include "global.h"
#include "hash.h"
#include "cstack.h"

static Toy_Type* bind_args(Toy_Interp*, Toy_Type* arglist, struct _toy_argspec* argspec,
			   Toy_Env**, Hash* args, int fun_paramno, int call_paramno);
static Toy_Type* eval_sig_handl(Toy_Interp *interp, int code);
static Toy_Type* toy_yield_bind(Toy_Interp *interp, Toy_Type *bind_var);

/*
#define SET_RESULT(interp,obj) \
	hash_set_t(interp->func_stack[interp->cur_func_stack]->localvar,const_Question,obj)
*/

#define SIG_ACTION() 							\
	if (sig_flag && (! interp->co_calling)) {			\
		Toy_Type *sig_result;					\
		int code = sig_flag;					\
		sig_flag = 0;						\
		sig_result = eval_sig_handl(interp, code);		\
		if (sig_result && (GET_TAG(sig_result) != NIL)) {	\
			result = sig_result;				\
		}							\
	}

Toy_Type*
toy_eval_script(Toy_Interp* interp, Toy_Type *script) {
    Toy_Type *l;
    Toy_Env *env = NULL;
    Toy_Type* result;
    int t;
    int script_id;
    extern volatile sig_atomic_t CStack_in_baria;
    result = const_Nil;

    SIG_ACTION();

    l = script->u.statement_list;
    if (IS_LIST_NULL(l)) return result;

    script_id = interp->script_id;
    if (GET_SCRIPT_ID(script) > 0) {
	interp->script_id = GET_SCRIPT_ID(script);
    }

    if (CStack_in_baria) {
	/* +++ */
	fprintf(stderr, "toy_eval_script: detect SOVF(1)\n");
	result = new_exception(TE_STACKOVERFLOW, "C stack overflow.", interp);
	interp->script_id = script_id;
	cstack_return();
	return result;
    }

    while (l) {
	result = toy_eval(interp, list_get_item(l), &env);
	interp->last_status = result;

	if (CStack_in_baria) {
	    /* +++ */
	    fprintf(stderr, "toy_eval_script: detect SOVF(2)\n");
	    result = new_exception(TE_STACKOVERFLOW, "C stack overflow.", interp);
	}

	SIG_ACTION();

	t = GET_TAG(result);
	if ((t == CONTROL) || (t == EXCEPTION)) {
	    if ((t == EXCEPTION) &&
		(strcmp(TE_NOFUNC, cell_get_addr(result->u.exception.code)) == 0)) {
		Toy_Type *body;

		if (NULL != hash_get_t(interp->funcs, const_unknown)) {

		    body = new_list(const_unknown);
		    list_set_cdr(body, list_get_item(l)->u.statement.item_list);

		    result = toy_eval(interp,
				      new_statement(body, interp->trace_info->line),
				      &env);
		    interp->last_status = result;
		}
	    }

	    interp->script_id = script_id;
	    if (CStack_in_baria) cstack_return();
	    return result;
	}

	l = list_next(l);
    }

    interp->script_id = script_id;
    return result;
}

Toy_Type*
toy_eval(Toy_Interp* interp, Toy_Type *statement, Toy_Env** env) {
    Toy_Type *l;
    Toy_Type *first, *tfirst, *first_org, *method, *self;
    Cell *msg;
    Toy_Obj_Env *obj_env;
    int ostack_use, lstack_use;
    Toy_Type *ret;
    Toy_Type *posargs, *posargsl;
    Hash *namedargs;
    int arglen;
    Toy_Type *arg, *name;
    Hash *local_var;
    int script_id;
    int paramno_hint;

    local_var = NULL;

control_goto:
    ret = NULL;
    obj_env = NULL;
    lstack_use = 0;
    ostack_use = 0;
    arglen = 0;
    script_id = -1;

    l = statement->u.statement.item_list;

    interp->trace_info = statement->u.statement.trace_info;
    interp->trace_info->statement = statement;
    paramno_hint = GET_PARAMNO(statement);

    if (interp->trace) {
	char *buff;
	int sts;

	buff = GC_MALLOC(512);
	ALLOC_SAFE(buff);
	snprintf(buff, 512, "%s:%d: %s\n",
		 get_script_path(interp, interp->script_id),
		 interp->trace_info->line,
		 to_string(statement));
	buff[511] = 0;
	sts = write(interp->trace_fd, buff, strlen(buff));
    }

    if (IS_LIST_NULL(l)) {
	ret = const_Nil;
	goto exit_eval;
    }

    first_org = first = list_get_item(l);
    if (GET_TAG(first) == REF) {
	first = toy_resolv_var(interp, first, 1);
	if (GET_TAG(first) == EXCEPTION) {
	    ret = first;
	    goto exit_eval;
	}
    } else if (GET_TAG(first) == LIST) {
	first = toy_expand(interp, first, env);
    }
    self = first;

    if (GET_TAG(first) != SYMBOL && GET_TAG(first) != LIST) {
	first = toy_expand(interp, first, env);
	if (GET_TAG(first) == EXCEPTION) {
	    ret = first;
	    goto exit_eval;
	}
	if (GET_TAG(first) == BIND) {
	    ret = toy_yield_bind(interp, first->u.bind_var);
	    goto exit_eval;
	}
	self = first;
    }

    switch (GET_TAG(first)) {
    case SYMBOL:
	interp->trace_info->func_name = first;
	tfirst = toy_resolv_function(interp, first);

	if (NULL == tfirst) {
	    Cell *msg;

	    msg = new_cell("No such function, '");
	    cell_add_str(msg, to_string(first));
	    cell_add_str(msg, "'.");

	    ret = new_exception(TE_NOFUNC, cell_get_addr(msg), interp);
	    goto exit_eval;
	}
	first = tfirst;
	break;

    case OBJECT: case FUNC: case NATIVE:
	break;

    default:
	first = toy_resolv_object(interp, first);
	if (GET_TAG(first) == EXCEPTION) {
	    ret = first;
	    goto exit_eval;
	}
    }

    if (GET_TAG(first) == OBJECT) {

	l = list_next(l);
	method = list_get_item(l);
	if (method == NULL) {
	    msg = new_cell("No specified method");
	    ret = new_exception(TE_NOMETHOD, cell_get_addr(msg), interp);
	    goto exit_eval;
	}
	if (GET_TAG(method) != SYMBOL) {
	    method = toy_expand(interp, method, env);
	    if (GET_TAG(method) == EXCEPTION) {
		ret = first;
		goto exit_eval;
	    }
	}
	if (GET_TAG(method) != SYMBOL) {
	    msg = new_cell("Method is not a symbol, '");
	    cell_add_str(msg, to_string(method));
	    cell_add_str(msg, "'.");
	    ret = new_exception(TE_BADMETHOD, cell_get_addr(msg), interp);
	    goto exit_eval;
	}
	interp->trace_info->func_name = method;
	method = search_method(interp, first, method);
	if (GET_TAG(method) == EXCEPTION) {
	    ret = method;
	    goto exit_eval;
	}
	obj_env = toy_new_obj_env(interp, first, self);
	interp->trace_info->object_name = first;
	first = method;

	if (paramno_hint != TAG_MAX_PARAMNO) {
	    paramno_hint --;
	}
    }

    switch (GET_TAG(first)) {
    case NATIVE:
	l = list_next(l);
	posargsl = posargs = new_list(NULL);
	namedargs = new_hash();
	while (l) {
	    arg = list_get_item(l);

	    if (GET_TAG(arg) == SYMBOL) {
		if (IS_SWITCH_SYM(arg)) {
		    Cell *ckey;
		    ckey = new_cell(&(cell_get_addr(arg->u.symbol.cell)[1]));
		    cell_add_str(ckey, ":");
		    hash_set(namedargs, cell_get_addr(ckey), const_int1);
		} else if (IS_NAMED_SYM(arg)) {
		    name = arg;
		    l = list_next(l);
		    if (NULL == l) {
			msg = new_cell("No given named argument variable, name: '");
			cell_add_str(msg, cell_get_addr(name->u.symbol.cell));
			cell_add_str(msg, "'.");
			ret = new_exception(TE_NONAMEARG, cell_get_addr(msg), interp);
			goto exit_eval;
		    }
		    arg = toy_expand(interp, list_get_item(l), env);
		    if (GET_TAG(arg) == EXCEPTION) {
			ret = arg;
			goto exit_eval;
		    }
		    hash_set_t(namedargs, name, arg);
		} else {
		    posargs = list_append(posargs, arg);
		    arglen++;
		}
	    } else {
		arg = toy_expand(interp, arg, env);
		if (GET_TAG(arg) == EXCEPTION) {
		    ret = arg;
		    goto exit_eval;
		}
		posargs = list_append(posargs, arg);
		arglen++;
	    }

	    l = list_next(l);
	}
	if (NULL != obj_env) {
	    if (0 == toy_push_obj_env(interp, obj_env)) {
		ret = new_exception(TE_STACKOVERFLOW, "Object satck overflow.", interp);
		goto exit_eval;
	    }
	    ostack_use = 1;
	}
	ret = (first->u.native.cfunc)(interp, posargsl, namedargs, arglen);
	goto exit_eval;

    case FUNC:
	if (! local_var) {
	    local_var = new_hash();
	}

	ret = bind_args(interp, list_next(l), first->u.func.argspec, env, local_var,
			   GET_PARAMNO(first), paramno_hint);
	if (GET_TAG(ret) == EXCEPTION) {
	    goto exit_eval;
	}

	if (NULL != obj_env) {
	    if (0 == toy_push_obj_env(interp, obj_env)) {
		ret = new_exception(TE_STACKOVERFLOW, "Object satck overflow.", interp);
		goto exit_eval;
	    }
	    ostack_use = 1;
	}

	script_id = interp->script_id;
	interp->script_id = GET_SCRIPT_ID(first->u.func.closure->u.closure.block_body);
	if (0 == toy_push_func_env(interp, local_var,
				   first->u.func.closure->u.closure.env->func_env, NULL)) {

	    ret = new_exception(TE_STACKOVERFLOW, "Function satck overflow.", interp);
	    goto exit_eval;
	}
	lstack_use = 1;

	ret = toy_eval_script(interp, first->u.func.closure->u.closure.block_body);
	if ((GET_TAG(ret) == CONTROL) && (ret->u.control.code != CTRL_GOTO)) {
	    ret = ret->u.control.ret_value;
	}
	goto exit_eval;

    default:
	msg = new_cell("No runnable object, '");
	cell_add_str(msg, to_string(first));
	cell_add_str(msg, "'.");
	ret = new_exception(TE_NORUNNABLE, cell_get_addr(msg), interp);
    }

exit_eval:

    if (ostack_use) {
	toy_pop_obj_env(interp);
    }

    if (lstack_use) {
	toy_pop_func_env(interp);

	if ((GET_TAG(ret) == CONTROL) && (ret->u.control.code == CTRL_GOTO)) {
	    Toy_Type *ctrl_ret;

	    ctrl_ret = ret->u.control.ret_value;
	    local_var = list_get_item(ctrl_ret)->u.dict;

	    ctrl_ret = list_next(ctrl_ret);
	    statement = new_statement(list_get_item(ctrl_ret), interp->trace_info->line);

	    goto control_goto;
	}
    }

    /* SET_RESULT(interp, ret); */

    if (GET_TAG(ret) == EXCEPTION) {
	hash_set_t(interp->globals, const_atstacktrace,
		   list_get_item(list_next(ret->u.exception.msg_list)));
    }

    if (script_id != -1) interp->script_id = script_id;
    return ret;
}

Toy_Type*
toy_expand(Toy_Interp* interp, Toy_Type* obj, Toy_Env** env) {

    switch (GET_TAG(obj)) {
    case REF:
	return toy_resolv_var(interp, obj, 1);

    case LIST:
	return toy_expand_list(interp, obj, env);

    case BLOCK:
	if (*env == NULL) {
	    *env = new_closure_env(interp);
	}
	return new_closure(obj->u.block_body, *env, interp->script_id);

    case EVAL:
	return toy_eval_script(interp, obj->u.eval_body);

    case GETMACRO:
    {
	Toy_Type *st, *stl, *result;

	stl = st = new_list(obj->u.getmacro.obj);
	st = list_append(st, const_Get);
	st = list_append(st, obj->u.getmacro.para);

	result = toy_eval(interp, new_statement(stl, interp->trace_info->line), env);
	return result;
    }

    case INITMACRO:
    {
	Toy_Type *st, *stl, *result;

	stl = st = new_list(const_new);
	st = list_append(st, obj->u.initmacro.class);
	st = list_append(st, const_init);
	st = list_append(st, new_list(obj->u.initmacro.param));

	result = toy_eval(interp, new_statement(stl, interp->trace_info->line), env);
	return result;
    }

    default:
	return obj;
    }
}

Toy_Type*
toy_expand_list(Toy_Interp* interp, Toy_Type* list, Toy_Env** env) {
    Toy_Type *nlist, *l, *sl;
    Toy_Type *a;

    l = nlist = new_list(NULL);

    sl = list;
    while (sl) {
	
	if (GET_TAG(sl) != LIST) {
	    if (GET_TAG(sl) == SYMBOL) {
		a = sl;
	    } else {
		a = toy_expand(interp, sl, env);
	    }
	    list_set_cdr(nlist, a);
	    break;
	}

	a = toy_expand(interp, list_get_item(sl), env);
	if (GET_TAG(a) == EXCEPTION) return a;

	nlist = list_append(nlist, a);
	sl = list_next(sl);
    }
    
    return l;
}

Toy_Type*
toy_resolv_function(Toy_Interp* interp, Toy_Type* obj) {
    Hash *h;
    Toy_Type *val;

    h = interp->funcs;
    val = hash_get_t(h, obj);
    if (NULL != val) return val;

    h = interp->classes;
    val = hash_get_t(h, obj);
    if (NULL != val) return val;

    val = search_method(interp, interp->obj_stack[interp->cur_obj_stack]->cur_object, obj);
    if (GET_TAG(val) == EXCEPTION) return NULL;

    return val;
}

Toy_Type*
toy_resolv_var(Toy_Interp* interp, Toy_Type* var, int stack_trace) {
    Hash *h;
    Toy_Type *val;
    Cell *c;
    Toy_Func_Env *upenv;

    h = interp->func_stack[interp->cur_func_stack]->localvar;
    val = hash_get_t(h, var);
    if (NULL != val) {
	if (IS_LAZY(val) && (GET_TAG(val) == CLOSURE)) {
	    val = eval_closure(interp, val);
	    hash_set_t(h, var, val);
	}
	return val;
    }

    upenv = interp->func_stack[interp->cur_func_stack]->upstack;
    while (upenv) {
	h = upenv->localvar;
	if (h) {
	    val = hash_get_t(h, var);
	    if (NULL != val) {
		if (IS_LAZY(val) && (GET_TAG(val) == CLOSURE)) {
		    val = eval_closure(interp, val);
		    hash_set_t(h, var, val);
		}
		return val;
	    }
	}
	upenv = upenv->upstack;
    }

    h = (interp->obj_stack[interp->cur_obj_stack])->cur_object_slot;
    val = hash_get_t(h, var);
    if (NULL != val) {
	if (IS_LAZY(val) && (GET_TAG(val) == CLOSURE)) {
	    val = eval_closure(interp, val);
	    hash_set_t(h, var, val);
	}
	return val;
    }

    h = interp->globals;
    val = hash_get_t(h, var);
    if (NULL != val) {
	if (IS_LAZY(val) && (GET_TAG(val) == CLOSURE)) {
	    val = eval_closure(interp, val);
	    hash_set_t(h, var, val);
	}
	return val;
    }

    c = new_cell("No such variable, '");
    cell_add_str(c, cell_get_addr(var->u.ref.cell));
    cell_add_str(c, "'.");

    if (stack_trace) {
	return new_exception(TE_NOVAR, cell_get_addr(c), interp);
    } else {
	return new_exception(TE_NOVAR, cell_get_addr(c), NULL);
    }
}

Toy_Type*
set_closure_var(Toy_Interp *interp, Toy_Type *var, Toy_Type *val) {
    Hash *h;
    Toy_Func_Env *upenv;
    Toy_Type *v;

    upenv = interp->func_stack[interp->cur_func_stack]->upstack;
    while (upenv) {
	h = upenv->localvar;
	if (h) {
	    v = hash_get_t(h, var);
	    if (NULL != v) {
		if (NULL != val) {
		    hash_set_t(h, var, val);
		    return val;
		} else {
		    return v;
		}
	    }
	}
	upenv = upenv->upstack;
    }

    return NULL;
}

Toy_Type*
search_method(Toy_Interp *interp, Toy_Type *object, Toy_Type *method) {
    Hash *h;
    Toy_Type *ho;
    Toy_Type *m;
    Toy_Type *l;
    Toy_Type *a;
    Cell *msg;

    switch (GET_TAG(object)) {
    case NATIVE: case FUNC:
	return object;

    case OBJECT:
#ifdef HAS_GCACHE
	m = hash_get_method_cache(interp->gcache, object, method);
	if (m) {
	    interp->cache_hit ++;
	    return m;
	}
#endif

	h = object->u.object.slots;
	m = hash_get_t(h, method);
	if (m) {
#ifdef HAS_GCACHE
	    hash_set_method_cache(interp->gcache, object, method, m);
	    interp->cache_missing ++;
#endif
	    return m;
	} else {
	    l = object->u.object.delegate_list;

	    a = list_get_item(l);
	    while (a) {
		if (GET_TAG(a) == SYMBOL) {
		    ho = hash_get_t(interp->classes, a);
		    if (NULL == ho) goto error2;
		} else if (GET_TAG(a) == OBJECT) {
		    ho = a;
		} else {
		    goto error2;
		}

		m = search_method(interp, ho, method);
		if (GET_TAG(m) != EXCEPTION) {
#ifdef HAS_GCACHE
		    hash_set_method_cache(interp->gcache, object, method, m);
		    interp->cache_missing ++;
#endif
		    return m;
		}
		l = list_next(l);
		a = list_get_item(l);
	    }

	    goto error;
	}

    default:
	return toy_resolv_object(interp, object);
    }

error:
    msg = new_cell("No such method, '");
    cell_add_str(msg, to_string(method));
    cell_add_str(msg, "'.");

    return new_exception(TE_NOMETHOD, cell_get_addr(msg), interp);

error2:
    msg = new_cell("No such delegate class, '");
    cell_add_str(msg, cell_get_addr(a->u.symbol.cell));
    cell_add_str(msg, "'.");

    return new_exception(TE_NODELEGATE, cell_get_addr(msg), interp);
}

Toy_Type*
toy_resolv_object(Toy_Interp *interp, Toy_Type *object) {
    Toy_Type *val;
    Cell *msg;

    switch (GET_TAG(object)) {
    case NIL:
	val = hash_get_t(interp->classes, const_NilClass);
	break;
    case CLOSURE:
	val = hash_get_t(interp->classes, const_Block);
	break;
    case LIST:
	val = hash_get_t(interp->classes, const_List);
	break;
    case REAL:
	val = hash_get_t(interp->classes, const_Real);
	break;
    case INTEGER:
	val = hash_get_t(interp->classes, const_Integer);
	break;
    case STRING:
	val = hash_get_t(interp->classes, const_String);
	break;
    case RQUOTE:
	val = hash_get_t(interp->classes, const_RQuote);
	break;
    case EXCEPTION:
	val = hash_get_t(interp->classes, const_Exception);
	break;
#if 0
    case CALLCC:
	val = hash_get_t(interp->classes, const_CallCC);
	break;
#endif
    case DICT:
	val = hash_get_t(interp->classes, const_Dict);
	break;

    case VECTOR:
	val = hash_get_t(interp->classes, const_Vector);
	break;

    case COROUTINE:
	val = hash_get_t(interp->classes, const_Coro);
	break;

    default:
	goto error;
    }

    return val;

error:
    msg = new_cell("No such class, '");
    cell_add_str(msg, toy_get_type_str(object));
    cell_add_str(msg, "'.");

    return new_exception(TE_NOOBJECT, cell_get_addr(msg), interp);
}

#define PARAM_BIND(n) {							\
	val = list_get_item(l);						\
	var = &(a->array[n]);						\
	if (GET_TAG(val) == EVAL) {					\
	    if (*env == NULL) {						\
		*env = new_closure_env(interp);				\
	    }								\
	    val = new_closure(val->u.eval_body, *env, interp->script_id);\
	    if (GET_TAG(val) == EXCEPTION) {return val;}		\
	    SET_LAZY(val);						\
	} else {							\
	    val = toy_expand(interp, val, env);				\
	    if (GET_TAG(val) == EXCEPTION) {return val;}		\
	    if (IS_LAZY(var)) {SET_LAZY(val);}				\
	}								\
	hash_set_t(args, var, val);					\
	l = list_next(l);						\
}

static Toy_Type*
bind_args(Toy_Interp *interp, Toy_Type *arglist, struct _toy_argspec *aspec,
	  Toy_Env **env, Hash *args, int fun_paramno, int call_paramno) {

    int argpos, i;
    Toy_Type *l, *val, *var;
    Cell *msg;

    l = arglist;

    if ((fun_paramno < TAG_MAX_PARAMNO) && (call_paramno < TAG_MAX_PARAMNO)
	&& (fun_paramno == call_paramno)) {

	Array *a = aspec->posarg_array;
	i = 0;

	switch (fun_paramno) {
	case 6:
	    PARAM_BIND(i);
	    i ++;
	case 5:
	    PARAM_BIND(i);
	    i ++;
	case 4:
	    PARAM_BIND(i);
	    i ++;
	case 3:
	    PARAM_BIND(i);
	    i ++;
	case 2:
	    PARAM_BIND(i);
	    i ++;
	case 1:
	    PARAM_BIND(i);
	}

	return const_Nil;
    }

    argpos = 0;
    while (l) {
	val = list_get_item(l);

	if (IS_NAMED_SYM(val)) {

	    var = hash_get_t(aspec->namedarg, val);
	    if (NULL == var) goto error3;

	    l = list_next(l);
	    if (NULL == l) goto error4;

	    val = list_get_item(l);
	    val = toy_expand(interp, val, env);
	    if (GET_TAG(val) == EXCEPTION) return val;
	    if (IS_LAZY(var)) {
		SET_LAZY(val);
	    }

	    hash_set_t(args, var, val);

	} else if (IS_SWITCH_SYM(val)) {
	    Cell *cval;

	    cval = new_cell(&(cell_get_addr(val->u.symbol.cell))[1]);
	    cell_add_str(cval, ":");
	    var = hash_get(aspec->namedarg, cell_get_addr(cval));

	    if (NULL == var) goto error3;

	    hash_set_t(args, var, const_int1);

	} else {

	    var = array_get(aspec->posarg_array, argpos);

	    val = toy_expand(interp, val, env);
	    if (GET_TAG(val) == EXCEPTION) return val;
	    if (var && IS_LAZY(var)) {
		SET_LAZY(val);
	    }

	    if (NULL == var) {

		var = hash_get_t(aspec->namedarg, const_Args);
		if (NULL == var) goto error1;

		if (IS_LIST_NULL(l)) {
		    hash_set_t(args, var, new_list(NULL));
		} else {
		    
		    val = toy_expand(interp, l, env);
		    if (GET_TAG(val) == EXCEPTION) return val;

		    hash_set_t(args, var, val);
		}

		return const_Nil;
	    }

	    hash_set_t(args, var, val);

	    argpos++;
	}

	l = list_next(l);
    }

    if (argpos < aspec->posarg_len) goto error2;

    var = hash_get_t(aspec->namedarg, const_Args);
    if (NULL != var) {
	hash_set_t(args, var, new_list(NULL));
    }

    return const_Nil;
    

error1:
    msg = new_cell("Too many arguments, required: '");
    cell_add_str(msg, to_string(aspec->list));
    cell_add_str(msg, "'.");
    return new_exception(TE_MANYARGS, cell_get_addr(msg), interp);

error2:
    msg = new_cell("Few arguments, required: '");
    cell_add_str(msg, to_string(aspec->list));
    cell_add_str(msg, "'.");
    return new_exception(TE_FEWARGS, cell_get_addr(msg), interp);

error3:
    msg = new_cell("No specified named argument, '");
    cell_add_str(msg, to_string(val));
    cell_add_str(msg, "'.");
    return new_exception(TE_NOSPECARGS, cell_get_addr(msg), interp);

error4:
    msg = new_cell("No given named argument variable, name: '");
    cell_add_str(msg, to_string(val));
    cell_add_str(msg, "'.");
    return new_exception(TE_NONAMEARG, cell_get_addr(msg), interp);
}

Toy_Type*
eval_closure(Toy_Interp *interp, Toy_Type *closure) {
    Toy_Env *env;
    Cell *c;
    Toy_Type *result;
    int func_flag = 0, obj_flag = 0;

    if (GET_TAG(closure) != CLOSURE) {
	c = new_cell("Not a closure, '");
	cell_add_str(c, to_string(closure));
	c = new_cell("'.");
	return new_exception(TE_NORUNNABLE, cell_get_addr(c), interp);
    }

    env = closure->u.closure.env;

    if (interp->obj_stack[interp->cur_obj_stack] != env->object_env) {
	if (0 == toy_push_obj_env(interp, env->object_env)) {
	    result = new_exception(TE_STACKOVERFLOW, "Object satck overflow.", interp);
	    goto error_exit;
	}
	obj_flag = 1;
    }
    if (interp->func_stack[interp->cur_func_stack]->localvar != env->func_env->localvar) {
	if (0 == toy_push_func_env(interp, env->func_env->localvar, env->func_env, env->tobe_bind_val)) {
	    result = new_exception(TE_STACKOVERFLOW, "Function satck overflow.", interp);
	    goto error_exit;
	}
	func_flag = 1;
    } else {
	interp->func_stack[interp->cur_func_stack]->tobe_bind_val = env->tobe_bind_val;
    }

    result = toy_eval_script(interp, closure->u.closure.block_body);

error_exit:
    if (func_flag) toy_pop_func_env(interp);
    if (obj_flag) toy_pop_obj_env(interp);

    return result;
}

Toy_Type*
toy_call_init(Toy_Interp *interp, Toy_Type *object, Toy_Type *args) {
    Toy_Type *method;
    Toy_Obj_Env *obj_env;
    Toy_Type *posargs;
    Hash *namedargs;
    Toy_Type *result = NULL;

    method = search_method(interp, object, const_Init);
    if (GET_TAG(method) == EXCEPTION) {
	if (strcmp(cell_get_addr(method->u.exception.code), TE_NOMETHOD) == 0) {
	    return const_Nil;
	}
	return method;
    }
    obj_env = toy_new_obj_env(interp, object, object);
    
    posargs = args;
    namedargs = new_hash(NULL);

    switch (GET_TAG(method)) {
    case NATIVE:
	if (0 == toy_push_obj_env(interp, obj_env)) break;
	result = (method->u.native.cfunc)(interp, posargs, namedargs, list_length(posargs));
	toy_pop_obj_env(interp);
	break;

    case FUNC:
        {
	    Hash *local_var;
	    Toy_Type *r;
	    Toy_Env *env = NULL;
	
	    local_var = new_hash();
	    r = bind_args(interp, posargs, method->u.func.argspec, &env, local_var,
			  TAG_MAX_PARAMNO, TAG_MAX_PARAMNO);
	    if (GET_TAG(r) == EXCEPTION) return r;

	    if (0 == toy_push_obj_env(interp, obj_env)) {
		result = new_exception(TE_STACKOVERFLOW, "Object satck overflow.", interp);
		break;
	    }
	    if (0 == toy_push_func_env(interp, local_var,
				       method->u.func.closure->u.closure.env->func_env, NULL)) {
		toy_pop_obj_env(interp);
		result = new_exception(TE_STACKOVERFLOW, "Function satck overflow.", interp);
		break;
	    }
	    result = toy_eval_script(interp, method->u.func.closure->u.closure.block_body);
	    toy_pop_func_env(interp);
	    toy_pop_obj_env(interp);
	    break;
	}
    }

    return result;
}

Toy_Type*
toy_call(Toy_Interp *interp, Toy_Type *list) {
    Toy_Env *env;
    Toy_Type *result;

    env = new_closure_env(interp);

    result = toy_eval(interp, new_statement(list, interp->trace_info->line), &env);
    return result;
}

static Toy_Type*
eval_sig_handl(Toy_Interp *interp, int code) {
    Toy_Type *trapdic;
    Hash *hash;
    Toy_Type *sig;
    Toy_Type *block;

    trapdic = hash_get_t(interp->globals, const_attrap);
    if (NULL == trapdic) return NULL;

    hash = (Hash*)trapdic->u.dict;
    switch (code) {
    case SIGHUP:
	sig = const_SIGHUP;
	break;
    case SIGINT:
	sig = const_SIGINT;
	break;
    case SIGQUIT:
	sig = const_SIGQUIT;
	break;
    case SIGPIPE:
	sig = const_SIGPIPE;
	break;
    case SIGALRM:
	sig = const_SIGALRM;
	break;
    case SIGTERM:
	sig = const_SIGTERM;
	break;
    case SIGURG:
	sig = const_SIGURG;
	break;
    case SIGCHLD:
	sig = const_SIGCHLD;
	break;
    case SIGUSR1:
	sig = const_SIGUSR1;
	break;
    case SIGUSR2:
	sig = const_SIGUSR2;
	break;
    default:
	return NULL;
    }

    block = hash_get_t(hash, sig);
    if (NULL == block) return NULL;

    return eval_closure(interp, block);
}

char*
to_string_call(Toy_Interp *interp, Toy_Type *obj) {
    Toy_Env *env;
    Toy_Type *result;
    Toy_Type *list, *l;

    if (! ((GET_TAG(obj) == OBJECT) || IS_NOPRINTABLE(obj))) {
	return to_string(obj);	
    }

    env = NULL;

    l = list = new_list(obj);
    l = list_append(l, const_string);

    result = toy_eval(interp, new_statement(list, interp->trace_info->line), &env);
    if (GET_TAG(result) != STRING) return to_string(obj);

    return cell_get_addr(result->u.string);
}

static Toy_Type*
toy_yield_bind(Toy_Interp *interp, Toy_Type *bind_var) {
    Toy_Type *result, *l, *item, *vl = NULL, *v;
    Hash *local;

    local = interp->func_stack[interp->cur_func_stack]->localvar;
    if (interp->func_stack[interp->cur_func_stack]->tobe_bind_val != NULL) {
	vl = interp->func_stack[interp->cur_func_stack]->tobe_bind_val;
    }

    l = result = new_list(NULL);
    while (! IS_LIST_NULL(bind_var)) {
	item = list_get_item(bind_var);
	if (GET_TAG(item) != SYMBOL) goto error;

	if (vl && (! IS_LIST_NULL(vl))) {
	    v = list_get_item(vl);
	    hash_set_t(local, item, v);
	    vl = list_next(vl);
	    l = list_append(l, v);
	} else {
	    hash_set_t(local, item, const_Nil);
	    l = list_append(l, const_Nil);
	}
	
	bind_var = list_next(bind_var);
    }

    return result;

error:
    return new_exception(TE_BADBINDSPEC, "Bad bind spec.", interp);
}

Toy_Type*
toy_yield(Toy_Interp *interp, Toy_Type *closure, Toy_Type *args) {
    Toy_Env *env;
    Cell *c;

    if (GET_TAG(closure) != CLOSURE) {
	c = new_cell("Not a closure, '");
	cell_add_str(c, to_string(closure));
	c = new_cell("'.");
	return new_exception(TE_NORUNNABLE, cell_get_addr(c), interp);
    }
    
    env = closure->u.closure.env;
    env->tobe_bind_val = args;

    return eval_closure(interp, closure);
}
