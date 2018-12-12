/* $Id: commands.c,v 1.68 2012/03/06 06:09:27 mit-sato Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <setjmp.h>
#include <float.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#if __FreeBSD__
#include <netinet/in.h>
#endif /* __FreeBSD__ */
#include <netdb.h>
#include "config.h"
#include "toy.h"
#include "interp.h"
#include "types.h"
#include "cell.h"
#include "global.h"
#include "hash.h"
#include "array.h"
#include "bulk.h"
#include "cstack.h"
#include "util.h"
#include "encoding.h"

static void println(Toy_Interp *interp, wchar_t *msg);


Toy_Type*
cmd_false(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    return const_Nil;
}

Toy_Type*
cmd_true(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    return const_T;
}

Toy_Type*
cmd_set(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *var, *val;
    Hash *h;
    int len;
    Toy_Type *res, *var2, *val2, *newargs, *res2;

    if (hash_get_length(nameargs) > 0) goto error;
    len = arglen;

    if (((len > 2) || (len < 1)) ||
	(hash_get_length(nameargs) > 0)) goto error;

    var = list_get_item(posargs);
    if (GET_TAG(var) == LIST) {
	
	if (len != 2) goto error;

	/* multiple variable set */

	posargs = list_next(posargs);
	val = list_get_item(posargs);

	if (GET_TAG(val) == LIST) {
	    if (list_length(var) != list_length(val)) goto error;
	    
	    res2 = new_list(NULL);

	    while (var && val) {
		var2 = list_get_item(var);
		val2 = list_get_item(val);
		newargs = new_list(var2);
		list_append(newargs, val2);

		res = cmd_set(interp, newargs, nameargs, 2);
		if (GET_TAG(res) == EXCEPTION) return res;
		
		list_append(res2, res);

		var = list_next(var);
		val = list_next(val);
	    }

	    return res2;
	}

	goto error;
    }

    if (GET_TAG(var) != SYMBOL) goto error;

    h = interp->func_stack[interp->cur_func_stack]->localvar;

    if (len == 1) {
	res = hash_get_t(h, var);
	if (NULL == res) {
	    Cell *msg;

	    msg = new_cell(L"No such variable '");
	    cell_add_str(msg, to_string(var));
	    cell_add_str(msg, L"'.");
	    return new_exception(TE_NOVAR, cell_get_addr(msg), interp);
	}
	return res;

    } else {

	posargs = list_next(posargs);
	val = list_get_item(posargs);
	if (NULL == val) goto error;

	switch (GET_TAG(val)) {
	case INTEGER: case REAL:
	    val = toy_clone(val);
	    break;
	case STRING:
	    val = new_string_str(cell_get_addr(val->u.string));
	    break;
	}

	hash_set_t(h, var, val);

	return val;
    }

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: \n\tset var [val]\n\
\tor\n\tset (var1 var2 ...) (val1 val2 ...)\n", interp);
}

Toy_Type*
cmd_sets(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *var, *val;
    Hash *h;
    int len;
    Toy_Type *res;

    if (hash_get_length(nameargs) > 0) goto error;
    h = interp->obj_stack[interp->cur_obj_stack]->cur_object_slot;
    len = arglen;

    if (((len > 2) || (len < 1)) ||
	(hash_get_length(nameargs) > 0)) goto error;

    var = list_get_item(posargs);
    if (GET_TAG(var) != SYMBOL) goto error;

    if (len == 1) {

	res = hash_get_t(h, var);
	if (NULL == res) {
	    Cell *msg;
	    msg = new_cell(L"No such variable '");
	    cell_add_str(msg, to_string(var));
	    cell_add_str(msg, L"'.");
	    return new_exception(TE_NOVAR, cell_get_addr(msg), interp);
	}
	return res;

    } else {

	posargs = list_next(posargs);
	val = list_get_item(posargs);
	switch (GET_TAG(val)) {
	case INTEGER: case REAL:
	    hash_set_t(h, var, toy_clone(val));
	    break;
	case STRING:
	    hash_set_t(h, var, new_string_str(cell_get_addr(val->u.string)));
	    break;
	default:
	    hash_set_t(h, var, val);
	}

	return val;
    }

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: sets var [val]", interp);
}

Toy_Type*
cmd_setc(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *var, *val;
    int len;
    Toy_Type *res;

    if (hash_get_length(nameargs) > 0) goto error;
    len = arglen;

    if ((len > 2) || (len < 1)) goto error;

    var = list_get_item(posargs);
    if (GET_TAG(var) != SYMBOL) goto error;

    if (len == 1) {

	res = set_closure_var(interp, var, NULL);
	if (NULL == res) {
	    Cell *msg;
	    msg = new_cell(L"No such variable '");
	    cell_add_str(msg, to_string(var));
	    cell_add_str(msg, L"'.");
	    return new_exception(TE_NOVAR, cell_get_addr(msg), interp);
	}
	return res;

    } else {

	posargs = list_next(posargs);
	val = list_get_item(posargs);
	switch (GET_TAG(val)) {
	case INTEGER: case REAL:
	    res = set_closure_var(interp, var, toy_clone(val));
	    break;
	case STRING:
	    res = set_closure_var(interp, var, new_string_str(cell_get_addr(val->u.string)));
	    break;
	default:
	    res = set_closure_var(interp, var, val);
	}

	if (NULL == res) {
	    Cell *msg;
	    msg = new_cell(L"No such variable '");
	    cell_add_str(msg, to_string(var));
	    cell_add_str(msg, L"'.");
	    return new_exception(TE_NOVAR, cell_get_addr(msg), interp);
	}
	
	return val;
    }

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: setc var [val]", interp);
}

Toy_Type*
cmd_defvar(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *var, *val;
    Hash *h;
    int len;
    Toy_Type *res;

    if (hash_get_length(nameargs) > 0) goto error;
    h = interp->globals;
    len = arglen;

    if (((len > 2) || (len < 1)) ||
	(hash_get_length(nameargs) > 0)) goto error;

    var = list_get_item(posargs);
    if (GET_TAG(var) != SYMBOL) goto error;

    if (len == 1) {

	res = hash_get_t(h, var);
	if (NULL == res) {
	    Cell *msg;
	    msg = new_cell(L"No such variable '");
	    cell_add_str(msg, to_string(var));
	    cell_add_str(msg, L"'.");
	    return new_exception(TE_NOVAR, cell_get_addr(msg), interp);
	}
	return res;

    } else {
	posargs = list_next(posargs);
	val = list_get_item(posargs);
	if (hash_is_exists_t(h, var)) {
	    Cell *msg;
	    msg = new_cell(L"Already exists '");
	    cell_add_str(msg, to_string(var));
	    cell_add_str(msg, L"'.");
	    return new_exception(TE_ALREADYDEF, cell_get_addr(msg), interp);
	}
	switch (GET_TAG(val)) {
	case INTEGER: case REAL:
	    hash_set_t(h, var, toy_clone(val));
	    break;
	case STRING:
	    hash_set_t(h, var, new_string_str(cell_get_addr(val->u.string)));
	    break;
	default:
	    hash_set_t(h, var, val);
	}

	return val;
    }

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: defvar var [val]", interp);
}

Toy_Type*
cmd_setvar(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *var, *val;
    Hash *h;
    int len;
    Toy_Type *res;

    if (hash_get_length(nameargs) > 0) goto error;
    h = interp->globals;
    len = arglen;

    if (((len > 2) || (len < 1)) ||
	(hash_get_length(nameargs) > 0)) goto error;

    var = list_get_item(posargs);
    if (GET_TAG(var) != SYMBOL) goto error;

    if (len == 1) {

	res = hash_get_t(h, var);
	if (NULL == res) {
	    Cell *msg;
	    msg = new_cell(L"No such variable '");
	    cell_add_str(msg, to_string(var));
	    cell_add_str(msg, L"'.");
	    return new_exception(TE_NOVAR, cell_get_addr(msg), interp);
	}
	return res;

    } else {
	posargs = list_next(posargs);
	val = list_get_item(posargs);
	if (! hash_is_exists_t(h, var)) {
	    Cell *msg;
	    msg = new_cell(L"No such variable '");
	    cell_add_str(msg, to_string(var));
	    cell_add_str(msg, L"'.");
	    return new_exception(TE_NOVAR, cell_get_addr(msg), interp);
	}
	switch (GET_TAG(val)) {
	case INTEGER: case REAL:
	    hash_set_t(h, var, toy_clone(val));
	    break;
	case STRING:
	    hash_set_t(h, var, new_string_str(cell_get_addr(val->u.string)));
	    break;
	default:
	    hash_set_t(h, var, val);
	}

	return val;
    }

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: setvar var [val]", interp);
}

Toy_Type*
cmd_fun(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *l;
    Toy_Type *argv;
    wchar_t *argvs;
    Toy_Type *argspec, *body;
    Toy_Type* func;
    int nposarg = 0;
    Hash *namedargh;
    Array *posarga;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 2) goto error;

    l = new_list(NULL);
    namedargh = new_hash();
    posarga = new_array();

    /* make argspec */

    argspec = list_get_item(posargs);
    if (GET_TAG(argspec) != LIST) goto error;

    while (! IS_LIST_NULL(argspec)) {

	argv = list_get_item(argspec);
	if (GET_TAG(argv) != SYMBOL) goto error;

	argvs = cell_get_addr(argv->u.symbol.cell);
	list_append(l, argv);

	if (argvs[cell_get_length(argv->u.symbol.cell)-1] == L':') {
	    Toy_Type *key;

	    key = argv;
	    argspec = list_next(argspec);
	    argv = list_get_item(argspec);
	    if (GET_TAG(argv) != SYMBOL) goto error;

	    if ((cell_get_addr(argv->u.symbol.cell))[0] == L'&') {
		Toy_Type *s;
		s = new_symbol(&((cell_get_addr(argv->u.symbol.cell))[1]));
		SET_LAZY(s);
		hash_set_t(namedargh, key, s);
	    } else {
		hash_set_t(namedargh, key, argv);
	    }
	    list_append(l, argv);

	} else {
	    if ((cell_get_addr(argv->u.symbol.cell))[0] == L'&') {
		Toy_Type *s;
		s = new_symbol(&((cell_get_addr(argv->u.symbol.cell))[1]));
		SET_LAZY(s);
		array_append(posarga, s);
	    } else {
		array_append(posarga, argv);
	    }
	    nposarg++;
	}

	argspec = list_next(argspec);
    }

    body = list_get_item(list_next(posargs));
    if (GET_TAG(body) != CLOSURE) goto error;

    func = new_func(l, nposarg, posarga, namedargh, body);

    return func;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'fun', syntax: fun (argspec) {block}\n\t\
argspec: [arg ...] [keyword: arg ...] [args: variable-args ...]", interp);
}

Toy_Type*
cmd_defun(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *err;
    Toy_Type *res;
    Toy_Type *cmd;
    Hash *h;
    
    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 3) goto error;

    cmd = list_get_item(posargs);
    if (GET_TAG(cmd) != SYMBOL) goto error;

    posargs = list_next(posargs);

    arglen--;
    res = cmd_fun(interp, posargs, nameargs, arglen);

    if (GET_TAG(res) == EXCEPTION) {
	err = res;
	if (cell_eq_str(err->u.exception.code, TE_SYNTAX) == 0) goto error;

	return err;
    }

    h = interp->funcs;
    hash_set_t(h, cmd, res);

    return res;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'defun', syntax: defun name (argspec) {block}\n\t\
argspec: [arg ...] [keyword: arg ...] [args: variable-args ...]", interp);
}

Toy_Type*
cmd_info(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *h2;
    Toy_Type *option;
    wchar_t *options;

    if (hash_get_length(nameargs) > 0) goto error;

    option = list_get_item(posargs);
    if (NULL == option) goto error;
    if (GET_TAG(option) != SYMBOL) goto error;

    options = cell_get_addr(option->u.symbol.cell);

    if (wcscmp(options, L"local") == 0) {
	h2 = interp->func_stack[interp->cur_func_stack]->localvar;
    } else if (wcscmp(options, L"closure") == 0) {
	Toy_Type *l, *s;
	Toy_Func_Env *p;

	l = new_list(NULL);
	p = interp->func_stack[interp->cur_func_stack]->upstack;
	while (p) {
	    s = hash_get_keys(p->localvar);
	    while (! IS_LIST_NULL(s)) {
		list_append(l, list_get_item(s));
		s = list_next(s);
	    }
	    p = p->upstack;
	}
	return l;
    } else if (wcscmp(options, L"self") == 0) {
	h2 = interp->obj_stack[interp->cur_obj_stack]->cur_object_slot;
    } else if (wcscmp(options, L"func") == 0) {
	h2 = interp->funcs;
    } else if (wcscmp(options, L"class") == 0) {
	h2 = interp->classes;
    } else if (wcscmp(options, L"global") == 0) {
	h2 = interp->globals;
    } else if (wcscmp(options, L"script") == 0) {
	h2 = interp->scripts;
    } else goto error;

    if (NULL == h2) return new_list(NULL);
    return hash_get_keys(h2);
    
error:
    return new_exception(TE_SYNTAX,
      L"Syntax error at 'info', syntax: info local | closure | self | func | class | global | script", interp);
}

Toy_Type*
cmd_return(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 1) goto error;

    if (arglen == 0) {
	return new_control(CTRL_RETURN, const_Nil);
    } else {
	return new_control(CTRL_RETURN, list_get_item(posargs));
    }

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'return', syntax: return [val]", interp);
}

Toy_Type*
cmd_break(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 1) goto error;

    if (arglen == 0) {
	return new_control(CTRL_BREAK, const_Nil);
    } else {
	return new_control(CTRL_BREAK, list_get_item(posargs));
    }

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'break', syntax: break [val]", interp);
}

Toy_Type*
cmd_continue(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 0) goto error;

    return new_control(CTRL_CONTINUE, const_Nil);

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'continue', syntax: continue", interp);
}

Toy_Type*
cmd_redo(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 0) goto error;

    return new_control(CTRL_REDO, const_Nil);

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'redo', syntax: redo", interp);
}

Toy_Type*
cmd_retry(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 0) goto error;

    return new_control(CTRL_RETRY, const_Nil);

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'retry', syntax: retry", interp);
}


Toy_Type*
cmd_new(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *object;
    Toy_Type *delegate_list, *add_delegate_list;
    Toy_Type *pname;
    Cell *msg;
    Toy_Type *l;
    int t;
    Toy_Type *args, *result;

    switch (arglen) {
    case 0:
	pname = const_Object;
	break;

    case 1:
	pname = list_get_item(posargs);
	if (GET_TAG(pname) != SYMBOL) goto error1;
	break;

    default:
	goto error1;
    }

    object = hash_get_t(interp->classes, pname);
    if (NULL == object) {
	/* auto load class file */
	Toy_Type *path;

	path = hash_get(interp->globals, L"LIB_PATH");
	if (path && (GET_TAG(path) == LIST)) {
	    while (path) {
		Cell *spath;
		Toy_Type *loadl, *result;

		spath = new_cell(to_string(list_get_item(path)));
		cell_add_str(spath, L"/");
		cell_add_str(spath, to_string(pname));
		cell_add_str(spath, L".prfm");

		loadl = new_list(new_symbol(L"load"));
		list_append(loadl, new_string_cell(spath));
		result = toy_eval_script(interp,
					 new_script(new_list(new_statement(loadl,
						    interp->func_stack[interp->cur_func_stack]->trace_info->line))));
		if ((GET_TAG(result) == EXCEPTION) &&
		    (0 != wcscmp(cell_get_addr(result->u.exception.code), TE_FILENOTOPEN))) {
		    return result;
		} else {
		    if (GET_TAG(result) != EXCEPTION) break;
		}
		
		path = list_next(path);
	    }
	    object = hash_get_t(interp->classes, pname);
	    if (NULL == object) goto error2;

	} else {
	    goto error2;
	}
    }

    l = delegate_list = new_list(pname);

    add_delegate_list = hash_get_and_unset_t(nameargs, const_delegate);
    if (NULL != add_delegate_list) {
    
	t = GET_TAG(add_delegate_list);
	if ((t == SYMBOL) || (t == OBJECT)) {
	    l = list_append(l, add_delegate_list);
	} else if (t == LIST) {
	    Toy_Type *tl;
	    tl = add_delegate_list;
	    while (tl) {
		if (GET_TAG(list_get_item(tl)) != SYMBOL) goto error1;
		tl = list_next(tl);
		if (tl && (GET_TAG(tl) != LIST)) goto error1;
	    }
	    list_set_cdr(l, add_delegate_list);
	} else {
	    goto error1;
	}
    }

    args = hash_get_and_unset_t(nameargs, const_init);

    if (hash_get_length(nameargs) > 0) goto error1;

    object = new_object(cell_get_addr(pname->u.symbol.cell), new_hash(), delegate_list);

    /* call initializer */

    result = toy_call_init(interp, object, args);

    if (GET_TAG(result) == EXCEPTION) {
	return result;
    } else {
	return object;
    }

error1:
    return new_exception(TE_SYNTAX,
	 L"Syntax error at 'new', syntax: new [class]\n\t\
[delegate: (additional-delegate-class ...)] [init: (val ...)]", interp);

error2:
    msg = new_cell(L"No such class '");
    cell_add_str(msg, cell_get_addr(pname->u.symbol.cell));
    cell_add_str(msg, L"'.");
    return new_exception(TE_NOCLASS, cell_get_addr(msg), interp);
}

Toy_Type*
cmd_showstack(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *l, *p;

    l = new_list(NULL);

    p = new_cons(new_symbol(L"stack_size"), new_integer_si(interp->stack_size));
    list_append(l, p);

    p = new_cons(new_symbol(L"cur_obj_stack"), new_integer_si(interp->cur_obj_stack));
    list_append(l, p);

    p = new_cons(new_symbol(L"cur_func_stack"), new_integer_si(interp->cur_func_stack));
    list_append(l, p);

    return l;
}

Toy_Type*
cmd_if(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *cond_block, *then_block, *else_block;
    Toy_Type *cond, *result;
    int t;
    
    if (hash_get_length(nameargs) == 0) {
	if (! ((arglen >= 1) && (arglen <= 3))) goto error;
	cond_block = list_get_item(posargs);
	posargs = list_next(posargs);

	then_block = const_Nil;
	else_block = const_Nil;
	if (! IS_LIST_NULL(posargs)) {
	    then_block = list_get_item(posargs);
	    posargs = list_next(posargs);
	}
	if (! IS_LIST_NULL(posargs)) {
	    else_block = list_get_item(posargs);
	}
	
    } else {
	if (arglen != 1) goto error;
	if (hash_get_length(nameargs) > 2) goto error;

	cond_block = list_get_item(posargs);
	then_block = hash_get_and_unset_t(nameargs, const_then);
	else_block = hash_get_and_unset_t(nameargs, const_else);
	if (hash_get_length(nameargs) > 0) goto error;
    }

    if (GET_TAG(cond_block) == CLOSURE) {
	cond = eval_closure(interp, cond_block, interp->trace_info);
	t = GET_TAG(cond);
	if (t == EXCEPTION) return cond;
	if (t == CONTROL) {
	    cond = cond->u.control.ret_value;
	    t = GET_TAG(cond);
	}
    } else {
	t = GET_TAG(cond_block);
    }

    if (t == NIL) {
	if (else_block == NULL) return const_Nil;
	if (GET_TAG(else_block) != CLOSURE) return else_block;
	result = eval_closure(interp, else_block, interp->trace_info);
    } else {
	if (then_block == NULL) return const_Nil;
	if (GET_TAG(then_block) != CLOSURE) return then_block;
	result = eval_closure(interp, then_block, interp->trace_info);
    }

    return result;
    
error:
    return new_exception(TE_SYNTAX,
L"Syntax error at 'if', \n\
syntax(1): if cond [then: then-body] [else: else-body]\n\
syntax(2): if cond [then-body] [else-body]\n\
	cond:		{cond-block} or value\n\
	then-body:	{then-block} or value\n\
	else-body:	{else-block} or value",
			 interp);
}

Toy_Type*
cmd_while(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *cond_block, *do_block;
    Toy_Type *cond, *result;
    int t, r;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 1) goto error;

    cond_block = list_get_item(posargs);
    do_block = hash_get_and_unset_t(nameargs, const_do);
    if (hash_get_length(nameargs) > 0) goto error;
    if ((do_block == NULL) ||
	(GET_TAG(do_block) != CLOSURE)) goto error;

    result = const_Nil;

loop:
    cond = eval_closure(interp, cond_block, interp->trace_info);
    t = GET_TAG(cond);
    if (t == EXCEPTION) return cond;
    if (t == CONTROL) {
	cond = cond->u.control.ret_value;
	t = GET_TAG(cond);
    }

    if (t != NIL) {
    loop2:
	result = eval_closure(interp, do_block, interp->trace_info);
	r = GET_TAG(result);
	if (r == EXCEPTION) return result;
	if (r == CONTROL) {
	    switch (result->u.control.code) {
	    case CTRL_RETURN: case CTRL_GOTO:
		return result;
	    case CTRL_BREAK:
		return result->u.control.ret_value;
	    case CTRL_CONTINUE:
		result = const_Nil;
		goto loop;
	    case CTRL_REDO:
		result = const_Nil;
		goto loop2;
	    case CTRL_RETRY:
		result = const_Nil;
		goto loop;
	    }
	}
	goto loop;
    }

    return result;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'while', syntax: while {cond-block} do: {do-block}", interp);
}

Toy_Type*
cmd_print(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *l, *body;
    Cell *s;
    Toy_Type *file, *result;

    if (hash_get_length(nameargs) > 1) goto error;

    file = hash_get_and_unset_t(nameargs, const_filec);
    if (hash_get_length(nameargs) > 0) goto error;

    if (NULL == file) {
	file = toy_resolv_var(interp, const_atout, 0, interp->trace_info);
	if (GET_TAG(file) == EXCEPTION) {
	    file = toy_resolv_var(interp, const_stdout, 0, interp->trace_info);
	    if (GET_TAG(file) == EXCEPTION) {
		return new_exception(TE_NOVAR,
				     L"No such file object '@out' and 'stdout'.", interp);
	    }
	}
    }
    
    l = body = new_list(file);

    s = new_cell(L"");
    while (! IS_LIST_NULL(posargs)) {
	cell_add_str(s, to_string_call(interp, list_get_item(posargs)));
	posargs = list_next(posargs);
    }

    l = list_append(l, const_puts);
    l = list_append(l, new_symbol(L":nonewline"));
    l = list_append(l, new_string_cell(s));

    result = toy_call(interp, body);

    if (GET_TAG(result) == EXCEPTION) return result;

    return const_Nil;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at \'print\', syntax: print [file: file-object] item ...",
			 interp);
}

Toy_Type*
cmd_println(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *l, *body;
    Cell *s;
    Toy_Type *file, *result;

    if (hash_get_length(nameargs) > 1) goto error;

    file = hash_get_and_unset_t(nameargs, const_filec);
    if (hash_get_length(nameargs) > 0) goto error;

    if (NULL == file) {
	file = toy_resolv_var(interp, const_atout, 0, interp->trace_info);
	if (GET_TAG(file) == EXCEPTION) {
	    file = toy_resolv_var(interp, const_stdout, 0, interp->trace_info);
	    if (GET_TAG(file) == EXCEPTION) {
		return new_exception(TE_NOVAR,
				     L"No such file object '@out' and 'stdout'.", interp);
	    }
	}
    }
    
    l = body = new_list(file);

    s = new_cell(L"");
    while (! IS_LIST_NULL(posargs)) {
	cell_add_str(s, to_string_call(interp, list_get_item(posargs)));
	posargs = list_next(posargs);
    }

    l = list_append(l, const_puts);
    l = list_append(l, new_string_cell(s));

    result = toy_call(interp, body);

    if (GET_TAG(result) == EXCEPTION) return result;

    return const_Nil;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at \'println\', syntax: println [file: file-object] item ...",
			 interp);
}

Toy_Type*
cmd_time(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *script;
    Toy_Type *result = NULL;
    struct timeval s, e;
    double rs, re, rt;
    wchar_t *buff;
    Toy_Type *l, *v;
    int count = 1, i;
    double min = DBL_MAX, max = 0.0, avg = 0.0, sum = 0.0;

    buff = GC_MALLOC(256*sizeof(wchar_t));
    ALLOC_SAFE(buff);

    if (arglen > 1) goto error;
    v = hash_get_and_unset_t(nameargs, new_symbol(L"count:"));
    if (v) {
	if (GET_TAG(v) != INTEGER) goto error;
	count = mpz_get_si(v->u.biginteger);
	if (count <= 0) count = 1;
    }
    if (hash_get_length(nameargs) > 0) goto error;

    for (i=1; i<=count; i++) {
	script = list_get_item(posargs);
	if (GET_TAG(script) != CLOSURE) goto error;

	gettimeofday(&s, NULL);
	rs = (double)s.tv_sec + ((double)s.tv_usec)/1000000.0;

	result = eval_closure(interp, script, interp->trace_info);
	if (GET_TAG(result) == EXCEPTION) return result;
	
	gettimeofday(&e, NULL);
	re = (double)e.tv_sec + ((double)e.tv_usec)/1000000.0;

	rt = re - rs;
	sum += rt;
	if (rt < min) min = rt;
	if (rt > max) max = rt;

	/* print Elapsed time */
	if (count == 1) {
	    swprintf(buff, 256, L"Elapsed time: %f", rt);
	} else {
	    swprintf(buff, 256, L"Elapsed time #%d: %f", i, rt);
	}
	buff[255] = 0;
	l = new_list(new_symbol(L"println"));
	list_append(l, new_string_str(buff));
	toy_call(interp, l);
    }

    if (count > 1) {
	avg = sum / count;

	/* print separator time */
	swprintf(buff, 256, L"--");
	buff[255] = 0;
	l = new_list(new_symbol(L"println"));
	list_append(l, new_string_str(buff));
	toy_call(interp, l);

	/* print min time */
	swprintf(buff, 256, L"Min time: %f", min);
	buff[255] = 0;
	l = new_list(new_symbol(L"println"));
	list_append(l, new_string_str(buff));
	toy_call(interp, l);

	/* print Max time */
	swprintf(buff, 256, L"Max time: %f", max);
	buff[255] = 0;
	l = new_list(new_symbol(L"println"));
	list_append(l, new_string_str(buff));
	toy_call(interp, l);

	/* print Total time */
	swprintf(buff, 256, L"Total time: %f", sum);
	buff[255] = 0;
	l = new_list(new_symbol(L"println"));
	list_append(l, new_string_str(buff));
	toy_call(interp, l);

	/* print Avg time */
	swprintf(buff, 256, L"Average time: %f", avg);
	buff[255] = 0;
	l = new_list(new_symbol(L"println"));
	list_append(l, new_string_str(buff));
	toy_call(interp, l);
    }
    
    return result;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'time', syntax: time [count: n] {block}", interp);
}

Toy_Type*
cmd_class(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *result;
    Toy_Type *name;

    name = list_get_item(posargs);
    if (GET_TAG(name) != SYMBOL) goto error;

    arglen--;
    result = cmd_new(interp, list_next(posargs), nameargs, arglen);
    if (GET_TAG(result) != OBJECT) return result;

    hash_set_t(interp->classes, name, result);

    return result;

error:
    return new_exception(TE_SYNTAX,
	 L"Syntax error at 'class', syntax: class name [class] \n\t[delegate: (additional-delegate-class ...)]", interp);
}

Toy_Type*
cmd_try(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *body;
    Toy_Type *catch_body;
    Toy_Type *fin_body;
    Toy_Type *result, *result_fin;

    if (arglen != 1) goto error;
    body = list_get_item(posargs);
    if (GET_TAG(body) != CLOSURE) goto error;

    catch_body = hash_get_and_unset_t(nameargs, const_catch);
    if (catch_body && (GET_TAG(catch_body) != CLOSURE)) goto error;

    fin_body = hash_get_and_unset_t(nameargs, const_fin);
    if (fin_body && (GET_TAG(fin_body) != CLOSURE)) goto error;

    if (hash_get_length(nameargs) > 0) goto error;

retry:
    result = eval_closure(interp, body, interp->trace_info);

    if (GET_TAG(result) == EXCEPTION) {
	if (catch_body) {
	    result = toy_yield(interp, catch_body,
			       new_list(new_cons(new_symbol(cell_get_addr(result->u.exception.code)),
						 list_get_item(result->u.exception.msg_list))));
	}
    }

    if (GET_TAG(result) == CONTROL) {
	if (result->u.control.code == CTRL_RETRY) goto retry;
#if 0
	if (result->u.control.code == CTRL_BREAK) {
	    result = result->u.control.ret_value;
	}
#endif
    }

    if (fin_body) {
	result_fin = eval_closure(interp, fin_body, interp->trace_info);
	if (GET_TAG(result_fin) == EXCEPTION) {
	    return result_fin;
	}
    }

    return result;
    
error:
    return new_exception(TE_SYNTAX,
	 L"Syntax error at 'try',\n\tsyntax: try {body} [catch: {| exception | catch-body}] [fin: {fin-body}]",
			 interp);
}

Toy_Type*
cmd_throw(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *name;
    Toy_Type *msg;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 2) goto error;

    name = list_get_item(posargs);
    if (GET_TAG(name) == SYMBOL) {

	msg = NULL;
	if (arglen == 2) {
	    msg = list_get_item(list_next(posargs));
	    if (GET_TAG(msg) != STRING) goto error;
	    
	    return new_exception(cell_get_addr(name->u.symbol.cell),
				 cell_get_addr(msg->u.string), interp);
	}
	
	return new_exception(cell_get_addr(name->u.symbol.cell), L"", interp);

    } else if (GET_TAG(name) == LIST) {

	Toy_Type *l;

	l = new_list(NULL);
	list_append(l, list_get_item(name));
	if (list_next(name)) {
	    list_append(l, list_next(name));
	} else {
	    list_append(l, new_string_str(L""));
	}

	return cmd_throw(interp, l, nameargs, 2);
    }
    
error:
    return new_exception(TE_SYNTAX,
	 L"Syntax error at 'throw', syntax: throw name [\"message\"] | (name . [message])", interp);
}

Toy_Type*
cmd_case(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *var;
    Toy_Type *val;
    Toy_Type *body;
    Toy_Type *l;
    Toy_Type *result;
    wchar_t *pvar;

    if ((arglen < 3) || ((arglen % 2) == 0)) goto error;

    var = list_get_item(posargs);
    pvar = to_string_call(interp, var);
    l = list_next(posargs);

    result = const_Nil;

    while (l) {
	val = list_get_item(l);
	l = list_next(l);
	body = list_get_item(l);

	if (wcscmp(to_string_call(interp, val), L"*") == 0) {
	    if (GET_TAG(body) == CLOSURE) {
		result = eval_closure(interp, body, interp->trace_info);
	    } else {
		result = body;
	    }
	    break;

	} else if (wcscmp(pvar, to_string_call(interp, val)) == 0) {
	    if (GET_TAG(body) == CLOSURE) {
		result = eval_closure(interp, body, interp->trace_info);
	    } else {
		result = body;
	    }
	    break;
	}

	l = list_next(l);
    }

    return result;
    
error:
    return new_exception(TE_SYNTAX,
	 L"Syntax error at 'case',\n\tsyntax: case var val1 {body1} val2 {body2} ... * {default-body}", interp);
}

Toy_Type*
cmd_cond(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *l;
    Toy_Type *exp, *cond, *body;
    Toy_Type *result = const_Nil;

    if ((arglen < 1) || ((arglen % 2) != 0)) goto error;

    l = posargs;

    while (l) {
	exp = list_get_item(l);
	l = list_next(l);
	body = list_get_item(l);
	l = list_next(l);

	if (GET_TAG(exp) == CLOSURE) {
	    cond = eval_closure(interp, exp, interp->trace_info);
	} else {
	    cond = exp;
	}
	if (GET_TAG(cond) == EXCEPTION) return cond;

	if (GET_TAG(cond) != NIL) {
	    if (GET_TAG(body) == CLOSURE) {
		result = toy_yield(interp, body, new_list(cond));
	    } else {
		result = body;
	    }
	    if (GET_TAG(result) == CONTROL) {
		if (result->u.control.code == CTRL_BREAK) {
		    return result->u.control.ret_value;
		} else {
		    return result;
		}
	    }
	    if (GET_TAG(result) == EXCEPTION) return result;
	} else {
	    result = const_Nil;
	}
    }

    return result;

error:
    return new_exception(TE_SYNTAX,
	 L"Syntax error at 'cond',\n\tsyntax: cond exp1 {| e1 | body1} exp2 {| e2 | body2} ...", interp);
}

Toy_Type*
cmd_isset(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *var;
    Hash *h;
    Toy_Type *res;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    h = interp->func_stack[interp->cur_func_stack]->localvar;

    var = list_get_item(posargs);
    if (GET_TAG(var) != SYMBOL) goto error;

    res = hash_get_t(h, var);
    if (NULL == res) {
	return const_Nil;
    } else {
	return const_T;
    }

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: set? var", interp);
}

Toy_Type*
cmd_issets(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *var;
    Hash *h;
    Toy_Type *res;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    h = interp->obj_stack[interp->cur_obj_stack]->cur_object_slot;

    var = list_get_item(posargs);
    if (GET_TAG(var) != SYMBOL) goto error;

    res = hash_get_t(h, var);
    if (NULL == res) {
	return const_Nil;
    } else {
	return const_T;
    }

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: sets? var", interp);
}

Toy_Type*
cmd_issetc(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *var, *res;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    var = list_get_item(posargs);
    if (GET_TAG(var) != SYMBOL) goto error;

    res = set_closure_var(interp, var, NULL);
    if (NULL == res) {
	return const_Nil;
    } else {
	return const_T;
    }

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: setc? var", interp);
}

Toy_Type*
cmd_unset(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *var;
    Hash *h;
    Toy_Type *res;
    Toy_Type *silent;
    
    silent = hash_get_and_unset_t(nameargs, const_silent);

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    h = interp->func_stack[interp->cur_func_stack]->localvar;

    var = list_get_item(posargs);
    if (GET_TAG(var) != SYMBOL) goto error;

    res = hash_get_and_unset_t(h, var);
    if (NULL == res) {
	return const_Nil;
    } else {
	if (NULL == silent) {
	    return res;
	} else {
	    return const_Nil;
	}
    }

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: unset [:silent] var", interp);
}

Toy_Type*
cmd_unsets(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *var;
    Hash *h;
    Toy_Type *res;
    Toy_Type *silent;
    
    silent = hash_get_and_unset_t(nameargs, const_silent);

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    h = interp->obj_stack[interp->cur_obj_stack]->cur_object_slot;

    var = list_get_item(posargs);
    if (GET_TAG(var) != SYMBOL) goto error;

    res = hash_get_and_unset_t(h, var);
    if (NULL == res) {
	return const_Nil;
    } else {
	if (NULL == silent) {
	    return res;
	} else {
	    return const_Nil;
	}
    }

error:
    return new_exception(TE_SYNTAX, L"syntax error, syntax: unsets [:silent] var", interp);
}

Toy_Type*
cmd_self(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 0) goto error;

    return SELF(interp);

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: self", interp);
}

Toy_Type*
cmd_stacktrace(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Cell *stack;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 0) goto error;

    stack = new_cell(L"");
    return new_string_str(get_stack_trace(interp, stack));

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: stack-trace", interp);
}

static void
sig_func(int code) {
    sig_flag = (sig_atomic_t)code;
}

Toy_Type*
cmd_trap(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *sig;
    Toy_Type *block;
    wchar_t *psig;
    int isig;
    Toy_Type *trapdic;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 2) goto error;

    sig = list_get_item(posargs);
    if (GET_TAG(sig) != SYMBOL) goto error;

    psig = cell_get_addr(sig->u.symbol.cell);

    if (wcscmp(psig, L"SIGHUP") == 0) {
	isig = SIGHUP;
    } else if (wcscmp(psig, L"SIGINT") == 0) {
	isig = SIGINT;
    } else if (wcscmp(psig, L"SIGQUIT") == 0) {
	isig = SIGQUIT;
    } else if (wcscmp(psig, L"SIGPIPE") == 0) {
	isig = SIGPIPE;
    } else if (wcscmp(psig, L"SIGALRM") == 0) {
	isig = SIGALRM;
    } else if (wcscmp(psig, L"SIGTERM") == 0) {
	isig = SIGTERM;
    } else if (wcscmp(psig, L"SIGURG") == 0) {
	isig = SIGURG;
    } else if (wcscmp(psig, L"SIGCHLD") == 0) {
	isig = SIGCHLD;
    } else if (wcscmp(psig, L"SIGUSR1") == 0) {
	isig = SIGUSR1;
    } else if (wcscmp(psig, L"SIGUSR2") == 0) {
	isig = SIGUSR2;
    } else goto error;

    posargs = list_next(posargs);
    block = list_get_item(posargs);

    /* create new dict instance : XXX */
    trapdic = hash_get_t(interp->globals, const_attrap);
    if (NULL == trapdic) {
	trapdic = new_dict(new_hash());
	hash_set_t(interp->globals, const_attrap, trapdic);
    }

    if (block == NULL) {
	block = hash_get_and_unset_t(trapdic->u.dict, sig);
	signal(isig, SIG_DFL);

	if (NULL == block) return const_Nil;
	return block;
    }

    if (GET_TAG(block) != CLOSURE) goto error;

    if (NULL != hash_set_t(trapdic->u.dict, sig, block)) {

	signal(isig, sig_func);
    }

    return sig;

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: trap signal [{block}]\n\t\
signal: SIGHUP|SIGINT|SIGQUIT|SIGPIPE|SIGALRM|SIGTERM|SIGURG|SIGCHLD|SIGUSR1|SIGUSR2", interp);
}

Toy_Type*
cmd_not(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *o;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    o = list_get_item(posargs);

    if (GET_TAG(o) == NIL) {
	return const_T;
    } else {
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: ! object", interp);
}

Toy_Type*
cmd_and(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *o;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen == 0) goto error;

    while (posargs) {
	o = list_get_item(posargs);
	if (GET_TAG(o) == NIL) return const_Nil;
	posargs = list_next(posargs);
    }

    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: and object [object ...]", interp);
}

Toy_Type*
cmd_or(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *o;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen == 0) goto error;

    while (posargs) {
	o = list_get_item(posargs);
	if (GET_TAG(o) != NIL) return const_T;
	posargs = list_next(posargs);
    }

    return const_Nil;

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: or object [object ...]", interp);
}

Toy_Type*
cmd_load(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *cont;
    char *path;
    Toy_Type *tpath;
    Bulk *src;
    Toy_Type *script, *result;
    Hash *shash;
    Toy_Type *tid, *nid;
    int id;
    Toy_Script *sscript;
    Toy_Type *enc;
    int iencoder;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    iencoder = NENCODE_RAW;
    enc = hash_get_t(interp->globals, const_DEFAULT_SCRIPT_ENCODING);
    if (enc) {
	if (GET_TAG(enc) == SYMBOL) {
	    iencoder = get_encoding_index(cell_get_addr(enc->u.symbol.cell));
	    if (-1 == iencoder) {
		return new_exception(TE_BADENCODER, L"Bad encoder specified.", interp);
	    }
	} else {
	    return new_exception(TE_BADENCODER, L"Bad encoder specified, need symbol.", interp);
	}
    }

    tpath = list_get_item(posargs);
    if (GET_TAG(tpath) != STRING) goto error;
    path = to_char(to_string_call(interp, tpath));

    src = new_bulk();
    if (NULL == src) return const_Nil;

    if (0 == bulk_load_file(src, path, iencoder)) {
	return new_exception(TE_FILENOTOPEN, L"Script file not open.", interp);
    }

    script = toy_parse_start(src);
    if (GET_TAG(script) == EXCEPTION) {
	return script;
    }

    shash = interp->scripts;
    tid = hash_get_t(shash, const_atscriptid);
    id = mpz_get_si(tid->u.biginteger);
    id++;
    nid = new_integer_si(id);
    SET_SCRIPT_ID(script, id);

    sscript = GC_MALLOC(sizeof(struct _toy_script));
    ALLOC_SAFE(sscript);

    sscript->id = id;
    sscript->path = tpath;
    sscript->src = src;
    sscript->script = script;
    cont = new_container(sscript);

    hash_set_t(shash, const_atscriptid, nid);
    hash_set(shash, to_string(nid), cont);
    hash_set(shash, to_wchar(path), cont);

    result = toy_eval_script(interp, script);

    if (GET_TAG(result) != EXCEPTION) {
	return nid;
    }

    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: load \"path\"", interp);
}

Toy_Type*
cmd_sid(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *func, *name;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    name = list_get_item(posargs);
    if (GET_TAG(name) == FUNC) {
	return new_integer_si(GET_SCRIPT_ID(name->u.func.closure->u.closure.block_body));
    }

    if (GET_TAG(name) != SYMBOL) goto error;

    func = hash_get_t(interp->funcs, name);
    if (NULL == func) {
	return const_Nil;
    }
    if (GET_TAG(func) != FUNC) {
	return const_Nil;
    }
    return new_integer_si(GET_SCRIPT_ID(func->u.func.closure->u.closure.block_body));

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: sid func", interp);
}


Toy_Type*
cmd_sdir(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *h;
    Toy_Type *cont;
    Toy_Type *id;
    int i;
    Toy_Type *result, *resultl, *item, *iteml;
    Toy_Script *script;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    h = interp->scripts;
    id = hash_get_t(h, const_atscriptid);
    if (NULL == id) return const_Nil;

    resultl = result = new_list(NULL);
    for (i=1; i<=mpz_get_si(id->u.biginteger); i++) {
	cont = hash_get(h, to_string(new_integer_si(i)));
	if (NULL == cont) return const_Nil;

	script = (Toy_Script*)(cont->u.container);
	iteml = item = new_list(NULL);
	iteml = list_append(iteml, new_integer_si(script->id));
	iteml = list_append(iteml, script->path);

	resultl = list_append(resultl, item);
    }

    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: sdir", interp);
}

Toy_Type*
cmd_pwd(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    char *buff;
    Toy_Type *cwd;

    buff = GC_MALLOC(MAXPATHLEN);
    ALLOC_SAFE(buff);

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    if (NULL == getcwd(buff, MAXPATHLEN)) {
	return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
    }

    cwd = new_string_str(to_wchar(buff));
    hash_set_t(interp->globals, const_CWD, cwd);

    return cwd;

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: pwd", interp);
}

Toy_Type*
cmd_chdir(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *dir;
    int t;
    wchar_t *p;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    if (arglen == 0) {
	Toy_Type *l, *cmd;
	l = cmd = new_list(new_ref(L"ENV"));
	l = list_append(l, new_symbol(L"get"));
	l = list_append(l, new_string_str(L"HOME"));
	dir = toy_call(interp, cmd);
    } else {
	dir = list_get_item(posargs);
    }

    t = GET_TAG(dir);
    switch (t) {
    case STRING:
	p = cell_get_addr(dir->u.string);
	break;
    case SYMBOL:
	p = cell_get_addr(dir->u.symbol.cell);
	break;
    default:
	goto error;
    }

    if (0 == chdir(to_char(p))) {
	return cmd_pwd(interp, new_list(NULL), new_hash(), 0);
    }

    return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: cd [dir]", interp);
}

Toy_Type*
cmd_alias(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *up, *orig, *alias;
    int iup, s;

    if (arglen != 2) goto error;
    orig = list_get_item(posargs);
    posargs = list_next(posargs);
    alias = list_get_item(posargs);

    up = hash_get_and_unset_t(nameargs, const_up);
    if (hash_get_length(nameargs) > 0) goto error;

    if (NULL == up) {
	iup = 0;
    } else {
	if (GET_TAG(up) != INTEGER) goto error;
	iup = mpz_get_si(up->u.biginteger);
	if (iup < 0) goto error;
    }

    if ((interp->cur_func_stack - iup) < 0) {
	return new_exception(TE_STACKUNDERFLOW, L"Stack underflow.", interp);
    }

    s = hash_link_t(interp->func_stack[interp->cur_func_stack]->localvar, alias,
		    interp->func_stack[interp->cur_func_stack - iup]->localvar, orig);

    if (0 == s) {
	return new_exception(TE_ALIAS, L"Already exists variable or link looped.", interp);
    } else {
	return const_T;
    }

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: alias [up: upvar] orig-var alias",
			 interp);
}

Toy_Type*
cmd_sleep(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *msec;
    int imsec;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    msec = list_get_item(posargs);
    if (GET_TAG(msec) != INTEGER) goto error;

    imsec = mpz_get_si(msec->u.biginteger);

    usleep((useconds_t)(imsec * 1000));
    return new_integer_si(imsec);

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: sleep msec", interp);
}

Toy_Type*
cmd_file(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *command;
    wchar_t *commands;

    if (arglen == 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    command = list_get_item(posargs);
    if (GET_TAG(command) != SYMBOL) goto error;
    commands = cell_get_addr(command->u.symbol.cell);

    if (wcscmp(commands, L"?") == 0) {

	println(interp, L"file command [args ...]");
	println(interp, L"commands are:");
	println(interp, L"  ?                    Print this message.");
	println(interp, L"  exists? \"file\"       If \"file\" exist then return t.");
	println(interp, L"  dir? \"file\"          If \"file\" is directory then return t.");
	println(interp, L"  read? \"file\"         If \"file\" is readable then return t.");
	println(interp, L"  write? \"file\"        If \"file\" is writable then return t.");
	println(interp, L"  exec? \"file\"         If \"file\" is excecutable then return t.");
	println(interp, L"  list \"directory\"     Return \"directory\"\'s entry list.");
	println(interp, L"  stat \"file\"          Return \"file\" status.");
	println(interp, L"  rm \"file\"            rm \"file\".");
	println(interp, L"  rmdir \"dir\"          rmdir \"dir\".");
	println(interp, L"  rename \"from\" \"dest\" rename file \"from\" to \"dest\".");
	println(interp, L"  mkdir \"dir\"          make directory \"dir\" and mode.");
	println(interp, L"  chmod \"file\" mode    change \"file\" access mode.");

	return const_Nil;
	
    } else if (wcscmp(commands, L"exists?") == 0) {
	Toy_Type *fname;
	char *fnames;
	int status;
	struct stat sb;

	if (arglen != 2) goto error_exists;
	posargs = list_next(posargs);
	fname = list_get_item(posargs);
	if (GET_TAG(fname) != STRING) goto error_exists;
	fnames = to_char(cell_get_addr(fname->u.string));

	status = stat((const char*)fnames, &sb);

	if (0 != status) return const_Nil;
	return const_T;

    } else if (wcscmp(commands, L"dir?") == 0) {
	Toy_Type *fname;
	char *fnames;
	int status;
	struct stat sb;

	if (arglen != 2) goto error_dir;
	posargs = list_next(posargs);
	fname = list_get_item(posargs);
	if (GET_TAG(fname) != STRING) goto error_dir;
	fnames = to_char(cell_get_addr(fname->u.string));

	status = stat((const char*)fnames, &sb);

	if (0 != status) return const_Nil;

	if (S_ISDIR(sb.st_mode)) {
	    return const_T;
	}

	return const_Nil;

    } else if (wcscmp(commands, L"read?") == 0) {
	Toy_Type *fname;
	char *fnames;
	int status;

	if (arglen != 2) goto error_read;
	posargs = list_next(posargs);
	fname = list_get_item(posargs);
	if (GET_TAG(fname) != STRING) goto error_read;
	fnames = to_char(cell_get_addr(fname->u.string));

	status = access((const char*)fnames, R_OK);

	if (0 != status) return const_Nil;

	return const_T;

    } else if (wcscmp(commands, L"write?") == 0) {
	Toy_Type *fname;
	char *fnames;
	int status;

	if (arglen != 2) goto error_write;
	posargs = list_next(posargs);
	fname = list_get_item(posargs);
	if (GET_TAG(fname) != STRING) goto error_write;
	fnames = to_char(cell_get_addr(fname->u.string));

	status = access((const char*)fnames, W_OK);

	if (0 != status) return const_Nil;

	return const_T;

    } else if (wcscmp(commands, L"exec?") == 0) {
	Toy_Type *fname;
	char *fnames;
	int status;

	if (arglen != 2) goto error_exec;
	posargs = list_next(posargs);
	fname = list_get_item(posargs);
	if (GET_TAG(fname) != STRING) goto error_exec;
	fnames = to_char(cell_get_addr(fname->u.string));

	status = access((const char*)fnames, X_OK);

	if (0 != status) return const_Nil;

	return const_T;

    } else if (wcscmp(commands, L"list") == 0) {
	Toy_Type *fname;
	char *fnames;
	DIR *dir;
	struct dirent *dirent;
	Toy_Type *dirl, *l;

	if ((arglen != 1) && (arglen != 2)) goto error_list;
	if (arglen == 2) {
	    posargs = list_next(posargs);
	    fname = list_get_item(posargs);
	    if (GET_TAG(fname) != STRING) goto error_list;
	    fnames = to_char(cell_get_addr(fname->u.string));
	} else {
	    fnames = ".";
	}

	dir = opendir(fnames);
	if (NULL == dir) {
	    return new_exception(TE_FILEACCESS, L"Directory not open.", interp);
	}

	l = dirl = new_list(NULL);
	
	dirent = readdir(dir);
	while (dirent) {
	    l = list_append(l, new_string_str(to_wchar(dirent->d_name)));

	    dirent = readdir(dir);
	}

	closedir(dir);
	return dirl;

    } else if (wcscmp(commands, L"stat") == 0) {
	Toy_Type *fname;
	char *fnames;
	int sts;
	struct stat fstat;
	Toy_Type *result, *l;
	wchar_t *t;

	if (arglen != 2) goto error_stat;
	posargs = list_next(posargs);
	fname = list_get_item(posargs);
	if (GET_TAG(fname) != STRING) goto error_stat;
	fnames = to_char(cell_get_addr(fname->u.string));

	sts = stat(fnames, &fstat);
	if (-1 == sts) {
	    return new_exception(TE_FILEACCESS, L"file stat can\'t get.", interp);
	}
	l = result = new_list(NULL);
	
	l = list_append(l, new_cons(new_symbol(L"dev-major"), 
				    new_integer_si(major(fstat.st_dev))));
	l = list_append(l, new_cons(new_symbol(L"dev-mainor"), 
				    new_integer_si(minor(fstat.st_dev))));
	l = list_append(l, new_cons(new_symbol(L"inode"), 
				    new_integer_si(fstat.st_ino)));
	l = list_append(l, new_cons(new_symbol(L"mode"), 
				    new_integer_si(fstat.st_mode)));
	l = list_append(l, new_cons(new_symbol(L"link"), 
				    new_integer_si(fstat.st_nlink)));
	l = list_append(l, new_cons(new_symbol(L"uid"), 
				    new_integer_si(fstat.st_uid)));
	l = list_append(l, new_cons(new_symbol(L"gid"), 
				    new_integer_si(fstat.st_gid)));
	l = list_append(l, new_cons(new_symbol(L"device"), 
				    new_integer_si(fstat.st_rdev)));
	l = list_append(l, new_cons(new_symbol(L"size"), 
				    new_integer_si(fstat.st_size)));
	l = list_append(l, new_cons(new_symbol(L"block-size"), 
				    new_integer_si(fstat.st_blksize)));
	l = list_append(l, new_cons(new_symbol(L"blocks"), 
				    new_integer_si(fstat.st_blocks)));
	l = list_append(l, new_cons(new_symbol(L"atime"), 
				    new_integer_ullsi(fstat.st_atim.tv_sec)));
	l = list_append(l, new_cons(new_symbol(L"mtime"), 
				    new_integer_ullsi(fstat.st_mtim.tv_sec)));
	l = list_append(l, new_cons(new_symbol(L"ctime"), 
				    new_integer_ullsi(fstat.st_ctim.tv_sec)));

	if ((fstat.st_mode & S_IFMT) == S_IFSOCK) {
	    t = L"s";
	} else if ((fstat.st_mode & S_IFMT) == S_IFLNK) {
	    t = L"l";
	} else if ((fstat.st_mode & S_IFMT) == S_IFREG) {
	    t = L"-";
	} else if ((fstat.st_mode & S_IFMT) == S_IFBLK) {
	    t = L"b";
	} else if ((fstat.st_mode & S_IFMT) == S_IFDIR) {
	    t = L"d";
	} else if ((fstat.st_mode & S_IFMT) == S_IFCHR) {
	    t = L"c";
	} else if ((fstat.st_mode & S_IFMT) == S_IFIFO) {
	    t = L"p";
	} else {
	    t = L"?";
	}
	l = list_append(l, new_cons(new_symbol(L"type"),
				    new_string_str(t)));
	l = list_append(l, new_cons(new_symbol(L"perm"),
				    new_integer_si(fstat.st_mode & 07777)));
					       
	return result;

    } else if (wcscmp(commands, L"rm") == 0) {
	Toy_Type *fname;
	char *fnames;
	
	if (arglen != 2) goto error_delete;
	posargs = list_next(posargs);
	fname = list_get_item(posargs);
	if (GET_TAG(fname) != STRING) goto error_delete;
	fnames = to_char(cell_get_addr(fname->u.string));

	if (-1 == unlink(fnames)) {
	    return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
	}

	return const_T;
	
    } else if (wcscmp(commands, L"rmdir") == 0) {
	Toy_Type *fname;
	char *fnames;
	
	if (arglen != 2) goto error_deldir;
	posargs = list_next(posargs);
	fname = list_get_item(posargs);
	if (GET_TAG(fname) != STRING) goto error_deldir;
	fnames = to_char(cell_get_addr(fname->u.string));

	if (-1 == rmdir(fnames)) {
	    return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
	}

	return const_T;
	
    } else if (wcscmp(commands, L"rename") == 0) {
	Toy_Type *fname, *dname;
	char *fnames, *dnames;
	
	if (arglen != 3) goto error_move;
	posargs = list_next(posargs);
	fname = list_get_item(posargs);
	posargs = list_next(posargs);
	dname = list_get_item(posargs);
	if (GET_TAG(fname) != STRING) goto error_move;
	if (GET_TAG(dname) != STRING) goto error_move;
	fnames = to_char(cell_get_addr(fname->u.string));
	dnames = to_char(cell_get_addr(dname->u.string));

	if (-1 == rename(fnames, dnames)) {
	    return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
	}

	return const_T;

    } else if (wcscmp(commands, L"mkdir") == 0) {
	Toy_Type *dirname, *mode;
	char *dirnames;
	int imode;
	
	if (arglen != 3) goto error_mkdir;
	posargs = list_next(posargs);
	dirname = list_get_item(posargs);
	if (GET_TAG(dirname) != STRING) goto error_mkdir;
	dirnames = to_char(cell_get_addr(dirname->u.string));
	posargs = list_next(posargs);
	mode = list_get_item(posargs);
	if (GET_TAG(mode) != INTEGER) goto error_mkdir;
	imode = mpz_get_si(mode->u.biginteger);

	if (-1 == mkdir(dirnames, imode)) {
	    return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
	}

	return const_T;
	
    } else if (wcscmp(commands, L"chmod") == 0) {
	Toy_Type *fname, *mode;
	char *fnames;
	int modei;
	
	if (arglen != 3) goto error_chmod;
	posargs = list_next(posargs);
	fname = list_get_item(posargs);
	posargs = list_next(posargs);
	mode = list_get_item(posargs);
	if (GET_TAG(fname) != STRING) goto error_chmod;
	if (GET_TAG(mode) != INTEGER) goto error_chmod;
	fnames = to_char(cell_get_addr(fname->u.string));
	modei = mpz_get_ui(mode->u.biginteger);

	if (-1 == chmod(fnames, modei)) {
	    return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
	}

	return const_T;

    }
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: file command (or for help ?) [args ...]", interp);
error_exists:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: file exists? \"file\"", interp);
error_dir:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: file dir? \"file\"", interp);
error_read:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: file read? \"file\"", interp);
error_write:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: file write? \"file\"", interp);
error_exec:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: file exec? \"file\"", interp);
error_list:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: file list \"directory\"", interp);
error_stat:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: file stat \"file\"", interp);
error_delete:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: file rm \"file\"", interp);
error_deldir:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: file rmdir \"dir\"", interp);
error_move:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: file rename \"from\" \"dest\"", interp);
error_mkdir:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: file mkdir \"dir\" mode", interp);
error_chmod:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: file chmod \"file\" mode", interp);
}

Toy_Type*
cmd_exists(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *sym, *val;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    sym = list_get_item(posargs);
    if (GET_TAG(sym) != SYMBOL) goto error;

    val = toy_resolv_var(interp, sym, 0, interp->trace_info);
    if (GET_TAG(val) == EXCEPTION) return const_Nil;

    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: exists? symbol", interp);
}

Toy_Type*
cmd_isdefvar(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *var;
    Hash *h;
    Toy_Type *res;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    var = list_get_item(posargs);
    if (GET_TAG(var) != SYMBOL) goto error;

    h = interp->globals;

    res = hash_get_t(h, var);
    if (NULL == res) {
	return const_Nil;
    }

    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: defvar? var", interp);
}

Toy_Type*
cmd_istype(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    return new_symbol(toy_get_type_str(list_get_item(posargs)));

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: type? var", interp);
}

Toy_Type*
cmd_call(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *cmd, *eval, *l, *result;

    if (arglen != 2) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    cmd = list_get_item(posargs);

    posargs = list_next(posargs);

    l = eval = new_list(cmd);
    if (GET_TAG(posargs) == LIST) {
	posargs = list_get_item(posargs);
	while (! IS_LIST_NULL(posargs)) {
	    l = list_append(l, list_get_item(posargs));
	    posargs = list_next(posargs);
	}
    } else {
	if (posargs != NULL) goto error;
    }

    result = toy_call(interp, eval);

    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: call func (args ...) | object (method args ...)", interp);
}

Toy_Type*
cmd_rand(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    static unsigned long int s = 0;
    unsigned long int r;

    if (0 == s) {
	struct timeval t;
	gettimeofday(&t, NULL);
	s = t.tv_sec + t.tv_usec;
	srandom(s);
    }
    r = random();
    return new_real((double)r / (double)0x7fffffff);
}

Toy_Type*
cmd_symbol(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *s;

    if (arglen != 1) goto error;

    s = list_get_item(posargs);
    if (GET_TAG(s) != STRING) goto error;

    return new_symbol(cell_get_addr(s->u.string));

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: symbol-string", interp);
}


Toy_Type*
cmd_trace(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *level, *fd, *result;
    int ilevel;

    if (arglen > 1) goto error;

    fd = hash_get_and_unset_t(nameargs, const_out);
    if (hash_get_length(nameargs) > 0) goto error;
    if (NULL != fd) {
	if (GET_TAG(fd) != INTEGER) goto error;
	interp->trace_fd = mpz_get_si(fd->u.biginteger);
    }

    if (arglen == 0) return new_integer_si(interp->trace);

    level = list_get_item(posargs);
    if (GET_TAG(level) != INTEGER) {
	if (GET_TAG(level) == CLOSURE) {
	    int otrace;
	    otrace = interp->trace;
	    interp->trace = 1;
	    result = eval_closure(interp, level, interp->trace_info);
	    interp->trace = otrace;
	    return result;
	} else {
	    goto error;
	}
    }

    ilevel = mpz_get_si(level->u.biginteger);
    interp->trace = ilevel;

    return new_integer_si(ilevel);

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: trace level | {block} [out: fd]", interp);
}

Toy_Type*
cmd_exit(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *code;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 1) goto error;

    if (arglen == 1) {
	code = list_get_item(posargs);
	if (GET_TAG(code) != INTEGER) goto error;
	exit(mpz_get_si(code->u.biginteger));
    }
    fflush(stdout);
    fflush(stderr);
    exit(0);

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: exit [status]", interp);
}

Toy_Type*
cmd_fork(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    int pid;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 0) goto error;

    pid = fork();
    if (-1 == pid) {
	return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
    }

    return new_integer_si(pid);

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: fork", interp);
}

Toy_Type*
cmd_yield(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *closure;

    if (arglen < 1) goto error;
    if (hash_get_length(nameargs) > 1) goto error;

    closure = list_get_item(posargs);
    posargs = list_next(posargs);

    if (GET_TAG(closure) != CLOSURE) goto error;

    return toy_yield(interp, closure, posargs);

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: yield {| var | block } [ args ... ]", interp);
}

Toy_Type*
cmd_result(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *t;

    if (hash_get_length(nameargs) == 1) {
	if (NULL == (t = hash_get_and_unset_t(nameargs, const_last))) goto error;
	if (NIL == GET_TAG(t)) return const_Nil;
	return interp->last_status;
    } else if (hash_get_length(nameargs) > 1) {
	goto error;
    }

    if (arglen == 0) return const_Nil;
    if (arglen != 1) goto error;
    return list_get_item(posargs);

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: result [value | :last]", interp);
}

Toy_Type*
cmd_lazy(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *closure;

    if (hash_get_length(nameargs) != 0) goto error;
    if (arglen != 1) goto error;
    closure = list_get_item(posargs);
    if (GET_TAG(closure) != CLOSURE) goto error;
    SET_LAZY(closure);
    return closure;

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: lazy {closure}", interp);
}

#ifdef HAS_GCACHE
Toy_Type*
cmd_cacheinfo(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *l;
    l = new_list(NULL);
    list_append(l, new_integer_si(hash_get_length(interp->gcache)));
    list_append(l, new_integer_si(interp->cache_hit));
    list_append(l, new_integer_si(interp->cache_missing));

    return l;
}
#endif

Toy_Type*
cmd_begin(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *body, *result, *local;
    Hash *local_hash;
    Toy_Func_Env *closure_env;

    if (arglen != 1) goto error;

    body = list_get_item(posargs);
    if (GET_TAG(body) != CLOSURE) goto error;

    local = hash_get_and_unset_t(nameargs, const_local);
    if (NULL == local) {
	local_hash = new_hash();
    } else {
	if (GET_TAG(local) != DICT) goto error;
	local_hash = local->u.dict;
    }

    if (hash_get_and_unset_t(nameargs, const_rebase)) {
	closure_env = NULL;
    } else {
	closure_env = body->u.closure.env->func_env;
    }

    if (hash_get_length(nameargs) != 0) goto error;

    if (0 ==
	toy_push_func_env(interp, local_hash, closure_env,
			  interp->func_stack[interp->cur_func_stack]->tobe_bind_val, interp->trace_info)) {

	return new_exception(TE_STACKOVERFLOW, L"Function satck overflow.", interp);
    }

    result = toy_eval_script(interp, body->u.closure.block_body);

    toy_pop_func_env(interp);

    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: begin {block} [local: hash-object] [:rebase]", interp);
}

Toy_Type*
cmd_forkandexec(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    wchar_t *command;
    char **argv;
    int i, pid;
    int left_ch[2], right_ch[2], err_ch[2];
    Toy_Type *result;
    int flag;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen < 1) goto error;

    command  = to_string_call(interp, list_get_item(posargs));

    argv = GC_MALLOC(sizeof(char*) * (arglen+1));
    ALLOC_SAFE(argv);

    posargs = list_next(posargs);
    if ((GET_TAG(list_get_item(posargs)) == LIST) && (list_length(posargs) == 1)) {
	posargs = list_get_item(posargs);
	arglen = list_length(posargs) + 1;
    }
    
    argv[0] = to_char(command);
    for (i = 1; i<arglen; i++) {
	argv[i] = to_char(to_string_call(interp, list_get_item(posargs)));
	posargs = list_next(posargs);
    }
    argv[i] = NULL;

    if (-1 == pipe(left_ch)) {
	return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
    }
    if (-1 == pipe(right_ch)) {
	return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
    }
    if (-1 == pipe(err_ch)) {
	return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
    }
    
    pid = fork();
    
    if (-1 == pid) {
	return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
    }

    if (0 == pid) {
	/* I am a child */
	close(left_ch[1]);
	close(right_ch[0]);
	close(err_ch[0]);

	close(0);
	dup(left_ch[0]);
	close(left_ch[0]);

	close(1);
	dup(right_ch[1]);
	close(right_ch[1]);

	close(2);
	dup(err_ch[1]);
	close(err_ch[1]);

	execvp(to_char(command), argv);
	exit(255);
    }

    /* I am a parent */
    close(left_ch[0]);
    close(right_ch[1]);
    close(err_ch[1]);

    flag = fcntl(left_ch[1], F_GETFD, 0);
    if (flag >= 0) {
	flag |= FD_CLOEXEC;
	fcntl(left_ch[1], F_SETFD, flag);
    }

    flag = fcntl(right_ch[0], F_GETFD, 0);
    if (flag >= 0) {
	flag |= FD_CLOEXEC;
	fcntl(right_ch[0], F_SETFD, flag);
    }

    flag = fcntl(err_ch[0], F_GETFD, 0);
    if (flag >= 0) {
	flag |= FD_CLOEXEC;
	fcntl(err_ch[0], F_SETFD, flag);
    }

    result = new_list(NULL);
    list_append(result, new_cons(new_symbol(L"pid"), new_integer_si(pid)));
    list_append(result, new_cons(new_symbol(L"left"), new_integer_si(left_ch[1])));
    list_append(result, new_cons(new_symbol(L"right"), new_integer_si(right_ch[0])));
    list_append(result, new_cons(new_symbol(L"error"), new_integer_si(err_ch[0])));
    
    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: fork-exec command [arg ...] | [(arg ...)]", interp);
}

Toy_Type*
cmd_wait(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    int wsts, pid, ssts;
    Toy_Type *tpid;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    
    tpid = list_get_item(posargs);
    if (GET_TAG(tpid) != INTEGER) goto error;

    pid = mpz_get_si(tpid->u.biginteger);
    ssts = waitpid(pid, &wsts, 0);
    if (-1 == ssts) {
	return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
    }
	
    return new_integer_si(WEXITSTATUS(wsts));

error:
    return new_exception(TE_SYNTAX, L"Syntax error, syntax: wait pid", interp);
}


Toy_Type*
cmd_read(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *l, *body;
    Toy_Type *file, *result;
    Toy_Type *var;
    int flag_nonewline=0, flag_nocontrol=0;

    file = hash_get_and_unset_t(nameargs, const_filec);
    if (hash_get_and_unset_t(nameargs, const_nonewline)) {
	flag_nonewline = 1;
    }
    if (hash_get_and_unset_t(nameargs, const_nocontrol)) {
	flag_nocontrol = 1;
    }
    if (hash_get_length(nameargs) > 0) goto error;

    if (arglen > 1) goto error;
    if (arglen == 1) {
	var = list_get_item(posargs);
	if (GET_TAG(var) != SYMBOL) goto error;
    } else {
	var = NULL;
    }

    if (NULL == file) {
	file = toy_resolv_var(interp, const_atin, 0, interp->trace_info);
	if (GET_TAG(file) == EXCEPTION) {
	    file = toy_resolv_var(interp, const_stdin, 0, interp->trace_info);
	    if (GET_TAG(file) == EXCEPTION) {
		return new_exception(TE_NOVAR,
				     L"No such file object '@in' and 'stdin'.", interp);
	    }
	}
    }
    
    l = body = new_list(file);
    l = list_append(l, const_gets);
    if (flag_nonewline) {
	l = list_append(l, new_symbol(L":nonewline"));
    }
    if (flag_nocontrol) {
	l = list_append(l, new_symbol(L":nocontrol"));
    }

    result = toy_call(interp, body);

    if (var) {
	Hash *h;
	h = interp->func_stack[interp->cur_func_stack]->localvar;
	hash_set_t(h, var, result);
    }

    return result;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'read', syntax: read [var] [file: file-object] [:nonewline] [:nocontrol]", interp);
}

Toy_Type*
cmd_newdict(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *hash_list;
    Hash *hash;
    Toy_Type *item, *value;
    wchar_t *key;

    if (arglen == 0) {
	return new_dict(new_hash());
    }

    if (arglen > 1) goto error;
    hash_list = list_get_item(posargs);
    if (GET_TAG(hash_list) != LIST) goto error;

    hash = new_hash();
    while (! IS_LIST_NULL(hash_list)) {
	item = list_get_item(hash_list);
	if (GET_TAG(item) != LIST) goto error;
	if (NULL != list_get_item(item)) {
	    key = to_string_call(interp, list_get_item(item));
	    value = list_next(item);
	    if (NULL == value) value = const_Nil;
	    hash_set(hash, key, value);
	}

	hash_list = list_next(hash_list);
    }

    return new_dict(hash);

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'dict', syntax: dict [((key . val) ...)]", interp);
}

Toy_Type*
cmd_goto(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *ret_val;

    if (arglen < 1) goto error;
    if (hash_get_length(nameargs) > 1) goto error;

    ret_val = new_list(new_dict(interp->func_stack[interp->cur_func_stack]->localvar));
    list_append(ret_val, posargs);

    return new_control(CTRL_GOTO, ret_val);

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'goto', syntax: goto fun [val ...]", interp);
}

Toy_Type*
cmd_localdict(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    return new_dict(interp->func_stack[interp->cur_func_stack]->localvar);
}

Toy_Type*
cmd_objdict(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    return new_dict(interp->obj_stack[interp->cur_obj_stack]->cur_object_slot);
}

Toy_Type*
cmd_globaldict(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    return new_dict(interp->globals);
}

Toy_Type*
cmd_funcdict(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    return new_dict(interp->funcs);
}

Toy_Type*
cmd_classdict(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    return new_dict(interp->classes);
}

Toy_Type*
cmd_closuredict(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *closure;
    if (list_length(posargs) != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;

    closure = list_get_item(posargs);
    if (GET_TAG(closure) != CLOSURE) goto error;
    return new_dict(closure->u.closure.env->func_env->localvar);

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'dict-closure', syntax: dict-closure {block}", interp);
}

Toy_Type*
cmd_newvector(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *array_list;
    Array *array;
    Toy_Type *item;

    if (arglen == 0) {
	return new_vector(new_array());
    }

    if (arglen > 1) goto error;
    array_list = list_get_item(posargs);
    if (GET_TAG(array_list) != LIST) goto error;

    array = new_array();
    while (! IS_LIST_NULL(array_list)) {
	item = list_get_item(array_list);
	array_append(array, (item==NULL)?const_Nil:item);

	array_list = list_next(array_list);
    }

    return new_vector(array);

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'vector', syntax: vector [(val ...)]", interp);
}

Toy_Type*
cmd_equal(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *a, *b;
    int ta, tb;

    if (arglen != 2) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    a = list_get_item(posargs);
    posargs = list_next(posargs);
    b = list_get_item(posargs);

    ta = GET_TAG(a);
    tb = GET_TAG(b);

    switch (ta) {
    case NIL:
	if (tb == NIL) return const_T;
	return const_Nil;
	break;

    case SYMBOL:
	if (tb != SYMBOL) return const_Nil;
	if (wcscmp(cell_get_addr(a->u.symbol.cell), cell_get_addr(b->u.symbol.cell)) == 0)
	    return const_T;
	return const_Nil;
	break;

    case LIST:
	if (tb != LIST) return const_Nil;
	if (a == b) return const_T;
	return const_Nil;
	break;

    case INTEGER:
	if (tb != INTEGER) return const_Nil;
	if (0 == (mpz_cmp(a->u.biginteger,
			  b->u.biginteger))) return const_T;
	return const_Nil;
	break;

    case REAL:
	if (tb != REAL) return const_Nil;
	if (a->u.real == b->u.real) return const_T;
	return const_Nil;
	break;

    case STRING:
	if (tb != STRING) return const_Nil;
	if (wcscmp(cell_get_addr(a->u.string), cell_get_addr(b->u.string)) == 0)
	    return const_T;
	return const_Nil;
	break;

    case SCRIPT:
	if (tb != SCRIPT) return const_Nil;
	if (a == b) return const_T;
	return const_Nil;
	break;

    case STATEMENT:
	if (tb != STATEMENT) return const_Nil;
	if (a == b) return const_T;
	return const_Nil;
	break;

    case NATIVE:
	if (tb != NATIVE) return const_Nil;
	if (a == b) return const_T;
	return const_Nil;
	break;

    case OBJECT:
	if (tb != OBJECT) return const_Nil;
	if (a == b) return const_T;
	return const_Nil;
	break;

    case EXCEPTION:
	if (tb != EXCEPTION) return const_Nil;
	if (a == b) return const_T;
	return const_Nil;
	break;

    case CLOSURE:
	if (tb != CLOSURE) return const_Nil;
	if (a == b) return const_T;
	return const_Nil;
	break;

    case FUNC:
	if (tb != FUNC) return const_Nil;
	if (a == b) return const_T;
	return const_Nil;
	break;

    case CONTROL:
	if (tb != CONTROL) return const_Nil;
	if (a == b) return const_T;
	return const_Nil;
	break;

    case CONTAINER:
	if (tb != CONTAINER) return const_Nil;
	if (a == b) return const_T;
	return const_Nil;
	break;

    case RQUOTE:
	if (tb != RQUOTE) return const_Nil;
	if (wcscmp(cell_get_addr(a->u.rquote), cell_get_addr(b->u.rquote)) == 0)
	    return const_T;
	return const_Nil;
	break;

    case BIND:
	if (tb != BIND) return const_Nil;
	if (a == b) return const_T;
	return const_Nil;
	break;

    case DICT:
	if (tb != DICT) return const_Nil;
	if (a == b) return const_T;
	return const_Nil;
	break;

    case VECTOR:
	if (tb != VECTOR) return const_Nil;
	if (a == b) return const_T;
	return const_Nil;
	break;

    default:
	break;
    }

    return const_Nil;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'eq?', syntax: eq? val1 val2", interp);
}

Toy_Type*
cmd_connect(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    int socket_fd, sts;
    struct sockaddr_in serv_addr_in;
    Toy_Type *tport, *thostaddr;
    Toy_Type *tbindport, *tbindhostaddr;
    unsigned int port, hostaddr;
    unsigned int bindport=0, bindhostaddr=0;

    if (arglen != 2) goto error;

    tbindport = hash_get_and_unset_t(nameargs, new_symbol(L"bind-port:"));
    if (tbindport) {
	if (GET_TAG(tbindport) != INTEGER) goto error;
	bindport = mpz_get_si(tbindport->u.biginteger);
    }

    tbindhostaddr = hash_get_and_unset_t(nameargs, new_symbol(L"bind-address:"));
    if (tbindhostaddr) {
	if (GET_TAG(tbindhostaddr) != INTEGER) goto error;
	bindhostaddr = mpz_get_si(tbindhostaddr->u.biginteger);
    }

    if (hash_get_length(nameargs) > 0) goto error;

    thostaddr = list_get_item(posargs);
    if (GET_TAG(thostaddr) != INTEGER) goto error;
    hostaddr = mpz_get_si(thostaddr->u.biginteger);

    posargs = list_next(posargs);
    tport = list_get_item(posargs);
    if (GET_TAG(tport) != INTEGER) goto error;
    port = mpz_get_si(tport->u.biginteger);

    socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    
    if (-1 == socket_fd) {
	return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
    }

    if ((bindport != 0) || (bindhostaddr != 0)) {
	struct sockaddr_in bind_addr_in;

	bind_addr_in.sin_family = AF_INET;
	bind_addr_in.sin_port = htons((uint16_t)bindport);
	bind_addr_in.sin_addr.s_addr = htonl((uint32_t)bindhostaddr);
	sts = bind(socket_fd, (const struct sockaddr*)&bind_addr_in, sizeof(bind_addr_in));
	if (-1 == sts) {
	    return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
	}
    }

    serv_addr_in.sin_family = AF_INET;
    serv_addr_in.sin_port = htons((uint16_t)port);
    serv_addr_in.sin_addr.s_addr = htonl((uint32_t)hostaddr);
    
    sts = connect(socket_fd, (const struct sockaddr*)&serv_addr_in, sizeof(serv_addr_in));
    if (-1 == sts) {
	return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
    }

    return new_integer_si(socket_fd);

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'connect', syntax: connect hostaddr port [bind-port: port] [bind-address: address]", interp);
}

Toy_Type*
cmd_socket_server(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    int socket_fd, sts;
    struct sockaddr_in serv_addr_in;
    Toy_Type *tport, *thostaddr;
    unsigned int port, hostaddr=0;
    int val;

    if (arglen != 1) goto error;

    thostaddr = hash_get_and_unset_t(nameargs, new_symbol(L"bind-address:"));
    if (thostaddr) {
	if (GET_TAG(thostaddr) != INTEGER) goto error;
	hostaddr = mpz_get_si(thostaddr->u.biginteger);
    };

    if (hash_get_length(nameargs) > 0) goto error;

    tport = list_get_item(posargs);
    if (GET_TAG(tport) != INTEGER) goto error;
    port = mpz_get_si(tport->u.biginteger);

    socket_fd = socket(PF_INET, SOCK_STREAM, 0);

    if (-1 == socket_fd) {
	return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
    }

    val = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    serv_addr_in.sin_family = AF_INET;
    serv_addr_in.sin_port = htons((uint16_t)port);
    serv_addr_in.sin_addr.s_addr = htonl((uint32_t)hostaddr);
    sts = bind(socket_fd, (const struct sockaddr*)&serv_addr_in, sizeof(serv_addr_in));
    if (-1 == sts) {
	return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
    }

    sts = listen(socket_fd, SOMAXCONN);
    if (-1 == sts) {
	return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
    }

    return new_integer_si(socket_fd);
    
error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'socket-server', syntax: socket-server port [bind-address: address]", interp);
}

Toy_Type*
cmd_select(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    fd_set read_fds, write_fds, except_fds;
    Toy_Type *read_l, *write_l, *except_l, *timeout_v, *tfd;
    int maxfd = 0, fd;
    int timeout_l;
    struct timeval timeout;
    int result;
    int i;
    Toy_Type *read_result_l, *write_result_l, *except_result_l, *l, *result_l;;

    if (list_length(posargs) != 4) goto error;
    if (hash_get_length(nameargs) != 0) goto error;

    read_l = list_get_item(posargs);
    posargs = list_next(posargs);
    write_l = list_get_item(posargs);
    posargs = list_next(posargs);
    except_l = list_get_item(posargs);
    posargs = list_next(posargs);
    timeout_v = list_get_item(posargs);

    if (GET_TAG(read_l) != LIST) goto error;
    if (GET_TAG(write_l) != LIST) goto error;
    if (GET_TAG(except_l) != LIST) goto error;
    if ((GET_TAG(timeout_v) != INTEGER) && (GET_TAG(timeout_v) != NIL)) goto error;

    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&except_fds);

    while (! IS_LIST_NULL(read_l)) {
	tfd = list_get_item(read_l);
	if (GET_TAG(tfd) != INTEGER) goto error;
	fd = mpz_get_si(tfd->u.biginteger);
	FD_SET(fd, &read_fds);
	if (fd > maxfd) maxfd = fd;
	read_l = list_next(read_l);
    }

    while (! IS_LIST_NULL(write_l)) {
	tfd = list_get_item(write_l);
	if (GET_TAG(tfd) != INTEGER) goto error;
	fd = mpz_get_si(tfd->u.biginteger);
	FD_SET(fd, &write_fds);
	if (fd > maxfd) maxfd = fd;
	write_l = list_next(write_l);
    }

    while (! IS_LIST_NULL(except_l)) {
	tfd = list_get_item(except_l);
	if (GET_TAG(tfd) != INTEGER) goto error;
	fd = mpz_get_si(tfd->u.biginteger);
	FD_SET(fd, &except_fds);
	if (fd > maxfd) maxfd = fd;
	except_l = list_next(except_l);
    }

    maxfd ++;

    if (GET_TAG(timeout_v) == INTEGER) {
	timeout_l = mpz_get_si(timeout_v->u.biginteger);
	timeout.tv_sec = timeout_l / 1000;
	timeout.tv_usec = (timeout_l % 1000) * 1000;

	result = select(maxfd, &read_fds, &write_fds, &except_fds, &timeout);
    } else {
	result = select(maxfd, &read_fds, &write_fds, &except_fds, NULL);
    }

    if (-1 == result) {
	return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
    }

    l = read_result_l = new_list(NULL);
    for (i=0; i<maxfd; i++) {
	if (FD_ISSET(i, &read_fds)) {
	    l = list_append(l, new_integer_si(i));
	}
    }

    l = write_result_l = new_list(NULL);
    for (i=0; i<maxfd; i++) {
	if (FD_ISSET(i, &write_fds)) {
	    l = list_append(l, new_integer_si(i));
	}
    }

    l = except_result_l = new_list(NULL);
    for (i=0; i<maxfd; i++) {
	if (FD_ISSET(i, &except_fds)) {
	    l = list_append(l, new_integer_si(i));
	}
    }

    result_l = new_list(NULL);
    list_append(result_l, read_result_l);
    list_append(result_l, write_result_l);
    list_append(result_l, except_result_l);

    return result_l;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'select', syntax: select (read-fds ....) (write-fds ...) (except-fds ...) timeout\n\
\tread-fds ..... check for ready to read file descriptors list.\n\
\twrite-fds .... check for ready to write file descriptors list.\n\
\texcept-fds ... check for ready to except file descriptors list.\n\
\ttimeout ...... return timeout(milli-second), 0: never wait, nil: wait until some file descritors are ready.", interp);
}

Toy_Type*
cmd_accept(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *tfd;
    int fd;
    struct sockaddr_in client_addr_in;
    socklen_t client_addr_in_size;
    int sts;
    Toy_Type *result;

    if (list_length(posargs) != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;
    tfd = list_get_item(posargs);
    if (GET_TAG(tfd) != INTEGER) goto error;
    fd = mpz_get_si(tfd->u.biginteger);

    client_addr_in_size = (socklen_t)sizeof(client_addr_in);
    sts = accept(fd, (struct sockaddr*)&client_addr_in, &client_addr_in_size);
    if (-1 == sts) {
	return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
    }

    result = new_list(NULL);
    list_append(result, new_integer_si(sts));
    list_append(result, new_integer_si(ntohl(client_addr_in.sin_addr.s_addr)));
    list_append(result, new_integer_si((int)ntohs(client_addr_in.sin_port)));

    return result;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'accept', syntax: accept server-socket-fd", interp);
}

Toy_Type*
cmd_close(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *tfd;
    int fd;
    int sts;

    if (list_length(posargs) != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;
    tfd = list_get_item(posargs);
    if (GET_TAG(tfd) != INTEGER) goto error;
    fd = mpz_get_si(tfd->u.biginteger);

    sts = close(fd);
    if (-1 == sts) {
	return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
    }

    return new_integer_si(sts);
    
error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'close', syntax: close file-descriptor", interp);
}

Toy_Type*
cmd_resolv_in_addr(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *hostname;
    struct hostent *host_ent;
    wchar_t buff[16];

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    hostname = list_get_item(posargs);
    if (GET_TAG(hostname) != STRING) goto error;

    host_ent = gethostbyname(to_char(cell_get_addr(hostname->u.string)));
    endhostent();

    if (NULL == host_ent) {
	Cell *m;
	m = new_cell(L"No such host, \"");
	cell_add_str(m, cell_get_addr(hostname->u.string));
	cell_add_str(m, L"\".");
	return new_exception(TE_NOHOST, cell_get_addr(m), interp);
    }

    swprintf(buff, 16, L"%d.%d.%d.%d",
	     (unsigned char)host_ent->h_addr_list[0][0],
	     (unsigned char)host_ent->h_addr_list[0][1],
	     (unsigned char)host_ent->h_addr_list[0][2],
	     (unsigned char)host_ent->h_addr_list[0][3]);
    buff[15] = 0;

    return new_string_str(buff);

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'resolv-in-addr', syntax: resolv-in-addr hostname", interp);
}

Toy_Type*
cmd_coro(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *closure, *result;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    closure = list_get_item(posargs);
    if (GET_TAG(closure) != CLOSURE) goto error;

    result = new_coroutine(interp, closure);
    if (NULL == result) {
	return new_exception(TE_COROUTINE, L"Can\'t create co-routine.", interp);
    }

    return result;
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'coro', syntax: coro {block}", interp);
}

Toy_Type*
cmd_pause(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *val = NULL;
    Toy_Interp *parent;

    if (0 == interp->coroid) {
	return new_exception(TE_NOCOROUTINE, L"Not co-routine.", interp);
    }

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    if (arglen == 1) {
	val = list_get_item(posargs);
    } else {
	val = const_Nil;
    }

    parent = interp->co_parent;
    parent->co_value = toy_clone(val);

    co_resume();

    return interp->last_status;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'pause', syntax: pause [val]", interp);
}

Toy_Type*
cmd_gc(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
#ifndef PROF
    GC_gcollect();
#endif
    return const_Nil;
}

Toy_Type*
cmd_loop(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *do_block = NULL;
    Toy_Type *result;
    int r;

    if (hash_get_length(nameargs) == 1) {
	if (arglen != 0) goto error;
	do_block = hash_get_and_unset_t(nameargs, const_do);
	if ((do_block == NULL) ||
	    (GET_TAG(do_block) != CLOSURE)) goto error;
	if (hash_get_length(nameargs) != 0) goto error;
	
    } else {
	if (arglen != 1) goto error;
	if (hash_get_length(nameargs) != 0) goto error;

	do_block = list_get_item(posargs);
	if ((do_block == NULL) ||
	    (GET_TAG(do_block) != CLOSURE)) goto error;
    }

    result = const_Nil;

loop:
    if (1) {
    loop2:
	result = eval_closure(interp, do_block, interp->trace_info);
	r = GET_TAG(result);
	if (r == EXCEPTION) return result;
	if (r == CONTROL) {
	    switch (result->u.control.code) {
	    case CTRL_RETURN: case CTRL_GOTO:
		return result;
	    case CTRL_BREAK:
		return result->u.control.ret_value;
	    case CTRL_CONTINUE:
		result = const_Nil;
		goto loop;
	    case CTRL_REDO:
		result = const_Nil;
		goto loop2;
	    case CTRL_RETRY:
		result = const_Nil;
		goto loop;
	    }
	}
	goto loop;
    }

    return result;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'loop', \n\
syntax(1): loop {do-block}\n\
syntax(2): loop do: {do-block}", interp);
}

Toy_Type*
cmd_stacklist(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    return cstack_list();
}

Toy_Type*
cmd_issymbol(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;

    if (SYMBOL == GET_TAG(list_get_item(posargs))) return const_T;
    return const_Nil;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'symbol?', syntax: symbol? val", interp);
}

Toy_Type*
cmd_isnil(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;

    if (NIL == GET_TAG(list_get_item(posargs))) return const_T;
    return const_Nil;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'nil?', syntax: nil? val", interp);
}

Toy_Type*
cmd_islist(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;

    if (LIST == GET_TAG(list_get_item(posargs))) return const_T;
    return const_Nil;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'list?', syntax: list? val", interp);
} 

Toy_Type*
cmd_isinteger(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;

    if (INTEGER == GET_TAG(list_get_item(posargs))) return const_T;
    return const_Nil;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'integer?', syntax: integer? val", interp);
}

Toy_Type*
cmd_isreal(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;

    if (REAL == GET_TAG(list_get_item(posargs))) return const_T;
    return const_Nil;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'real?', syntax: real? val", interp);
}

Toy_Type*
cmd_isstring(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;

    if (STRING == GET_TAG(list_get_item(posargs))) return const_T;
    return const_Nil;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'string?', syntax: string? val", interp);
}

Toy_Type*
cmd_isrquote(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;

    if (RQUOTE == GET_TAG(list_get_item(posargs))) return const_T;
    return const_Nil;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'rquote?', syntax: rquote? val", interp);
}

Toy_Type*
cmd_isblock(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;

    if (CLOSURE == GET_TAG(list_get_item(posargs))) return const_T;
    return const_Nil;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'block?', syntax: block? val", interp);
}

Toy_Type*
cmd_isfunc(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;

    if (FUNC == GET_TAG(list_get_item(posargs))) return const_T;
    return const_Nil;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'func?', syntax: func? val", interp);
}

Toy_Type*
cmd_isobject(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;

    if (OBJECT == GET_TAG(list_get_item(posargs))) return const_T;
    return const_Nil;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'object?', syntax: object? val", interp);
}

Toy_Type*
cmd_isdict(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;

    if (DICT == GET_TAG(list_get_item(posargs))) return const_T;
    return const_Nil;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'dict?', syntax: dict? val", interp);
}

Toy_Type*
cmd_isvector(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;

    if (VECTOR == GET_TAG(list_get_item(posargs))) return const_T;
    return const_Nil;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'vector?', syntax: vector? val", interp);
}

Toy_Type*
cmd_cstack_release(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *t;
    int slot;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;

    t = list_get_item(posargs);
    if (INTEGER != GET_TAG(t)) goto error;

    slot = mpz_get_si(t->u.biginteger);
    if (NULL != cstack_get_start_addr(slot)) {
	co_delete((coroutine_t)cstack_get_start_addr(slot));
    }
    cstack_release_clear(slot);
    return const_Nil;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'cstack-release', syntax: cstack-release slot-id", interp);
}

Toy_Type*
cmd_coroid(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    int id;

    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) != 0) goto error;

    id = cstack_get_coroid();
    return new_integer_si(id);

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'coro-id', syntax: coro-id", interp);
}

Toy_Type*
cmd_remark(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    return interp->last_status;
}

Toy_Type*
cmd_uplevel(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Func_Env *env;
    Toy_Type *body, *result;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;

    body = list_get_item(posargs);
    if (GET_TAG(body) != CLOSURE) goto error;
    
    env = toy_pop_func_env(interp);
    if (NULL == env) {
	return new_exception(TE_STACKUNDERFLOW,
			     L"Stack underflow.", interp);
    }
    result = toy_eval_script(interp, body->u.closure.block_body);
    toy_push_func_env(interp, env->localvar, env->upstack, env->tobe_bind_val, interp->trace_info);

    return result;
    
error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'uplevel', syntax: uplevel {body}", interp);
}

Toy_Type*
cmd_debug(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    int olevel;
    Toy_Type *tlevel;

    if (arglen == 0) {
	return new_integer_si(interp->debug);
    }
    
    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;

    tlevel = list_get_item(posargs);
    if (GET_TAG(tlevel) != INTEGER) goto error;
    olevel = interp->debug;
    interp->debug = mpz_get_si(tlevel->u.biginteger);
    
    return new_integer_si(olevel);
    
error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'debug', syntax: debug [level]", interp);
}

Toy_Type*
cmd_where(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *result;
    Toy_Type *elem;
    int i;
    int top;

    if (arglen != 0) goto error;
    top = interp->cur_func_stack-1;
    if (hash_get_and_unset_t(nameargs, new_symbol(L"top:"))) {
	top = interp->cur_func_stack;
    }
    if (hash_get_length(nameargs) != 0) goto error;

    result = new_list(NULL);
    for (i=top; i>=0; i--) {
	elem = new_list(NULL);

	list_append(elem, new_cons(new_symbol(L"index"), 
				   new_integer_si(i)));
	list_append(elem, new_cons(new_symbol(L"line"),
				   new_integer_si(interp->func_stack[i]->trace_info->line)));
	list_append(elem, new_cons(new_symbol(L"object"),
				   toy_clone(interp->func_stack[i]->trace_info->object_name)));
	list_append(elem, new_cons(new_symbol(L"function"),
				   toy_clone(interp->func_stack[i]->trace_info->func_name)));
	list_append(elem, new_cons(new_symbol(L"statement"),
				   toy_clone(interp->func_stack[i]->trace_info->statement)));
	list_append(elem, new_cons(new_symbol(L"local"),
				   i>=1 ? new_dict(interp->func_stack[i-1]->localvar)
				        : new_dict(new_hash())));
	list_append(elem, new_cons(new_symbol(L"path"),
				   i>=1 ? new_string_str(get_script_path(interp, interp->func_stack[i-1]->script_id)) 
				        : new_string_str(L"-")));

	list_append(result, elem);
    }

    return result;
    
error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'where', syntax: where [:top]", interp);
}

Toy_Type*
cmd_istrue(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *var, *val;
    
    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    var = list_get_item(posargs);
    if (GET_TAG(var) != SYMBOL) goto error;
    
    val = toy_resolv_var(interp, var, 1, interp->trace_info);
    if (GET_TAG(val) == EXCEPTION) return const_Nil;
    if (GET_TAG(val) == NIL) return const_Nil;

    return const_T;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'true?', syntax: true? var", interp);
}

Toy_Type*
cmd_isfalse(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *var, *val;
    
    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    var = list_get_item(posargs);
    if (GET_TAG(var) != SYMBOL) goto error;
    
    val = toy_resolv_var(interp, var, 1, interp->trace_info);
    if (GET_TAG(val) == EXCEPTION) return const_T;
    if (GET_TAG(val) == NIL) return const_T;

    return const_Nil;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'false?', syntax: false? var", interp);
}

Toy_Type*
cmd_tag(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *var, *result, *attr;
    
    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    var = list_get_item(posargs);
    result = new_list(NULL);
    attr = new_list(NULL);
    
    list_append(result, new_cons(new_symbol(L"TAG"),
				 new_symbol(toy_get_type_str(var))));
    list_append(result, new_cons(new_symbol(L"SCRIPT_ID"), 
				 new_integer_si(GET_SCRIPT_ID(var))));
    list_append(result, new_cons(new_symbol(L"PARAM_NO"), 
				 new_integer_si(GET_PARAMNO(var))));
    if (IS_NAMED_SYM(var)) {
	list_append(attr, new_symbol(L"KEYWORD"));
    }
    if (IS_SWITCH_SYM(var)) {
	list_append(attr, new_symbol(L"SWITCH"));
    }
    if (IS_LAZY(var)) {
	list_append(attr, new_symbol(L"LAZY"));
    }
    if (IS_NOPRINTABLE(var)) {
	list_append(attr, new_symbol(L"NO_PRINT"));
    }
    list_append(result, new_cons(new_symbol(L"ATTR"), attr));
    
    return result;

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'tag?', syntax: tag? var", interp);
}

Toy_Type*
cmd_ref(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *var;
    
    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    var = list_get_item(posargs);
    if (GET_TAG(var) != SYMBOL) goto error;

    return toy_resolv_var(interp, var, 1, interp->trace_info);

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'ref', syntax: ref symbol", interp);
}

Toy_Type*
cmd_strftime(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *fmt, *ltime;
    char buff[256];
    struct tm t;
    long int i;
    
    if (arglen != 2) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    fmt = list_get_item(posargs);
    posargs = list_next(posargs);
    ltime = list_get_item(posargs);

    if (GET_TAG(fmt) != STRING) goto error;
    if (GET_TAG(ltime) != INTEGER) goto error;
    
    i = mpz_get_si(ltime->u.biginteger);
    localtime_r(&i, &t);
    if (0 == strftime(buff, 256, to_char(cell_get_addr(fmt->u.string)), &t)) {
	return new_exception(TE_SYNTAX, L"time format syntax error.", interp);
    }
    return new_string_str(to_wchar(buff));

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'strftime', syntax: strftime \"format\" localtime", interp);
}

Toy_Type*
cmd_gettimeofday(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    struct timeval tv;
    
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    memset(&tv, 0, sizeof(tv));
    if (-1 == gettimeofday(&tv, NULL)) {
	return new_exception(TE_SYSCALL, to_wchar(strerror(errno)), interp);
    }
    return new_integer_si(tv.tv_sec);

error:
    return new_exception(TE_SYNTAX,
			 L"Syntax error at 'time-of-day', syntax: get-time-of-day", interp);
}

int toy_add_commands(Toy_Interp *interp) {
    toy_add_func(interp, L"false", cmd_false, NULL);
    toy_add_func(interp, L"true", cmd_true, NULL);
    toy_add_func(interp, L"set", cmd_set, NULL);
    toy_add_func(interp, L"sets", cmd_sets, NULL);
    toy_add_func(interp, L"setc", cmd_setc, NULL);
    toy_add_func(interp, L"defvar", cmd_defvar, NULL);
    toy_add_func(interp, L"fun", cmd_fun, NULL);
    toy_add_func(interp, L"defun", cmd_defun, NULL);
    toy_add_func(interp, L"info", cmd_info, NULL);
    toy_add_func(interp, L"return", cmd_return, NULL);
    toy_add_func(interp, L"break", cmd_break, NULL);
    toy_add_func(interp, L"continue", cmd_continue, NULL);
    toy_add_func(interp, L"redo", cmd_redo, NULL);
    toy_add_func(interp, L"retry", cmd_retry, NULL);
    toy_add_func(interp, L"new", cmd_new, NULL);
    toy_add_func(interp, L"show-stack", cmd_showstack, NULL);
    toy_add_func(interp, L"if", cmd_if, NULL);
    toy_add_func(interp, L"while", cmd_while, NULL);
    toy_add_func(interp, L"print", cmd_print, NULL);
    toy_add_func(interp, L"println", cmd_println, NULL);
    toy_add_func(interp, L"time", cmd_time, NULL);
    toy_add_func(interp, L"class", cmd_class, NULL);
    toy_add_func(interp, L"try", cmd_try, NULL);
    toy_add_func(interp, L"throw", cmd_throw, NULL);
    toy_add_func(interp, L"case", cmd_case, NULL);
    toy_add_func(interp, L"cond", cmd_cond, NULL);
    toy_add_func(interp, L"set?", cmd_isset, NULL);
    toy_add_func(interp, L"sets?", cmd_issets, NULL);
    toy_add_func(interp, L"setc?", cmd_issetc, NULL);
    toy_add_func(interp, L"unset", cmd_unset, NULL);
    toy_add_func(interp, L"unsets", cmd_unsets, NULL);
    toy_add_func(interp, L"self", cmd_self, NULL);
    toy_add_func(interp, L"stack-trace", cmd_stacktrace, NULL);
    toy_add_func(interp, L"trap", cmd_trap, NULL);
    toy_add_func(interp, L"!", cmd_not, NULL);
    toy_add_func(interp, L"and", cmd_and, NULL);
    toy_add_func(interp, L"or", cmd_or, NULL);
    toy_add_func(interp, L"load", cmd_load, NULL);
    toy_add_func(interp, L"sid", cmd_sid, NULL);
    toy_add_func(interp, L"sdir", cmd_sdir, NULL);
    toy_add_func(interp, L"pwd", cmd_pwd, NULL);
    toy_add_func(interp, L"cd", cmd_chdir, NULL);
    toy_add_func(interp, L"alias", cmd_alias, NULL);
    toy_add_func(interp, L"sleep", cmd_sleep, NULL);
    toy_add_func(interp, L"setvar", cmd_setvar, NULL);
    toy_add_func(interp, L"file", cmd_file, NULL);
    toy_add_func(interp, L"exists?", cmd_exists, NULL);
    toy_add_func(interp, L"defvar?", cmd_isdefvar, NULL);
    toy_add_func(interp, L"type?", cmd_istype, NULL);
    toy_add_func(interp, L"call", cmd_call, NULL);
    toy_add_func(interp, L"rand", cmd_rand, NULL);
    toy_add_func(interp, L"symbol", cmd_symbol, NULL);
    toy_add_func(interp, L"trace", cmd_trace, NULL);
    toy_add_func(interp, L"exit", cmd_exit, NULL);
    toy_add_func(interp, L"fork", cmd_fork, NULL);
    toy_add_func(interp, L"yield", cmd_yield, NULL);
    toy_add_func(interp, L"result", cmd_result, NULL);
    toy_add_func(interp, L"lazy", cmd_lazy, NULL);
    toy_add_func(interp, L"begin", cmd_begin, NULL);
    toy_add_func(interp, L"fork-exec", cmd_forkandexec, NULL);
    toy_add_func(interp, L"wait", cmd_wait, NULL);
    toy_add_func(interp, L"read", cmd_read, NULL);
    toy_add_func(interp, L"goto", cmd_goto, NULL);
    toy_add_func(interp, L"dict", cmd_newdict, NULL);
    toy_add_func(interp, L"dict-local", cmd_localdict, NULL);
    toy_add_func(interp, L"dict-object", cmd_objdict, NULL);
    toy_add_func(interp, L"dict-global", cmd_globaldict, NULL);
    toy_add_func(interp, L"dict-func", cmd_funcdict, NULL);
    toy_add_func(interp, L"dict-class", cmd_classdict, NULL);
    toy_add_func(interp, L"dict-closure", cmd_closuredict, NULL);
    toy_add_func(interp, L"vector", cmd_newvector, NULL);
    toy_add_func(interp, L"eq?", cmd_equal, NULL);
    toy_add_func(interp, L"connect", cmd_connect, NULL);
    toy_add_func(interp, L"socket-server", cmd_socket_server, NULL);
    toy_add_func(interp, L"select", cmd_select, NULL);
    toy_add_func(interp, L"accept", cmd_accept, NULL);
    toy_add_func(interp, L"close", cmd_close, NULL);
    toy_add_func(interp, L"resolv-in-addr", cmd_resolv_in_addr, NULL);
    toy_add_func(interp, L"coro", cmd_coro, NULL);
    toy_add_func(interp, L"pause", cmd_pause, NULL);
    toy_add_func(interp, L"gc", cmd_gc, NULL);
    toy_add_func(interp, L"loop", cmd_loop, NULL);
    toy_add_func(interp, L"stack-list", cmd_stacklist, NULL);
    toy_add_func(interp, L"symbol?", cmd_issymbol, NULL);
    toy_add_func(interp, L"nil?", cmd_isnil, NULL);
    toy_add_func(interp, L"list?", cmd_islist, NULL);
    toy_add_func(interp, L"integer?", cmd_isinteger, NULL);
    toy_add_func(interp, L"real?", cmd_isreal, NULL);
    toy_add_func(interp, L"string?", cmd_isstring, NULL);
    toy_add_func(interp, L"rquote?", cmd_isrquote, NULL);
    toy_add_func(interp, L"block?", cmd_isblock, NULL);
    toy_add_func(interp, L"closure?", cmd_isblock, NULL);
    toy_add_func(interp, L"func?", cmd_isfunc, NULL);
    toy_add_func(interp, L"object?", cmd_isobject, NULL);
    toy_add_func(interp, L"dict?", cmd_isdict, NULL);
    toy_add_func(interp, L"vector?", cmd_isvector, NULL);
    toy_add_func(interp, L"cstack-release", cmd_cstack_release, NULL);
    toy_add_func(interp, L"coro-id", cmd_coroid, NULL);
    toy_add_func(interp, L"REM", cmd_remark, NULL);
    toy_add_func(interp, L"uplevel", cmd_uplevel, NULL);
    toy_add_func(interp, L"debug", cmd_debug, NULL);
    toy_add_func(interp, L"where", cmd_where, NULL);
    toy_add_func(interp, L"true?", cmd_istrue, NULL);
    toy_add_func(interp, L"false?", cmd_isfalse, NULL);
    toy_add_func(interp, L"tag?", cmd_tag, NULL);
    toy_add_func(interp, L"ref", cmd_ref, NULL);
    toy_add_func(interp, L"strftime", cmd_strftime, NULL);
    toy_add_func(interp, L"time-of-day", cmd_gettimeofday, NULL);

    return 0;
}

static void
println(Toy_Interp *interp, wchar_t *msg) {
    Toy_Type *l;

    l = new_list(new_symbol(L"println"));
    list_append(l, new_string_str(msg));

    toy_call(interp, l);

    return;
}
