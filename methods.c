/* $Id: methods.c,v 1.68 2011/12/09 12:54:40 mit-sato Exp $ */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <setjmp.h>
#include <ctype.h>
#include <sys/select.h>
#include <onigmo.h>
#include <math.h>
#include "toy.h"
#include "interp.h"
#include "types.h"
#include "cell.h"
#include "global.h"
#include "array.h"
#include "cstack.h"
#include "util.h"
#include "encoding.h"

Toy_Type* cmd_fun(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen);

Toy_Type*
mth_object_vars(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *selfh;
    
    if (arglen > 0) goto error;
    if (hash_get_length(nameargs)) goto error;

    selfh = SELF_HASH(interp);

    return hash_get_keys(selfh);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'vars', syntax: Object vars", interp);
}

Toy_Type*
mth_object_method(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
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

    h = SELF_HASH(interp);
    hash_set_t(h, cmd, res);
#ifdef HAS_GCACHE
    gcache_clear(interp);
#endif

    return res;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'method', syntax: Object method name (argspec) {block}\n\t\
argspec: [arg ...] [keyword: arg ...] [args: variable-args ...]", interp);
}

Toy_Type*
mth_object_get(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *h;
    Toy_Type *var, *o;
    Cell *c;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    var = list_get_item(posargs);
    if (GET_TAG(var) != SYMBOL) goto error;

    h = SELF_HASH(interp);
    o = hash_get_t(h, var);
    if (NULL == o) {
	c = new_cell(L"No such variable in slot, '");
	cell_add_str(c, cell_get_addr(var->u.symbol.cell));
	cell_add_str(c, L"'.");
	return new_exception(TE_NOVAR, cell_get_addr(c), interp);
    }
    return o;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'var?', syntax: Object var? | get name", interp);
}

Toy_Type*
mth_object_set(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *h;
    Toy_Type *var, *val;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 2) goto error;

    var = list_get_item(posargs);
    if (GET_TAG(var) != SYMBOL) goto error;

    posargs = list_next(posargs);
    val = list_get_item(posargs);

    h = SELF_HASH(interp);
    hash_set_t(h, var, val);

    return val;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'set', syntax: Object set name value", interp);
}

Toy_Type*
mth_object_type(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 0) goto error;

    return new_symbol(toy_get_type_str(SELF(interp)));
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'type?', syntax: Object type?", interp);
}

Toy_Type*
mth_object_delegate(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *var;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 0) goto error;

    var = SELF_OBJ(interp)->u.object.delegate_list;
    if (NULL == var) {
	return new_list(NULL);
    } else {
	return var;
    }
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'delegate?', syntax: Object delegate?", interp);
}

#if 0
Toy_Type*
mth_object_eq(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    if (SELF_OBJ(interp) == list_get_item(posargs)) return SELF_OBJ(interp);
    return const_Nil;
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'eq', syntax: Object eq object", interp);
}
#endif

Toy_Type*
mth_object_tostring(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 0) goto error;

    return new_string_str(to_string(SELF(interp)));
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'string', syntax: Object string", interp);
}

Toy_Type*
mth_object_toliteral(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 0) goto error;

    return new_string_str(to_print(SELF(interp)));
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'string', syntax: Object literal", interp);
}

Toy_Type*
mth_object_getmethod(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *method, *result;
    
    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    method = list_get_item(posargs);
    if (GET_TAG(method) != SYMBOL) goto error;

    result = search_method(interp, SELF_OBJ(interp), method);

    if (GET_TAG(result) == EXCEPTION) return const_Nil;
    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'method?', syntax: Object method? symbol", interp);
}

Toy_Type*
mth_object_apply(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *block;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    block = list_get_item(posargs);
    if (GET_TAG(block) != CLOSURE) goto error;

    return toy_eval_script(interp, block->u.closure.block_body);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'apply', syntax: Object apply {block}", interp);
}

static Toy_Type*
to_int(Toy_Interp *interp, Toy_Type *v) {
    Toy_Type *l, *cmd;
    
    l = cmd = new_list(v);
    l = list_append(l, new_symbol(L"int"));

    v = toy_call(interp, cmd);
    if (INTEGER == GET_TAG(v)) return v;
    if (EXCEPTION == GET_TAG(v)) return v;
    return new_exception(TE_TYPE, L"Type error.", interp);
}

static Toy_Type*
to_real(Toy_Interp *interp, Toy_Type *v) {
    Toy_Type *l, *cmd;
    
    l = cmd = new_list(v);
    l = list_append(l, new_symbol(L"real"));

    v = toy_call(interp, cmd);
    if (REAL == GET_TAG(v)) return v;
    if (EXCEPTION == GET_TAG(v)) return v;
    return new_exception(TE_TYPE, L"Type error.", interp);
}


Toy_Type*
mth_integer_plus(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;
    mpz_t s;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);

    if (INTEGER != GET_TAG(arg)) {
	arg = to_int(interp, arg);
	if (INTEGER != GET_TAG(arg)) return arg;
    }

    mpz_init(s);
    mpz_set(s, SELF(interp)->u.biginteger);
    mpz_add(s, s, arg->u.biginteger);
    return new_integer(s);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '+', syntax: Integer + number-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}


Toy_Type*
mth_integer_minus(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;
    mpz_t s;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);

    if (INTEGER != GET_TAG(arg)) {
	arg = to_int(interp, arg);
	if (INTEGER != GET_TAG(arg)) return arg;
    }

    mpz_init(s);
    mpz_set(s, SELF(interp)->u.biginteger);
    mpz_sub(s, s, arg->u.biginteger);
    return new_integer(s);
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at '-', syntax: Integer - number-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_mul(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;
    mpz_t s;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);

    if (INTEGER != GET_TAG(arg)) {
	arg = to_int(interp, arg);
	if (INTEGER != GET_TAG(arg)) return arg;
    }

    mpz_init(s);
    mpz_set(s, SELF(interp)->u.biginteger);
    mpz_mul(s, s, arg->u.biginteger);
    return new_integer(s);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '*', syntax: Integer * number-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_div(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;
    mpz_t s;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);

    if (INTEGER != GET_TAG(arg)) {
	arg = to_int(interp, arg);
	if (INTEGER != GET_TAG(arg)) return arg;
    }

    if (0 == mpz_cmp_si(arg->u.biginteger, 0)) {
	return new_exception(TE_ZERODIV, L"Zero divide.", interp);
    }
    mpz_init(s);
    mpz_set(s, SELF(interp)->u.biginteger);
    mpz_div(s, s, arg->u.biginteger);
    return new_integer(s);
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at '/', syntax: Integer / number-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_mod(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;
    mpz_t s;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);

    if (INTEGER != GET_TAG(arg)) {
	arg = to_int(interp, arg);
	if (INTEGER != GET_TAG(arg)) return arg;
    }

    if (0 == mpz_cmp_si(arg->u.biginteger, 0)) {
	return new_exception(TE_ZERODIV, L"Zero divide.", interp);
    }
    mpz_init(s);
    mpz_set(s, SELF(interp)->u.biginteger);
    mpz_mod(s, s, arg->u.biginteger);
    return new_integer(s);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '%', syntax: Integer % integer-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}


Toy_Type*
mth_integer_eq(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);

    if (INTEGER != GET_TAG(arg)) {
	arg = to_int(interp, arg);
	if (INTEGER != GET_TAG(arg)) return arg;
    }

    if (0 == (mpz_cmp(SELF(interp)->u.biginteger,
		      arg->u.biginteger))) {
	return SELF(interp);
    }
    return const_Nil;
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at '=', syntax: Integer = number-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_neq(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);

    if (INTEGER != GET_TAG(arg)) {
	arg = to_int(interp, arg);
	if (INTEGER != GET_TAG(arg)) return arg;
    }

    if (0 != (mpz_cmp(SELF(interp)->u.biginteger,
		      arg->u.biginteger))) {
	return SELF(interp);
    }
    return const_Nil;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '!=', syntax: Integer != number-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_gt(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);

    if (INTEGER != GET_TAG(arg)) {
	arg = to_int(interp, arg);
	if (INTEGER != GET_TAG(arg)) return arg;
    }

    if (mpz_cmp(SELF(interp)->u.biginteger, arg->u.biginteger) > 0) {
	return SELF(interp);
    }
    return const_Nil;
	
error:
    return new_exception(TE_SYNTAX, L"Syntax error at '>', syntax: Integer > number-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_lt(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);

    if (INTEGER != GET_TAG(arg)) {
	arg = to_int(interp, arg);
	if (INTEGER != GET_TAG(arg)) return arg;
    }

    if (mpz_cmp(SELF(interp)->u.biginteger, arg->u.biginteger) < 0) {
	return SELF(interp);
    }
    return const_Nil;
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at '<', syntax: Integer < number-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_ge(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);

    if (INTEGER != GET_TAG(arg)) {
	arg = to_int(interp, arg);
	if (INTEGER != GET_TAG(arg)) return arg;
    }

    if (mpz_cmp(SELF(interp)->u.biginteger, arg->u.biginteger) >= 0) {
	return SELF(interp);
    }
    return const_Nil;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '>=', syntax: Integer >= number-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_le(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);

    if (INTEGER != GET_TAG(arg)) {
	arg = to_int(interp, arg);
	if (INTEGER != GET_TAG(arg)) return arg;
    }

    if (mpz_cmp(SELF(interp)->u.biginteger, arg->u.biginteger) <= 0) {
	return SELF(interp);
    }
    return const_Nil;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '<=', syntax: Integer <= number-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_inc(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);

    if (arg == NULL) {
	mpz_add_ui(SELF(interp)->u.biginteger,
		   SELF(interp)->u.biginteger,
		   1);
	return SELF(interp);
    }

    if (GET_TAG(arg) != INTEGER) {
	arg = to_int(interp,arg);
    }
    if (GET_TAG(arg) == INTEGER) {
	mpz_add(SELF(interp)->u.biginteger,
		SELF(interp)->u.biginteger,
		arg->u.biginteger);
	return SELF(interp);
    }
    return arg;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '++', syntax: Integer ++ [int-val]", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_dec(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);

    if (arg == NULL) {
	mpz_sub_ui(SELF(interp)->u.biginteger,
		   SELF(interp)->u.biginteger,
		   1);
	return SELF(interp);
    }

    if (GET_TAG(arg) != INTEGER) {
	arg = to_int(interp,arg);
    }
    if (GET_TAG(arg) == INTEGER) {
	mpz_sub(SELF(interp)->u.biginteger,
		SELF(interp)->u.biginteger,
		arg->u.biginteger);
	return SELF(interp);
    }
    return arg;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '--', syntax: Integer -- [int-val]", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_each(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    int i, upto, step = 1;
    Toy_Type *any, *block;
    Toy_Type *result;
    int t;

    if (arglen > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    any = hash_get_and_unset_t(nameargs, const_to);
    if (NULL == any) goto error;
    if (GET_TAG(any) != INTEGER) goto error;
    upto = mpz_get_si(any->u.biginteger);

    block = hash_get_and_unset_t(nameargs, const_do);
    if (NULL == block) goto error;
    if (GET_TAG(block) != CLOSURE) goto error;

    if (hash_get_length(nameargs) > 0) goto error;
    
loop_retry:
    i = mpz_get_si(SELF(interp)->u.biginteger);

    if (i > upto) step = -1;

loop:
    result = toy_yield(interp, block, new_list(new_integer_si(i)));
    t = GET_TAG(result);
    if (t == EXCEPTION) return result;
    if (t == CONTROL) {

	switch (result->u.control.code) {
	case CTRL_RETURN: case CTRL_GOTO:
	    return result;
	    break;

	case CTRL_BREAK:
	    result = result->u.control.ret_value;
	    goto done;

	case CTRL_CONTINUE:
	    result = const_Nil;
	    goto loop_continue;
	    break;

	case CTRL_REDO:
	    result = const_Nil;
	    goto loop;
	    break;

	case CTRL_RETRY:
	    result = const_Nil;
	    goto loop_retry;
	    break;
	}
    }

loop_continue:    
    i += step;
    if (step == 1) {
	if (i <= upto) goto loop;
    } else {
	if (i >= upto) goto loop;
    }

done:
    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'each', syntax: Integer each to: number do: {| var | block}",
			 interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_list(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    int i, upto, step = 1;
    Toy_Type *any, *block;
    Toy_Type *result, *lresult, *l;
    int t, f;

    if (arglen != 1) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    any = list_get_item(posargs);
    if (NULL == any) goto error;
    if (GET_TAG(any) != INTEGER) goto error;
    upto = mpz_get_si(any->u.biginteger);

    block = hash_get_and_unset_t(nameargs, const_do);
    if (block && (GET_TAG(block) != CLOSURE)) goto error;

    f = block && 1;

    if (hash_get_length(nameargs) > 0) goto error;
    
    l = lresult = new_list(NULL);

loop_retry:
    i = mpz_get_si(SELF(interp)->u.biginteger);

    if (i > upto) step = -1;

loop:
    if (f) {
	result = toy_yield(interp, block, new_list(new_integer_si(i)));
    } else {
	result = new_integer_si(i);
    }
    t = GET_TAG(result);
    if (t == EXCEPTION) return result;
    if (t == CONTROL) {

	switch (result->u.control.code) {
	case CTRL_RETURN: case CTRL_GOTO:
	    result = result->u.control.ret_value;
	    break;

	case CTRL_BREAK:
	    result = result->u.control.ret_value;
	    goto done;

	case CTRL_CONTINUE:
	    result = const_Nil;
	    goto loop_continue;
	    break;

	case CTRL_REDO:
	    result = const_Nil;
	    goto loop;
	    break;

	case CTRL_RETRY:
	    result = const_Nil;
	    goto loop_retry;
	    break;
	}
    }

    l = list_append(l, result);

loop_continue:    
    i += step;
    if (step == 1) {
	if (i <= upto) goto loop;
    } else {
	if (i >= upto) goto loop;
    }

done:
    return lresult;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '..', syntax: Integer .. last [do: {| var | block}]",
			 interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_toreal(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    return new_real(mpz_get_d(SELF(interp)->u.biginteger));
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'real', syntax: Integer real", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_rol(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self, *arg;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != INTEGER) goto error2;
    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;

    return new_integer_si(mpz_get_si(self->u.biginteger)
			  << mpz_get_si(arg->u.biginteger));

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '<<', syntax: Integer << bit", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_ror(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self, *arg;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != INTEGER) goto error2;
    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;

    return new_integer_si(mpz_get_si(self->u.biginteger)
			  >> mpz_get_si(arg->u.biginteger));

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '>>', syntax: Integer >> bit", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_or(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self, *arg;
    mpz_t s;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != INTEGER) goto error2;
    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;

    mpz_init(s);
    mpz_set(s, self->u.biginteger);
    mpz_ior(s, s, arg->u.biginteger);
    return new_integer(s);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '||', syntax: Integer || val", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_and(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self, *arg;
    mpz_t s;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != INTEGER) goto error2;
    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;

    mpz_init(s);
    mpz_set(s, self->u.biginteger);
    mpz_and(s, s, arg->u.biginteger);
    return new_integer(s);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '&&', syntax: Integer && val", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_xor(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self, *arg;
    mpz_t s;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != INTEGER) goto error2;
    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;

    mpz_init(s);
    mpz_set(s, self->u.biginteger);
    mpz_xor(s, s, arg->u.biginteger);
    return new_integer(s);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '^^', syntax: Integer ^^ val", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_not(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;

    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) != 0) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != INTEGER) goto error2;

    return new_integer_si(~mpz_get_si(self->u.biginteger));

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '~~', syntax: Integer ~~ val", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_power(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;
    mpz_t s;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);
    if (INTEGER != GET_TAG(arg)) goto error;

    mpz_init(s);
    mpz_set(s, SELF(interp)->u.biginteger);
    if (mpz_get_si(arg->u.biginteger) < 0) goto error;
    mpz_pow_ui(s, s, mpz_get_si(arg->u.biginteger));
    return new_integer(s);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '^', syntax: Integer ^ integer-val(>=0)", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_nextprime(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    mpz_t s, next;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    mpz_init(s);
    mpz_init(next);
    mpz_set(s, SELF(interp)->u.biginteger);
    mpz_nextprime(next, s);

    return new_integer(next);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'nextprime', syntax: Integer nextprime", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_sqrt(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    mpz_t s, sqrt, zero;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    mpz_init(s);
    mpz_init(sqrt);
    mpz_init(zero);
    mpz_set(s, SELF(interp)->u.biginteger);
    if (mpz_cmp(s, zero) < 0) {
	return new_exception(TE_ZERODIV, L"Zero divide.", interp);
    }
    mpz_sqrt(sqrt, s);

    return new_integer(sqrt);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'sqrt', syntax: Integer sqrt", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_neg(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    mpz_t s, neg;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    mpz_init(s);
    mpz_init(neg);
    mpz_set(s, SELF(interp)->u.biginteger);
    mpz_neg(neg, s);

    return new_integer(neg);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'neg', syntax: Integer neg", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_integer_abs(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    mpz_t s;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    mpz_init(s);
    mpz_abs(s, SELF(interp)->u.biginteger);

    return new_integer(s);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'abs', syntax: Integer abs", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_plus(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    arg = list_get_item(posargs);
    if (REAL != GET_TAG(arg)) {
	arg = to_real(interp, arg);
	if (REAL != GET_TAG(arg)) return arg;
    }

    return new_real(SELF(interp)->u.real + arg->u.real);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '+', syntax: Real + number-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}


Toy_Type*
mth_real_minus(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    arg = list_get_item(posargs);
    if (REAL != GET_TAG(arg)) {
	arg = to_real(interp, arg);
	if (REAL != GET_TAG(arg)) return arg;
    }

    return new_real(SELF(interp)->u.real - arg->u.real);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '-', syntax: Real - number-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_mul(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    arg = list_get_item(posargs);
    if (REAL != GET_TAG(arg)) {
	arg = to_real(interp, arg);
	if (REAL != GET_TAG(arg)) return arg;
    }

    return new_real(SELF(interp)->u.real * arg->u.real);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '*', syntax: Real * number-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_div(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    arg = list_get_item(posargs);
    if (REAL != GET_TAG(arg)) {
	arg = to_real(interp, arg);
	if (REAL != GET_TAG(arg)) return arg;
    }

    if (arg->u.real == 0.0) {
	return new_exception(TE_ZERODIV, L"Zero divide.", interp);
    }
    return new_real(SELF(interp)->u.real / arg->u.real);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '/', syntax: Real / number-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_eq(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    arg = list_get_item(posargs);
    if (REAL != GET_TAG(arg)) {
	arg = to_real(interp, arg);
	if (REAL != GET_TAG(arg)) return arg;
    }

    if (SELF(interp)->u.real == arg->u.real) {
	return SELF(interp);
    }
    return const_Nil;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '=', syntax: Real = number-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_neq(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    arg = list_get_item(posargs);
    if (REAL != GET_TAG(arg)) {
	arg = to_real(interp, arg);
	if (REAL != GET_TAG(arg)) return arg;
    }

    if (SELF(interp)->u.real != arg->u.real) {
	return SELF(interp);
    }
    return const_Nil;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '!=', syntax: Real != number-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_gt(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    arg = list_get_item(posargs);
    if (REAL != GET_TAG(arg)) {
	arg = to_real(interp, arg);
	if (REAL != GET_TAG(arg)) return arg;
    }

    if (SELF(interp)->u.real > arg->u.real) {
	return SELF(interp);
    }
    return const_Nil;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '>', syntax: Real > number-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_lt(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    arg = list_get_item(posargs);
    if (REAL != GET_TAG(arg)) {
	arg = to_real(interp, arg);
	if (REAL != GET_TAG(arg)) return arg;
    }

    if (SELF(interp)->u.real < arg->u.real) {
	return SELF(interp);
    }
    return const_Nil;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '<', syntax: Real < number-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_ge(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    arg = list_get_item(posargs);
    if (REAL != GET_TAG(arg)) {
	arg = to_real(interp, arg);
	if (REAL != GET_TAG(arg)) return arg;
    }

    if (SELF(interp)->u.real >= arg->u.real) {
	return SELF(interp);
    }
    return const_Nil;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '>=', syntax: Real >= number-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_le(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    arg = list_get_item(posargs);
    if (REAL != GET_TAG(arg)) {
	arg = to_real(interp, arg);
	if (REAL != GET_TAG(arg)) return arg;
    }

    if (SELF(interp)->u.real <= arg->u.real) {
	return SELF(interp);
    }
    return const_Nil;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '<=', syntax: Real <= number-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_tointeger(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    if (isinf(SELF(interp)->u.real)) {
	return new_exception(TE_INFINITY, L"Do not convert infinity value.", interp);
    }
    if (isnan(SELF(interp)->u.real)) {
	return new_exception(TE_NAN, L"Do not convert NaN value.", interp);
    }

    return new_integer_d(SELF(interp)->u.real);
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'int', syntax: Real int", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_sqrt(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    if (SELF(interp)->u.real < 0.0) {
	return new_exception(TE_ZERODIV, L"Zero divide.", interp);
    }
    return new_real(sqrt(SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'sqrt', syntax: Real sqrt", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_sin(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(sin(SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'sin', syntax: Real sin", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_cos(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(cos(SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'cos', syntax: Real cos", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_tan(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(tan(SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'tan', syntax: Real tan", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_asin(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(asin(SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'asin', syntax: Real asin", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_acos(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(acos(SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'acos', syntax: Real acos", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_atan(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(atan(SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'atan', syntax: Real atan", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_log(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(log(SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'log', syntax: Real log", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_log10(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(log10(SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'log10', syntax: Real log10", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_exp(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(exp(SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'exp', syntax: Real exp", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_exp10(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(pow(10.0, SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'exp10', syntax: Real exp10", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_pow(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *exp;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    exp = list_get_item(posargs);
    if (REAL != GET_TAG(exp)) goto error;

    return new_real(pow(SELF(interp)->u.real, exp->u.real));
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'pow', syntax: Real pow real-value", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_neg(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(- SELF(interp)->u.real);
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'neg', syntax: Real neg", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_abs(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(fabs(SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'abs', syntax: Real abs", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_isinf(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    if (isinf(SELF(interp)->u.real)) {
	return const_T;
    } else {
	return const_Nil;
    }
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'inf?', syntax: Real inf?", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_real_isnan(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    if (isnan(SELF(interp)->u.real)) {
	return const_T;
    } else {
	return const_Nil;
    }
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'nan?', syntax: Real nan?", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_last(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *l;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != LIST) goto error2;

    l = SELF(interp);
    if (GET_TAG(l) != LIST) goto error;

    while (l->u.list.nextp) {
	l = list_next(l);
	if (GET_TAG(l) != LIST) return l;
    }
    return l;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'last', syntax: List last", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_item(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *l;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != LIST) goto error2;

    l = SELF(interp);
    if (GET_TAG(l) != LIST) goto error;

    return ((l->u.list.item==NULL)?const_Nil:(l->u.list.item));

//    if (NULL == list_get_item(l)) return new_list(NULL);
//    return list_get_item(l);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'car', syntax: List car | item", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_cdr(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *l;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != LIST) goto error2;

    l = SELF(interp);
    if (GET_TAG(l) != LIST) goto error;

    return ((l->u.list.nextp==NULL)?const_Nil:(l->u.list.nextp));

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'cdr', syntax: List cdr | next", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_append(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *l;

    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != LIST) goto error2;

    l = SELF(interp);
    while (posargs) {
	if (! IS_LIST_NULL(posargs)) {
	    l = list_append(l, list_get_item(posargs));
	}
	posargs = list_next(posargs);
    }

    return l;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'append', syntax: List append! | + [val ...]", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_add(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *l, *result;

    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != LIST) goto error2;

    l = result = SELF(interp);
    while (posargs) {
	if (! IS_LIST_NULL(posargs)) {
	    l = list_append(l, list_get_item(posargs));
	}
	posargs = list_next(posargs);
    }

    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'add', syntax: List add [val ...]", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_each(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *block;
    Toy_Type *l, *item;
    Toy_Type *result;
    int t;
    
    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 2) goto error;
    if (GET_TAG(SELF(interp)) != LIST) goto error2;

    block = hash_get_and_unset_t(nameargs, const_do);
    if (hash_get_length(nameargs) > 0) goto error;

    if ((block == NULL) || (GET_TAG(block) != CLOSURE)) goto error;

loop_retry:
    result = const_Nil;
    l = SELF(interp);

loop:
    if (! l) goto fin;
    if (IS_LIST_NULL(l)) goto fin;
    
    if (GET_TAG(l) == LIST) {
	item = list_get_item(l);
    } else {
	item = l;
    }
    result = toy_yield(interp, block, new_list(item));
    t = GET_TAG(result);
    if (t == EXCEPTION) return result;
    if (t == CONTROL) {
	switch (result->u.control.code) {
	case CTRL_RETURN: case CTRL_GOTO:
	    return result;
	    break;
	case CTRL_BREAK:
	    return result->u.control.ret_value;
	    break;
	case CTRL_CONTINUE:
	    result = const_Nil;
	    goto loop_continue;
	    break;
	case CTRL_REDO:
	    result = const_Nil;
	    goto loop;
	    break;
	case CTRL_RETRY:
	    result = const_Nil;
	    goto loop_retry;
	    break;
	}
    }

loop_continue:
    l = list_next(l);
    goto loop;
    
fin:
    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'each', syntax: List each do: {| var | block}", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_len(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *l;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != LIST) goto error2;

    l = SELF(interp);
    return new_integer_si(list_length(l));

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'len', syntax: List len", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_isnull(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *l;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != LIST) goto error2;

    l = SELF(interp);
    if (IS_LIST_NULL(l)) {
	return const_T;
    } else {
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'null?', syntax: List null?", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_join(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *sep, *self;
    Cell *result;
    wchar_t *psep;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 1) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != LIST) goto error2;
    sep = hash_get_and_unset_t(nameargs, const_sep);
    if (sep == NULL) {
	sep = new_string_str(L"");
    } else {
	sep = new_string_str(to_string(sep));
    }
    psep = cell_get_addr(sep->u.string);

    result = new_cell(L"");

    while (! IS_LIST_NULL(self)) {
	cell_add_str(result, to_string_call(interp, list_get_item(self)));

	self = list_next(self);
	if (self) {
	    cell_add_str(result, psep);
	}
	if (self && GET_TAG(self) != LIST) {
	    cell_add_str(result, to_string_call(interp, self));
	    break;
	}
    }

    return new_string_cell(result);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'join', syntax: List join [sep: separator]", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_eval(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Env *env = NULL;
    Toy_Type *result;

    if (arglen > 0) goto error;

    result = toy_eval(interp, new_statement(SELF(interp), 
					    interp->func_stack[interp->cur_func_stack]->trace_info->line), &env);
    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'eval', syntax: List eval", interp);
}

Toy_Type*
mth_list_new_append(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *result, *l, *src;

    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != LIST) goto error2;

    result = l = new_list(NULL);
    src = SELF(interp);
    while (src) {
	l = list_append(l, list_get_item(src));
	src = list_next(src);
    }

    while (posargs) {
	if (! IS_LIST_NULL(posargs)) {
	    l = list_append(l, list_get_item(posargs));
	}
	posargs = list_next(posargs);
    }

    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '.', syntax: List . [val ...]", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_get(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    int pos, i;
    Toy_Type *src, *index;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != LIST) goto error2;

    index = list_get_item(posargs);
    if (GET_TAG(index) != INTEGER) goto error;
    pos = mpz_get_si(index->u.biginteger);
    if (pos < 0) goto error;

    src = SELF(interp);

    for (i=0; i<pos; i++) {
	if (NULL == src) return const_Nil;
	src = list_next(src);
    }

    if (NULL == src) return const_Nil;

    return (GET_TAG(src)==LIST) ? (list_get_item(src) ? list_get_item(src) : const_Nil) : src;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'get', syntax: List get index", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_filter(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *block, *self, *result, *l, *ret;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;

    block = list_get_item(posargs);
    if (GET_TAG(block) != CLOSURE) goto error;

    self = SELF(interp);
    if (GET_TAG(self) != LIST) goto error2;

    l = result = new_list(NULL);

    while (! IS_LIST_NULL(self)) {
	if (GET_TAG(self) == LIST) {
	    ret = toy_yield(interp, block, new_list(list_get_item(self)));
	} else {
	    ret = toy_yield(interp, block, new_list(self));
	}
	if (GET_TAG(ret) == EXCEPTION) return ret;
	if (! IS_NIL(ret)) {
	    l = list_append(l, list_get_item(self));
	}
	self = list_next(self);
	if (GET_TAG(self) != LIST) break;
    }

    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'filter', syntax: List filter {| var | block}", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_map(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *block, *self, *result, *l, *ret;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;

    block = list_get_item(posargs);
    if (GET_TAG(block) != CLOSURE) goto error;

    self = SELF(interp);
    if (GET_TAG(self) != LIST) goto error2;

    l = result = new_list(NULL);

    while (! IS_LIST_NULL(self)) {
	if (GET_TAG(self) == LIST) {
	    ret = toy_yield(interp, block, new_list(list_get_item(self)));
	} else {
	    ret = toy_yield(interp, block, new_list(self));
	}
	if (GET_TAG(ret) == EXCEPTION) return ret;
	if (GET_TAG(ret) == CONTROL) {
	    l = list_append(l, ret->u.control.ret_value);
	} else {
	    l = list_append(l, ret);
	}

	self = list_next(self);
	if (GET_TAG(self) != LIST) break;
    }

    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'map', syntax: List map {| var | block}", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_concat(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self, *l, *item;

    if (hash_get_length(nameargs) != 0) goto error;

    l = self = SELF(interp);
    if (GET_TAG(self) != LIST) goto error2;

    while (! IS_LIST_NULL(posargs)) {
	item = list_get_item(posargs);

	if (GET_TAG(item) == LIST) {
	    Toy_Type *fitem;
	    fitem = item;
	    while (! IS_LIST_NULL(fitem)) {
		l = list_append(l, list_get_item(fitem));
		fitem = list_next(fitem);
	    }
	} else {
	    l = list_append(l, item);
	}

	posargs = list_next(posargs);
    }

    return self;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'concat', syntax: List concat (list) | var ...", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_seek(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self, *t;
    int i;
    long long int index;

    if (hash_get_length(nameargs) != 0) goto error;
    if (arglen != 1) goto error;

    self = SELF(interp);
    if (GET_TAG(self) != LIST) goto error2;

    t = list_get_item(posargs);
    if (GET_TAG(t) != INTEGER) goto error;
    index = mpz_get_si(t->u.biginteger);

    i = 0;
    while ((! IS_LIST_NULL(self)) && (i < index)) {
	self = list_next(self);
	i++ ;
    }

    if (NULL == self) return new_list(NULL);
    return self;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'seek', syntax: List seek index", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_split(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self, *result, *t, *left, *lleft, *right, *lright;
    int i;
    long long int index;

    if (hash_get_length(nameargs) != 0) goto error;
    if (arglen != 1) goto error;

    self = SELF(interp);
    if (GET_TAG(self) != LIST) goto error2;

    t = list_get_item(posargs);
    if (GET_TAG(t) != INTEGER) goto error;
    index = mpz_get_si(t->u.biginteger);

    lleft = left = new_list(NULL);
    lright = right = new_list(NULL);
    result = new_list(NULL);
    i = 0;
    while ((! IS_LIST_NULL(self)) && (i < index)) {
	if (GET_TAG(self) != LIST) {
	    lleft = list_append(lleft, self);
	    break;
	} else {
	    lleft = list_append(lleft, list_get_item(self));
	}
	self = list_next(self);
	i++ ;
    }
    if (GET_TAG(self) == LIST) {
	while (! IS_LIST_NULL(self)) {
	    if (GET_TAG(self) != LIST) {
		lright = list_append(lright, self);
		break;
	    } else {
		lright = list_append(lright, list_get_item(self));
	    }
	    self = list_next(self);
	}
    }

    list_append(result, left);
    list_append(result, right);
    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'split', syntax: List split index", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_unshift(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self, *new;

    if (arglen != 1) goto error;

    self = SELF(interp);
    if (GET_TAG(self) != LIST) goto error2;

    if (IS_LIST_NULL(self)) {
	list_set_car(SELF(interp), list_get_item(posargs));
	list_set_cdr(SELF(interp), NULL);
    } else {
	new = new_list(NULL);
	list_set_car(new, list_get_item(self));
	list_set_cdr(new, list_next(self));
	list_set_car(SELF(interp), list_get_item(posargs));
	list_set_cdr(SELF(interp), new);
    }

    return SELF(interp);
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at '<<', syntax: List << val", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_shift(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self, *result;

    if (arglen != 0) goto error;

    self = SELF(interp);
    if (GET_TAG(self) != LIST) goto error2;

    result = list_get_item(self);
    list_set_car(SELF(interp), list_get_item(list_next(self)));
    list_set_cdr(SELF(interp), list_next(list_next(self)));

    if (result) return result;
    return const_Nil;
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at '>>', syntax: List >>", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_push(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self, *new;

    if (arglen != 1) goto error;

    self = SELF(interp);
    if (GET_TAG(self) != LIST) goto error2;

    if (IS_LIST_NULL(self)) {
	list_set_car(SELF(interp), list_get_item(posargs));
	list_set_cdr(SELF(interp), NULL);
    } else {
	while (list_next(self) && (GET_TAG(list_next(self)) == LIST)) {
	    self = list_next(self);
	}
	new = new_list(NULL);
	list_set_cdr(self, new);
	list_set_car(new, list_get_item(posargs));
    }

    return SELF(interp);
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at '<<-', syntax: List <<- val", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_pop(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self, *result, *b = NULL;

    if (arglen != 0) goto error;

    self = SELF(interp);
    if (GET_TAG(self) != LIST) goto error2;

    if (IS_LIST_NULL(self)) return const_Nil;

    while (list_next(self) && (GET_TAG(list_next(self)) == LIST)) {
	b = self;
	self = list_next(self);
    }
    result = list_get_item(self);

    if (b) {
	list_set_cdr(b, NULL);
    } else {
	list_set_car(SELF(interp), NULL);
	list_set_cdr(SELF(interp), NULL);
    }
    
    if (result) return result;
    return const_Nil;
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at '->>', syntax: List ->>", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_inject(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *block, *sum_val, *l, *item;
    Toy_Type *result, *yval;
    int t;
    
    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 1) goto error;
    if (GET_TAG(SELF(interp)) != LIST) goto error2;

    block = hash_get_and_unset_t(nameargs, const_do);
    if ((block == NULL) || (GET_TAG(block) != CLOSURE)) goto error;

    sum_val = list_get_item(posargs);
    if (sum_val == NULL) goto error;

loop_retry:
    result = sum_val;
    l = SELF(interp);
    if (GET_TAG(l) != LIST) goto error2;

loop:
    if (! l) goto fin;
    if (IS_LIST_NULL(l)) goto fin;
    
    item = list_get_item(l);

    yval = new_list(result);
    list_append(yval, item);

    result = toy_yield(interp, block, yval);
    t = GET_TAG(result);
    if (t == EXCEPTION) return result;
    if (t == CONTROL) {
	switch (result->u.control.code) {
	case CTRL_RETURN: case CTRL_GOTO:
	    return result;
	    break;
	case CTRL_BREAK:
	    return result->u.control.ret_value;
	    break;
	case CTRL_CONTINUE:
	    result = const_Nil;
	    goto loop_continue;
	    break;
	case CTRL_REDO:
	    result = const_Nil;
	    goto loop;
	    break;
	case CTRL_RETRY:
	    result = const_Nil;
	    goto loop_retry;
	    break;
	}
    }

loop_continue:
    
    l = list_next(l);
    goto loop;
    
fin:
    return result;

error:
    return new_exception(TE_SYNTAX,
	L"Syntax error at 'inject', syntax: List inject init-val do: {| sum-var each-var | block}", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_setcar(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *l, *v;

    if (hash_get_length(nameargs) > 0) goto error;
    if (list_length(posargs) != 1) goto error;

    l = SELF(interp);
    if (GET_TAG(l) != LIST) goto error2;
    v = list_get_item(posargs);
    
    l->u.list.item = v;

    return l;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'set-car!', syntax: List set-car! val", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_setcdr(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *l, *v;

    if (hash_get_length(nameargs) > 0) goto error;
    if (list_length(posargs) != 1) goto error;

    l = SELF(interp);
    if (GET_TAG(l) != LIST) goto error2;
    v = list_get_item(posargs);
    
    l->u.list.nextp = v;

    return l;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'set-car!', syntax: List set-cdr! val", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_list_block(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *l;

    if (hash_get_length(nameargs) > 0) goto error;
    if (list_length(posargs) != 0) goto error;

    l = SELF(interp);
    if (GET_TAG(l) != LIST) goto error2;
    return new_closure(new_script(new_list(
				      new_statement(l,
					interp->func_stack[interp->cur_func_stack]->trace_info->line))),
		       new_closure_env(interp), interp->func_stack[interp->cur_func_stack]->script_id);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'block', syntax: List create-block", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_len(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *s;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != STRING) goto error2;

    s = SELF(interp);
    return new_integer_si(cell_get_length(s->u.string));

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'len', syntax: String len", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_plus(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Cell *s;

    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != STRING) goto error2;

    s = new_cell(cell_get_addr(SELF(interp)->u.string));

    while (list_get_item(posargs)) {
	cell_add_str(s, to_string_call(interp, list_get_item(posargs)));
	posargs = list_next(posargs);
    }
    
    return new_string_cell(s);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '+', syntax: String + val ...", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_equal(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != STRING) goto error2;

    if (wcscmp(cell_get_addr(self->u.string),
	       to_string_call(interp, list_get_item(posargs))) == 0) {
	return const_T;
    } else {
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '=', syntax: String = object", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_gt(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != STRING) goto error2;

    if (wcscmp(cell_get_addr(self->u.string),
	       to_string_call(interp, list_get_item(posargs))) > 0) {
	return const_T;
    } else {
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '>', syntax: String > object", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_lt(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != STRING) goto error2;

    if (wcscmp(cell_get_addr(self->u.string),
	       to_string_call(interp, list_get_item(posargs))) < 0) {
	return const_T;
    } else {
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '<', syntax: String < object", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_ge(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != STRING) goto error2;

    if (wcscmp(cell_get_addr(self->u.string),
	       to_string_call(interp, list_get_item(posargs))) >= 0) {
	return const_T;
    } else {
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '>=', syntax: String >= object", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_le(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != STRING) goto error2;

    if (wcscmp(cell_get_addr(self->u.string),
	       to_string_call(interp, list_get_item(posargs))) <= 0) {
	return const_T;
    } else {
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '<=', syntax: String <= object", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_nequal(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != STRING) goto error2;

    if (wcscmp(cell_get_addr(self->u.string),
	       to_string_call(interp, list_get_item(posargs))) != 0) {
	return const_T;
    } else {
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '!=', syntax: String != object", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_eval(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;
    Bulk *b;
    Toy_Type *script, *result;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 0) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != STRING) goto error2;

    b = new_bulk();
    if (NULL == b) return const_Nil;

    bulk_set_string(b, cell_get_addr(self->u.string));
    script = toy_parse_start(b);
    if (GET_TAG(script) != SCRIPT) return script;

    SET_SCRIPT_ID(script, interp->script_id);
    script_apply_trace_info(script, interp->trace_info);
    result = toy_eval_script(interp, script);

    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'eval', syntax: String eval", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_append(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;
    Cell *o;

    if (hash_get_length(nameargs) > 0) goto error;

    self = SELF(interp);
    if (GET_TAG(self) != STRING) goto error2;

    o = self->u.string;

    while (posargs) {
	if (! IS_LIST_NULL(posargs)) {
	    cell_add_str(o, to_string_call(interp, list_get_item(posargs)));
	}

	posargs = list_next(posargs);
    }

    return self;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '+', syntax: String + [val ...]", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_concat(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;
    Cell *o;

    if (hash_get_length(nameargs) > 0) goto error;

    self = SELF(interp);
    if (GET_TAG(self) != STRING) goto error2;

    o = new_cell(cell_get_addr(self->u.string));

    while (posargs) {
	if (! IS_LIST_NULL(posargs)) {
	    cell_add_str(o, to_string_call(interp, list_get_item(posargs)));
	}

	posargs = list_next(posargs);
    }

    return new_string_cell(o);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at '.', syntax: String . [val ...]", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_match(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self, *tpattern;
    int r;
    unsigned char *start, *range, *end;
    regex_t *reg = NULL;
    OnigErrorInfo einfo;
    OnigRegion *region;
    unsigned char *pattern;
    unsigned char *str;
    Toy_Type *result, *resultl;
    int offs;
    int option, soption;
    Toy_Type *t_case = NULL;
    Toy_Type *t_all = NULL;
    Toy_Type *t_grep = NULL;
    Toy_Type *regex_hash_t;
    static Hash *regex_hash = NULL;
    Cell *key_str;
    Toy_Type *container;

    if (arglen > 1) goto error;

    self = SELF(interp);
    if (GET_TAG(self) != STRING) goto error2;

    tpattern = list_get_item(posargs);
    if (GET_TAG(tpattern) != RQUOTE) goto error;
    if (cell_get_length(tpattern->u.rquote) <= 0) {
	return new_exception(TE_REGEX, L"Null regex pattern.", interp);
    }

    /* parse option */
    t_case = hash_get_and_unset_t(nameargs, const_nocase);
    if ((t_case == NULL) || (IS_NIL(t_case))) t_case = NULL;

    t_all = hash_get_and_unset_t(nameargs, const_all);
    if ((t_all == NULL) || (IS_NIL(t_all))) t_all = NULL;

    t_grep = hash_get_and_unset_t(nameargs, const_grep);
    if ((t_grep == NULL) || (IS_NIL(t_grep))) t_grep = NULL;
    
    if (hash_get_length(nameargs) > 0) goto error;

    regex_hash_t = hash_get(interp->globals, const_regex_cache);
    if (NULL == regex_hash_t) {
	/* onece create regex cache */
	if (NULL == regex_hash) {
	    regex_hash = new_hash();
	}
    } else {
	regex_hash = regex_hash_t->u.dict;
    }

    /* make regex key, string format is: 'regex',[case|nocase],[default|grep] */
    key_str = new_cell(L"");
    cell_add_str(key_str, L"\'");
    cell_add_str(key_str, to_string(tpattern));
    cell_add_str(key_str, L"\'");
    cell_add_str(key_str, L",");
    if (t_case) {
	cell_add_str(key_str, L"nocase");
    } else {
	cell_add_str(key_str, L"case");
    }
    cell_add_str(key_str, L",");
    if (t_grep) {
	cell_add_str(key_str, L"grep");
    } else {
	cell_add_str(key_str, L"default");
    }

    /* regex cache search */
    container = hash_get_t(regex_hash, new_symbol(cell_get_addr(key_str)));
    if (NULL == container) {
	reg = NULL;
    } else if (GET_TAG(container) != CONTAINER) {
	reg = NULL;
    } else {
	/* cache hit! */
	reg = (regex_t*)container->u.container;
    }
    
    /* convert *wchar_t to UTF32-LE char stream data pointer */
    str = (unsigned char*)(cell_get_addr(self->u.string));
    pattern = (unsigned char*)(cell_get_addr(tpattern->u.rquote));

    option = ONIG_OPTION_NONE;
    if (t_case) {
	option |= ONIG_OPTION_IGNORECASE;
    }

    if (NULL == reg) {
	/* no cache then create regex object */
	r = onig_new(&reg,
		     pattern,
		     pattern + (cell_get_length(tpattern->u.rquote) * sizeof(wchar_t)),
		     option,
		     ONIG_ENCODING_UTF32_LE,
		     (t_grep == NULL) ? ONIG_SYNTAX_DEFAULT : ONIG_SYNTAX_GREP,
		     &einfo
	    );

	if (r != ONIG_NORMAL) {
	    OnigUChar s[ONIG_MAX_ERROR_MESSAGE_LEN];
	    onig_error_code_to_str(s, r, &einfo);
	    return new_exception(TE_REGEX, to_wchar((char*)s), interp);
	}

	/* and regex object set to cache */
	hash_set_t(regex_hash, new_symbol(cell_get_addr(key_str)), new_container(reg));
    }

    region = onig_region_new();

    end = str + (cell_get_length(self->u.string) * sizeof(wchar_t));
    start = str;
    range = end;
    offs = 0;
    resultl = result = new_list(NULL);
    soption = ONIG_OPTION_NONE;

next_search:
    r = onig_search(reg, str, end, start, range, region, soption);
    soption |= ONIG_OPTION_NOTBOL;

    if (r >= 0) {
	int i, n;
	Toy_Type *l, *ll;
	int max;

	max = -1;
	for (i=0; i<region->num_regs; i++) {
	    if (! ((region->beg[i] >= 0) && (region->end[i] >= 0))) continue;

	    ll = l = new_list(NULL);
	    l = list_append(l, new_integer_si((region->beg[i] + offs)/sizeof(wchar_t)));
	    l = list_append(l, new_integer_si((region->end[i] + offs)/sizeof(wchar_t)));
	    l = list_append(l, new_string_cell(cell_sub(self->u.string,
							(region->beg[i] + offs)/sizeof(wchar_t),
							(region->end[i] + offs)/sizeof(wchar_t))));
	    n = region->end[i];
	    if (n > max) max = n;
	    resultl = list_append(resultl, ll);
	}
	offs += max;
	onig_region_free(region, 1);
	region = onig_region_new();
	str += max;

	start += max;
	if ((start < end) && (max > 0) && t_all) goto next_search;

    } else if (r == ONIG_MISMATCH) {
	if (IS_LIST_NULL(result)) {
	    result = const_Nil;
	}
    } else {
	OnigUChar s[ONIG_MAX_ERROR_MESSAGE_LEN];
	onig_error_code_to_str(s, r, &einfo);
	result = new_exception(TE_REGEX, to_wchar((char*)s), interp);
    }

    onig_region_free(region, 1);
    /* omit onig regex object free by regex cache impliment */
    // onig_free(reg);
    onig_end();

    return result;

error:
    return new_exception(TE_SYNTAX,
	 L"Syntax error at '=~', syntax: String =~ [:nocase] [:all] [:grep] 'pattern'", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_sub(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;
    Cell *o;
    int start=0, end=0;
    Toy_Type *t;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 2) goto error;

    t = list_get_item(posargs);
    if (GET_TAG(t) != INTEGER) goto error;
    start = mpz_get_si(t->u.biginteger);

    self = SELF(interp);
    if (arglen == 2) {
	posargs = list_next(posargs);
	t = list_get_item(posargs);
	if (GET_TAG(t) != INTEGER) goto error;
	end = mpz_get_si(t->u.biginteger);
    } else {
	end = cell_get_length(self->u.string);
    }

    if (GET_TAG(self) != STRING) goto error2;

    o = self->u.string;

    return new_string_cell(cell_sub(o, start, end));

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'sub', syntax: String sub start [end]", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

static int
is_sub_eq(wchar_t *src, wchar_t *dest);

Toy_Type*
mth_string_split(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *sep = NULL;
    Toy_Type *self, *result, *l;
    wchar_t *p, *csep;
    Cell *word;
    int seplen, end_f=0;

    if (arglen > 0) goto error;

    sep = hash_get_and_unset_t(nameargs, const_sep);
    if (hash_get_length(nameargs) > 0) goto error;
    if (sep && (GET_TAG(sep) != STRING)) goto error;

    self = SELF(interp);
    if (GET_TAG(self) != STRING) goto error2;

    l = result = new_list(NULL);
    p = cell_get_addr(self->u.string);

    if (NULL == sep) {

	while (*p) {
	    word = new_cell(L"");

	    /* skip white space */
	    while (*p && wcisspace(*p)) {
		p++;
	    }
	    if (! *p) break;

	    /* collect word, until white space */
	    while (*p) {
		if (! wcisspace(*p)) {
		    cell_add_char(word, *p);
		} else {
		    break;
		}
		p++;
	    }

	    /* end condition */
	    if (! *p) {
		if (cell_get_length(word) != 0) {
		    l = list_append(l, new_string_cell(word));
		}
		break;
	    }
	    if (cell_get_length(word) == 0) break;

	    l = list_append(l, new_string_cell(word));
	}

    } else {

	csep = cell_get_addr(sep->u.string);
	seplen = wcslen(csep);
	
	while (*p) {
	    word = new_cell(L"");

	    /* collect word, until separator */
	    end_f = 0;
	    while (*p) {
		if (! is_sub_eq(p, csep)) {
		    cell_add_char(word, *p);
		} else {
		    end_f = 1;
		    if (0 == seplen) {
			cell_add_char(word, *p);
		    }
		    break;
		}
		p++;
	    }
	    l = list_append(l, new_string_cell(word));

	    /* end condition */
	    if (! *p) {
		break;
	    }

	    p += ((seplen == 0) ? 1 : seplen);
	}
	if ((end_f == 1) && (seplen != 0)) {
	    l = list_append(l, new_string_str(L""));
	}
    }

    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'split', syntax: String split [sep: separator]", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

static int
is_sub_eq(wchar_t *src, wchar_t *dest) {
    if (! *dest) return 1;
    
    while (*dest) {
	if (! *src) return 0;
	if (*src != *dest) return 0;
	src++; dest++;
    }

    return 1;
}

Toy_Type*
mth_string_toint(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *result;

    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != STRING) goto error2;

    result = toy_symbol_conv(new_symbol(cell_get_addr(SELF(interp)->u.string)));
    if (GET_TAG(result) == EXCEPTION) return result;
    if (GET_TAG(result) == INTEGER) return result;
    result = to_int(interp, result);
    if (GET_TAG(result) == INTEGER) return result;
    goto error2;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'int', syntax: String int", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_toreal(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *result;

    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != STRING) goto error2;

    result = toy_symbol_conv(new_symbol(cell_get_addr(SELF(interp)->u.string)));
    if (GET_TAG(result) == INTEGER) {
	return new_real(mpz_get_d(result->u.biginteger));
    }
    if (GET_TAG(result) == EXCEPTION) return result;
    if (GET_TAG(result) == REAL) return result;    
    result =  to_real(interp, result);
    if (GET_TAG(result) == REAL) return result;
    goto error2;
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'real', syntax: String real", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_tonumber(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *result;

    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != STRING) goto error2;

    result = toy_symbol_conv(new_symbol(cell_get_addr(SELF(interp)->u.string)));
    if ((GET_TAG(result) == INTEGER) || (GET_TAG(result) == REAL))
	return result;
    return const_Nil;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'number', syntax: String number", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_torquote(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != STRING) goto error2;

    return new_rquote(cell_get_addr(SELF(interp)->u.string));

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'rquote', syntax: String rquote", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

static wchar_t*
mth_string_format_fill(wchar_t* item, int fill, int trim) {
    Cell *result, *result2;
    int i, slen;

    slen = wcslen(item);

    result = new_cell(L"");
    if ((fill == 0) || (slen >= abs(fill))) {
	cell_add_str(result, item);
    } else if (fill < 0) {
	/* lef align */
	cell_add_str(result, item);
	for (i=0 ; i<(-fill-slen) ; i++) {
	    cell_add_char(result, L' ');
	}
    } else if (fill > 0) {
	/* right align */
	for (i=0 ; i<(fill-slen) ; i++) {
	    cell_add_char(result, L' ');
	}
	cell_add_str(result, item);
    }

    if (fill && trim) {
	result2 = new_cell(cell_get_addr(result));
	if (cell_get_length(result2) > abs(fill)) {
	    cell_get_addr(result2)[abs(fill)] = 0;
	}
	return cell_get_addr(result2);
    }

    /* otherwise aling only */
    return cell_get_addr(result);
}

static wchar_t*
mth_string_format_C(Toy_Type *item, Cell *fmt) {
    wchar_t *buff;

    buff = GC_MALLOC(64*sizeof(wchar_t));
    ALLOC_SAFE(buff);

    switch (GET_TAG(item)) {
    case INTEGER:
	/* XXX: fix it for big integer */
	swprintf(buff, 64, cell_get_addr(fmt),
		 mpz_get_si(item->u.biginteger));
	break;
    case REAL:
	swprintf(buff, 64, cell_get_addr(fmt),
		 item->u.real);
	break;
    default:
	return 0;
    }

    return buff;
}

Toy_Type*
mth_string_format(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    wchar_t *p, *f;
    int acc, neg, trim;
    int done, indicate_int;
    Toy_Type *result, *item;
    Cell *c, *sacc;

    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != STRING) goto error2;

    p = cell_get_addr(SELF(interp)->u.string);

    result = new_string_str(L"");
    c = result->u.string;
    while (*p) {
	switch (*p) {
	case L'%':
	    acc = 0;
	    neg = 1;
	    trim = 0;
	    done = 0;
	    sacc = new_cell(L"%");
	    indicate_int = 0;
	    p++;
	    if (! *p) {
		return result;
	    }
	    while (*p) {
		switch (*p) {
		case L'%':
		    cell_add_char(c, L'%');
		    done = 1;
		    break;

		case L'-':
		    neg = -1;
		    cell_add_char(sacc, *p);
		    break;

		case L'!':
		    trim = 1;
		    break;
		    
		case L'0': case L'1': case L'2': case L'3': case L'4':
		case L'5': case L'6': case L'7': case L'8': case L'9':
		    acc = acc*10 + (*p-L'0');
		    cell_add_char(sacc, *p);
		    break;

		case L'.':
		    cell_add_char(sacc, *p);
		    acc = 0;
		    break;

		case L'v':
		    item = list_get_item(posargs);
		    if (NULL == item) {
			item = const_nullstring;
		    } else {
			posargs = list_next(posargs);
		    }
		    cell_add_str(c, mth_string_format_fill(to_string_call(interp, item), acc * neg, trim));
		    done = 1;
		    break;

		case L'd':
		case L'o': case L'u': case L'x': case L'X':
		    indicate_int = 1;
		    cell_add_char(sacc, L'l');
		    cell_add_char(sacc, L'l');
		    /* fall thru */

		case L'f': case L'F':
		case L'e': case L'E':
		case L'g': case L'G':
		case L'a': case L'A':
		    cell_add_char(sacc, *p);

		    item = list_get_item(posargs);
		    if (NULL == item) {
			item = const_nullstring;
		    } else {
			posargs = list_next(posargs);
		    }
		    if ((GET_TAG(item) == INTEGER) && (! indicate_int)) {
			return new_exception(TE_TYPE, L"Format indicator type error.", interp);
		    }
		    if ((GET_TAG(item) == REAL) && indicate_int) {
			return new_exception(TE_TYPE, L"Format indicator type error.", interp);
		    }
		    f = mth_string_format_C(item, sacc);
		    if (f) {
			cell_add_str(c, f);
		    } else {
			return new_exception(TE_TYPE, L"Format indicator type error.", interp);
		    }
		    done = 1;
		    break;

		default:
		    return new_exception(TE_SYNTAX, L"Bad format indicator.", interp);
		}

		if (done) break;
		p++;
	    }
	    break;

	default:
	    if (*p) {
		cell_add_char(c, *p);
	    }
	}

	if (! *p) break;
	p++;
    }
    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'fmt', syntax: String fmt var ...", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_clean(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    wchar_t *p;
    Cell *c;
    int len, i;
    
    if (GET_TAG(SELF(interp)) != STRING) goto error2;
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    c = new_cell(L"");
    p = cell_get_addr(SELF(interp)->u.string);

    while (*p) {
	if (wcisspace(*p)) {
	    p++;
	} else {
	    break;
	}
    }

    len = wcslen(p);
    for (i=len-1; i>=0; i--) {
	if (wcisspace(p[i])) {
	    p[i]=0;
	} else {
	    break;
	}
    }
    
    while (*p) {
	if ((! wcisspace(*p)) || (*p == L' ') || (*p == L'\t')) {
	    if (wcisprint(*p) || (*p == L'\t')) {
		cell_add_char(c, *p);
	    }
	} else {
	    cell_add_char(c, L' ');
	}
	p++;
    }

    return new_string_cell(c);
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'clean', syntax: String clean", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_upper(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    wchar_t *p;
    Cell *c;
    
    if (GET_TAG(SELF(interp)) != STRING) goto error2;
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    c = new_cell(L"");
    p = cell_get_addr(SELF(interp)->u.string);
    while (*p) {
	if (islower(*p)) {
	    cell_add_char(c, toupper(*p));
	} else {
	    cell_add_char(c, *p);
	}
	p++;
    }

    return new_string_cell(c);
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'upper', syntax: String upper", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_lower(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    wchar_t *p;
    Cell *c;
    
    if (GET_TAG(SELF(interp)) != STRING) goto error2;
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    c = new_cell(L"");
    p = cell_get_addr(SELF(interp)->u.string);
    while (*p) {
	if (isupper(*p)) {
	    cell_add_char(c, tolower(*p));
	} else {
	    cell_add_char(c, *p);
	}
	p++;
    }

    return new_string_cell(c);
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'lower', syntax: String lower", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_uexport(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *l;
    Toy_Type *result, *r;
    int i;
    wchar_t *p;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 0) goto error;

    l = SELF(interp);
    if (GET_TAG(l) != STRING) goto error2;
    
    result = r = new_list(NULL);
    
    p = cell_get_addr(l->u.string);
    for (i=0; i<cell_get_length(l->u.string); i++) {
	r = list_append(r, new_integer_si(p[i]));
    }

    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'uexport', syntax: String uexport", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_uimport(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *l, *result, *i;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    result = SELF(interp);
    if (GET_TAG(result) != STRING) goto error2;

    l = list_get_item(posargs);
    if (GET_TAG(l) != LIST) goto error;
    
    while (! IS_LIST_NULL(l)) {
	i = list_get_item(l);
	if (GET_TAG(i) != INTEGER) goto error;
	cell_add_char(result->u.string, mpz_get_si(i->u.biginteger));
	l = list_next(l);
    }

    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'uimport!', syntax: String uimport! (unicode ...)", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_string_at(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;
    Toy_Type *result;
    Toy_Type *at;
    int iat;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != STRING) goto error2;

    result = new_string_str(L"");

    at = list_get_item(posargs);
    if ((GET_TAG(at) != INTEGER)) goto error;
    iat = mpz_get_si(at->u.biginteger);
    
    if (iat < 0) {
	iat = cell_get_length(self->u.string) + iat;
    }
    if (iat < 0) return result;
    if (iat >= cell_get_length(self->u.string)) return result;
    cell_add_char(result->u.string, cell_get_addr(self->u.string)[iat]);
    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'at', syntax: String at pos", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_block_eval(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *closure, *result;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 2) goto error;
    if (GET_TAG(SELF(interp)) != CLOSURE) goto error2;

    closure = SELF(interp);
    result = eval_closure(interp, closure, interp->trace_info);
    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'eval', syntax: Block eval", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

#define FMODE_INPUT	(1)
#define FMODE_OUTPUT	(2)
#define FMODE_APPEND	(3)
#define FMODE_INOUT	(4)

typedef struct _toy_file {
    FILE *fd;
    int mode;
    int newline;
    Toy_Type *path;
    Cell *r_pending;
    int noblock;
    int input_encoding;
    int output_encoding;
} Toy_File;

void
file_finalizer(void *obj, void *client_data) {
    Toy_File *o;
    
    o = (Toy_File*)obj;

    if (o->fd) {
	fclose(o->fd);
	o->fd = NULL;
	o->path = NULL;
	o->r_pending = NULL;
    }

    return;
}

static Toy_File*
new_file() {
    Toy_File *o;

    o = GC_MALLOC(sizeof(Toy_File));
    ALLOC_SAFE(o);
    memset(o, 0, sizeof(Toy_File));

    GC_register_finalizer_ignore_self((void*)o,
				      file_finalizer,
				      NULL,
				      NULL,
				      NULL);

/*
    GC_register_finalizer((void*)o,
			  file_finalizer,
			  NULL,
			  NULL,
			  NULL);
*/

    return o;
}

Toy_Type*
mth_file_init(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    Toy_File *f;
    Toy_Type *enc;
    int iencoder;

    self = SELF_HASH(interp);
    f = new_file();
    f->input_encoding = NENCODE_RAW;
    f->output_encoding = NENCODE_RAW;
    hash_set_t(self, const_Holder, new_container(f));

    enc = hash_get_t(interp->globals, const_DEFAULT_FILE_ENCODING);
    if (enc) {
	if (GET_TAG(enc) == SYMBOL) {
	    iencoder = get_encoding_index(cell_get_addr(enc->u.symbol.cell));
	    if (-1 == iencoder) {
		return new_exception(TE_BADENCODER, L"Bad encoder specified.", interp);
	    }
	    f->input_encoding = iencoder;
	    f->output_encoding = iencoder;
	} else {
	    return new_exception(TE_BADENCODER, L"Bad encoder specified, need symbol.", interp);
	}
    }

    if ((arglen > 0) && (LIST == GET_TAG(posargs))) {
	Toy_Type *cmd, *l;

	l = cmd = new_list(new_symbol(L"open"));
	while (posargs) {
	    list_append(l, list_get_item(posargs));
	    posargs = list_next(posargs);
	}
	return toy_call(interp, cmd);
    }

    return const_T;
}

Toy_Type*
mth_file_open(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    Toy_File *f;
    Toy_Type *container;
    Toy_Type *mode, *path;
    wchar_t *pmode;
    int flag;
    char *cpath;
    encoder_error_info *error_info;

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    f = container->u.container;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 1) goto error;

    mode = hash_get_t(nameargs, const_mode);

    if (mode == NULL) {
	f->mode = FMODE_INPUT;
    } else {
	if (GET_TAG(mode) != SYMBOL) goto error;

	pmode = cell_get_addr(mode->u.symbol.cell);
	if (wcscmp(pmode, L"i") == 0) {
	    f->mode = FMODE_INPUT;
	    f->newline = 0;
	} else if (wcscmp(pmode, L"o") == 0) {
	    f->mode = FMODE_OUTPUT;
	    f->newline = 1;
	} else if (wcscmp(pmode, L"a") == 0) {
	    f->mode = FMODE_APPEND;
	    f->newline = 1;
	} else if (wcscmp(pmode, L"io") == 0) {
	    f->mode = FMODE_INOUT;
	    f->newline = 1;
	} else goto error;
    }

    path = list_get_item(posargs);
    if (GET_TAG(path) != STRING) goto error;

    f->path = path;

    if (f->fd) {
	fclose(f->fd);
	f->fd = NULL;
    }

    cpath = encode_dirent(interp, cell_get_addr(path->u.string), &error_info);
    if (NULL == cpath) {
	return new_exception(TE_BADENCODER, error_info->message, interp);
    }

    f->fd = fopen(cpath,
		  (f->mode==FMODE_INPUT)  ? "r"  :
		  (f->mode==FMODE_OUTPUT) ? "w"  :
		  (f->mode==FMODE_INOUT)  ? "r+" :
		   "a");

    if (NULL == f->fd) {
	return new_exception(TE_FILENOTOPEN, to_wchar(strerror(errno)), interp);
    }

    flag = fcntl(fileno(f->fd), F_GETFD, 0);
    if (flag >= 0) {
	flag |= FD_CLOEXEC;
	fcntl(fileno(f->fd), F_SETFD, flag);
    }

    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'open', syntax: File oepn [mode: i | o | a | io] \"path\"", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_file_close(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    Toy_File *f;
    Toy_Type *container;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    f = container->u.container;

    if (f->fd) {
	fclose(f->fd);
	f->fd = NULL;
	f->path = NULL;
	f->mode = 0;
    }

    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'close', syntax: File close", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_file_gets(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    Toy_File *f;
    Toy_Type *container;
    Cell *cbuff;
    int c;
    int flag_nonewline=0, flag_nocontrol=0;
    encoder_error_info *enc_error_info;

    if (arglen > 0) goto error;

    enc_error_info = GC_MALLOC(sizeof(encoder_error_info));
    ALLOC_SAFE(enc_error_info);
    
    if (hash_get_and_unset_t(nameargs, const_nonewline)) {
	flag_nonewline = 1;
    }
    if (hash_get_and_unset_t(nameargs, const_nocontrol)) {
	flag_nocontrol = 1;
    }
    if (hash_get_length(nameargs) > 0) goto error;

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    f = container->u.container;

    if (0 == f->newline) {
	flag_nonewline = 1;
    }

    if (NULL == f->fd) {
	return new_exception(TE_FILEACCESS, L"File not open.", interp);
    }
    if ((FMODE_INPUT != f->mode) && (FMODE_INOUT != f->mode)) {
	return new_exception(TE_FILEACCESS, L"Bad file access mode.", interp);
    }

    if (feof(f->fd)) return const_Nil;

    if (f->r_pending) {
	cbuff = new_cell(cell_get_addr(f->r_pending));
	f->r_pending = NULL;
    } else {
	cbuff = new_cell(L"");
    }
    
    while (1) {
	c = fgetc(f->fd);
	if (EOF == c) {
	    if ((errno == EAGAIN) && (! feof(f->fd))) {
		clearerr(f->fd);
		f->r_pending = cbuff;
		return new_exception(TE_IOAGAIN, L"No data available at File::gets, try again.", interp);
	    }
	    
	    f->r_pending = NULL;
	    if (cell_get_length(cbuff) == 0) {
		return const_Nil;
	    } else {
		Cell *c = decode_raw_to_unicode(cbuff, f->input_encoding, enc_error_info);
		if (NULL == c) {
		    return new_exception(TE_BADENCODEBYTE, enc_error_info->message, interp);
		}
		return new_string_cell(c);
	    }
	}
	
	if (('\n' == c) || ('\r' == c)) {
	    if (! flag_nonewline) {
		if (! flag_nocontrol) {
		    cell_add_char(cbuff, c);
		}
	    }
	} else {
	    if (wcisprint(c)) {
		cell_add_char(cbuff, c);
	    } else {
		if (! flag_nocontrol) {
		    cell_add_char(cbuff, c);
		}
	    }
	}
	
	if ('\n' == c) {
	    Cell *c = decode_raw_to_unicode(cbuff, f->input_encoding, enc_error_info);
	    if (NULL == c) {
		return new_exception(TE_BADENCODEBYTE, enc_error_info->message, interp);
	    }
	    return new_string_cell(c);
	}
    }

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'gets', syntax: File gets [:nonewline] [:nocontrol]", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_file_puts(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    Toy_File *f;
    Toy_Type *container;
    int c;
    int flag_nonewline = 0;
    wchar_t *p;
    Cell *unicode, *raw;
    encoder_error_info *enc_error_info;

    if (arglen == 0) goto error;

    enc_error_info = GC_MALLOC(sizeof(encoder_error_info));
    ALLOC_SAFE(enc_error_info);

    if (hash_get_and_unset_t(nameargs, const_nonewline)) {
	flag_nonewline = 1;
    }
    if (hash_get_length(nameargs) > 0) goto error;

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    f = container->u.container;

    if ((f->newline == 1) && (flag_nonewline == 0)) {
	flag_nonewline = 0;
    } else {
	flag_nonewline = 1;
    }

    if (NULL == f->fd) {
	return new_exception(TE_FILEACCESS, L"File not open.", interp);
    }
    if ((FMODE_OUTPUT != f->mode) && (FMODE_APPEND != f->mode) && (FMODE_INOUT != f->mode)) {
	return new_exception(TE_FILEACCESS, L"Bad file access mode.", interp);
    }

    while (posargs) {
	unicode = new_cell(to_string_call(interp, list_get_item(posargs)));
	raw = encode_unicode_to_raw(unicode, f->output_encoding, enc_error_info);
	if (NULL == raw) {
	    return new_exception(TE_BADENCODEBYTE, enc_error_info->message, interp);
	}
	p = cell_get_addr(raw);
	c = fputs(to_char(p), f->fd);
	if (flag_nonewline == 0) {
	    c = fputs("\n", f->fd);
	}
	fflush(f->fd);

	if (EOF == c) {
	    return new_exception(TE_FILEACCESS, to_wchar(strerror(errno)), interp);
	}

	posargs = list_next(posargs);
    }

    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'puts', syntax: File puts [:nonewline] val ...", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_file_flush(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    Toy_File *f;
    Toy_Type *container;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    f = container->u.container;

    if (NULL == f->fd) {
	return new_exception(TE_FILEACCESS, L"File not open.", interp);
    }
    if ((FMODE_OUTPUT != f->mode) && (FMODE_APPEND != f->mode) && (FMODE_INOUT != f->mode)) {
	return new_exception(TE_FILEACCESS, L"Bad file access mode.", interp);
    }

    fflush(f->fd);

    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'flush', syntax: File flush", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_file_setnewline(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    Toy_File *f;
    Toy_Type *container, *flag;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    f = container->u.container;

    flag = list_get_item(posargs);
    if (IS_NIL(flag)) {
	f->newline = 0;
	return const_Nil;
    } else {
	f->newline = 1;
	return const_T;
    }
	
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'set-newline', syntax: File set-newline [t | nil]", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_file_stat(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    Toy_File *f;
    Toy_Type *container;
    Toy_Type *l, *mode;
    wchar_t *enc;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    f = container->u.container;

    l = new_list(NULL);

    list_append(l, new_cons(new_symbol(L"fd"),
			    f->fd ? new_integer_si(fileno(f->fd)) : const_Nil));
    switch (f->mode) {
    case FMODE_INPUT:
	mode = new_symbol(L"i");
	break;
    case FMODE_OUTPUT:
	mode = new_symbol(L"o");
	break;
    case FMODE_APPEND:
	mode = new_symbol(L"a");
	break;
    case FMODE_INOUT:
	mode = new_symbol(L"io");
	break;
    default:
	mode = const_Nil;
    }
    list_append(l, new_cons(new_symbol(L"mode"), mode));
    list_append(l, new_cons(new_symbol(L"path"), f->path ? f->path : const_Nil));
    if (f->fd) {
	if (feof(f->fd)) {
	    list_append(l, new_cons(new_symbol(L"eof"), const_T));
	} else {
	    list_append(l, new_cons(new_symbol(L"eof"), const_Nil));
	}
    } else {
	list_append(l, new_cons(new_symbol(L"eof"), const_Nil));
    }
    list_append(l, new_cons(new_symbol(L"newline"), f->newline ? const_T : const_Nil));
    list_append(l, new_cons(new_symbol(L"noblock"), f->noblock ? const_T : const_Nil));
    enc = get_encoding_name(f->input_encoding);
    list_append(l, new_cons(new_symbol(L"input-encoding"), new_symbol(enc?enc:L"(BAD ENCODING)")));
    enc = get_encoding_name(f->output_encoding);
    list_append(l, new_cons(new_symbol(L"output-encoding"), new_symbol(enc?enc:L"(BAD ENCODING)")));

    return l;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'stat', syntax: File stat", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_file_iseof(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    Toy_File *f;
    Toy_Type *container;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    f = container->u.container;

    if (NULL == f->fd) {
	return new_exception(TE_FILEACCESS, L"File not open.", interp);
    }

    if (feof(f->fd)) {
	return const_T;
    } else {
	return const_Nil;
    }


error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'eof?', syntax: File eof?", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_file_getfd(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    Toy_File *f;
    Toy_Type *container;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    f = container->u.container;

    if (NULL == f->fd) {
	return new_exception(TE_FILEACCESS, L"File not open.", interp);
    }

    return new_integer_si(fileno(f->fd));

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'fd?', syntax: File fd?", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_file_set(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    Toy_File *f;
    Toy_Type *container;
    Toy_Type *tfd;
    Toy_Type *mode;
    char *imode;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 1) goto error;

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    f = container->u.container;

    tfd = list_get_item(posargs);
    if (GET_TAG(tfd) != INTEGER) goto error;
    mode = hash_get_t(nameargs, const_mode);
    
    if (NULL == mode) {
	imode = "r";
	f->mode = FMODE_INPUT;
    } else if (GET_TAG(mode) != SYMBOL) {
	goto error;
    } else if (wcscmp(cell_get_addr(mode->u.symbol.cell), L"i") == 0) {
	imode = "r";
	f->mode = FMODE_INPUT;
    } else if (wcscmp(cell_get_addr(mode->u.symbol.cell), L"o") == 0) {
	imode = "w";
	f->mode = FMODE_OUTPUT;
    } else if (wcscmp(cell_get_addr(mode->u.symbol.cell), L"a") == 0) {
	imode = "a";
	f->mode = FMODE_APPEND;
    } else if (wcscmp(cell_get_addr(mode->u.symbol.cell), L"io") == 0) {
	imode = "r+";
	f->mode = FMODE_INOUT;
    } else goto error;
	
    f->fd = fdopen(mpz_get_si(tfd->u.biginteger), imode);
    f->path = const_Nil;

    if (NULL == f->fd) {
	return new_exception(TE_FILENOTOPEN, to_wchar(strerror(errno)), interp);
    }

    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'set!', syntax: File set! [mode: iomode] integer-fd", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_file_isready(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    Toy_Type *container;
    Toy_File *f;
    FILE *fd;
    int fdesc;
    int sts;
    fd_set fds;
    struct timeval timeout;
    int itimeout = 0;
    Toy_Type *ttimeout;

    if (arglen > 0) goto error;
    ttimeout = hash_get_and_unset_t(nameargs, const_timeout);
    if (ttimeout) {
	if (GET_TAG(ttimeout) != INTEGER) goto error;
	itimeout = mpz_get_si(ttimeout->u.biginteger);
    }
    if (hash_get_length(nameargs) > 0) goto error;

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    f = container->u.container;

    fd = f->fd;
    if (NULL == fd) return const_Nil;

    if (feof(f->fd)) return const_Nil;
    
    fdesc = fileno(fd);
    if (fdesc < 0) return const_Nil;

    FD_ZERO(&fds);
    FD_SET(fdesc, &fds);
    timeout.tv_sec = itimeout / 1000;
    timeout.tv_usec = (itimeout % 1000) * 1000;

    switch (f->mode) {
    case FMODE_INPUT:
    case FMODE_INOUT:	/* XXX: fix me!! */
	sts = select(fdesc+1, &fds, NULL, NULL, &timeout);
	break;

    case FMODE_OUTPUT:
    case FMODE_APPEND:
	sts = select(fdesc+1, NULL, &fds, NULL, &timeout);
	break;

    default:
	goto error2;
    }

    if (0 == sts) return const_Nil;
    if (FD_ISSET(fdesc, &fds)) return const_T;
    return const_Nil;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'ready?', syntax: File ready? [timeout: msec]", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_file_clear(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    Toy_Type *container;
    Toy_File *f;
    FILE *fd;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    f = container->u.container;

    fd = f->fd;
    if (NULL == fd) goto error2;

    clearerr(fd);
    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'clear', syntax: File clear", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_file_setnobuffer(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    Toy_Type *container;
    Toy_File *f;
    FILE *fd;

    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    f = container->u.container;
    fd = f->fd;
    if (EOF == setvbuf(fd, 0, _IOLBF, 0)) {
	return new_exception(TE_FILEACCESS, L"Buffering mode change error.", interp);
    };

    return const_T;
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'set-nobuffer', syntax: File set-nobuffer", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_file_setnoblock(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    Toy_Type *container, *flag;
    Toy_File *f;
    FILE *fd;
    int iflag = 0, val;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    flag = list_get_item(posargs);
    if (IS_NIL(flag)) {
	iflag = 0;	// mark normal (read block)
    } else {
	iflag = 1;	// mark non-block (read non-block)
    }

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    f = container->u.container;
    fd = f->fd;

    val = fcntl(fileno(fd), F_GETFL, 0);
    if (-1 == val) {
	return new_exception(TE_FILEACCESS, L"fcntl error at F_GETFL.", interp);
    }
    if (iflag) {
	// set non-block
	val |= O_NONBLOCK;
    } else {
	// set block (normal)
	val &= ~O_NONBLOCK;
    }
    if (fcntl(fileno(fd), F_SETFL, val) == -1) {
	return new_exception(TE_FILEACCESS, L"fcntl error at F_SETFL.", interp);
    }
    
    if (iflag) {
	f->noblock = 1;
	return const_T;
    } else {
	f->noblock = 0;
	return const_Nil;
    }
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'set-noblock', syntax: File set-noblock t | nil", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_file_setencoding(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    Toy_Type *container;
    Toy_File *f;
    Toy_Type *enc;
    int enc_index;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    f = container->u.container;

    enc = list_get_item(posargs);
    if (GET_TAG(enc) != SYMBOL) goto error;
    
    enc_index = get_encoding_index(cell_get_addr(enc->u.symbol.cell));
    if (enc_index == -1) {
	return new_exception(TE_NOENCODING, L"Encoding not implimentation.", interp);
    }

    f->input_encoding = enc_index;
    f->output_encoding = enc_index;
    
    return enc;
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'set-encoding', syntax: File set-encoding encoding-name", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_file_setinputencoding(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    Toy_Type *container;
    Toy_File *f;
    Toy_Type *enc;
    int enc_index;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    f = container->u.container;

    enc = list_get_item(posargs);
    if (GET_TAG(enc) != SYMBOL) goto error;
    
    enc_index = get_encoding_index(cell_get_addr(enc->u.symbol.cell));
    if (enc_index == -1) {
	return new_exception(TE_NOENCODING, L"Encoding not implimentation.", interp);
    }

    f->input_encoding = enc_index;
    
    return enc;
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'set-input-encoding', syntax: File set-input-encoding encoding-name", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_file_setoutputencoding(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    Toy_Type *container;
    Toy_File *f;
    Toy_Type *enc;
    int enc_index;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    f = container->u.container;

    enc = list_get_item(posargs);
    if (GET_TAG(enc) != SYMBOL) goto error;
    
    enc_index = get_encoding_index(cell_get_addr(enc->u.symbol.cell));
    if (enc_index == -1) {
	return new_exception(TE_NOENCODING, L"Encoding not implimentation.", interp);
    }

    f->output_encoding = enc_index;
    
    return enc;
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'set-output-encoding', syntax: File set-output-encoding encoding-name", interp);
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_dict_set(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *o, *val;
    wchar_t *key;

    if (arglen != 2) goto error;

    o = SELF(interp);
    if (GET_TAG(o) != DICT) goto error2;

    key = to_string_call(interp, list_get_item(posargs));
    val = list_get_item(list_next(posargs));

    hash_set(o->u.dict, key, toy_clone(val));

    return val;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'set', syntax: Dict set key value", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_dict_get(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *o, *result;
    wchar_t *key;

    if (arglen != 1) goto error;

    o = SELF(interp);
    if (GET_TAG(o) != DICT) goto error2;

    key = to_string_call(interp, list_get_item(posargs));

    result = hash_get(o->u.dict, key);
    if (result) {
	return result;
    }

    return const_Nil;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'get', syntax: Dict get key", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_dict_isset(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *o, *result;
    wchar_t *key;

    if (arglen != 1) goto error;

    o = SELF(interp);
    if (GET_TAG(o) != DICT) goto error2;

    key = to_string_call(interp, list_get_item(posargs));

    result = hash_get(o->u.dict, key);
    if (result) {
	return const_T;
    }

    return const_Nil;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'set?', syntax: Dict set? key", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_dict_len(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *o;

    if (arglen != 0) goto error;

    o = SELF(interp);
    if (GET_TAG(o) != DICT) goto error2;

    return new_integer_si(hash_get_length(o->u.dict));

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'len', syntax: Dict len", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_dict_stat(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *o;
    Toy_Type *result;

    if (arglen != 0) goto error;

    o = SELF(interp);
    if (GET_TAG(o) != DICT) goto error2;

    result = new_list(new_integer_si(hash_get_synonyms(o->u.dict)));
    list_append(result, new_integer_si(hash_get_bucketsize(o->u.dict)));
    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'stat', syntax: Dict stat", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_dict_unset(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *o, *result;
    wchar_t *key;

    if (arglen != 1) goto error;

    o = SELF(interp);
    if (GET_TAG(o) != DICT) goto error2;

    key = to_string_call(interp, list_get_item(posargs));

    result = hash_get_and_unset(o->u.dict, key);
    if (result) {
	return result;
    }

    return const_Nil;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'unset', syntax: Dict unset key", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_dict_keys(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *o;
    Toy_Type *string_flag;

    if (arglen != 0) goto error;

    string_flag = hash_get_and_unset_t(nameargs, const_string_key);
    if (hash_get_length(nameargs) != 0) goto error;

    o = SELF(interp);
    if (GET_TAG(o) != DICT) goto error2;

    if (string_flag && (! IS_NIL(string_flag))) {
	return hash_get_keys_str(o->u.dict);
    }
    return hash_get_keys(o->u.dict);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'keys', syntax: Dict keys [:string]", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_dict_pairs(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *o;
    Toy_Type *string_flag;

    if (arglen != 0) goto error;

    string_flag = hash_get_and_unset_t(nameargs, const_string_key);
    if (hash_get_length(nameargs) != 0) goto error;

    o = SELF(interp);
    if (GET_TAG(o) != DICT) goto error2;
    
    if (string_flag && (! IS_NIL(string_flag))) {
	return hash_get_pairs_str(o->u.dict);
    }
    return hash_get_pairs(o->u.dict);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'pairs', syntax: Dict pairs [:string]", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_vector_append(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *item;
    Toy_Type *o;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    item = list_get_item(posargs);
    o = SELF(interp);
    if (GET_TAG(o) != VECTOR) goto error2;

    array_append(o->u.vector, toy_clone(item));
    return item;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'append', syntax: Vector append val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_vector_set(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *index, *item;
    Toy_Type *o;

    if (arglen != 2) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    index  = list_get_item(posargs);
    if (GET_TAG(index) != INTEGER) goto error;
    posargs = list_next(posargs);
    item = list_get_item(posargs);

    o = SELF(interp);
    if (GET_TAG(o) != VECTOR) goto error2;

    if (! array_set(o->u.vector, toy_clone(item), mpz_get_si(index->u.biginteger))) {
	return new_exception(TE_ARRAYBOUNDARY, L"Array boundary error.", interp);
    }

    return item;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'set', syntax: Vector set pos val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_vector_get(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *index, *item;
    Toy_Type *o;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    index  = list_get_item(posargs);
    if (GET_TAG(index) != INTEGER) goto error;

    o = SELF(interp);
    if (GET_TAG(o) != VECTOR) goto error2;

    item = array_get(o->u.vector, mpz_get_si(index->u.biginteger));
    if (! item) {
	return new_exception(TE_ARRAYBOUNDARY, L"Array boundary error.", interp);
    }

    return item;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'get', syntax: Vector get pos", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_vector_len(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *o;

    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    o = SELF(interp);
    if (GET_TAG(o) != VECTOR) goto error2;

    return new_integer_si(array_get_size(o->u.vector));

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'len', syntax: Vector len", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_vector_last(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    int index;
    Toy_Type *o;

    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    o = SELF(interp);
    if (GET_TAG(o) != VECTOR) goto error2;

    index = array_get_size(o->u.vector);
    if (0 == index) return const_Nil;
    return array_get(o->u.vector, index - 1);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'last', syntax: Vector last", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_vector_list(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    int index, i;
    Toy_Type *result, *l;
    Toy_Type *o;

    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    o = SELF(interp);
    if (GET_TAG(o) != VECTOR) goto error2;

    index = array_get_size(o->u.vector);
    l = result = new_list(NULL);
    for (i=0; i<index; i++) {
	l = list_append(l, array_get(o->u.vector, i));
    }

    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'list', syntax: Vector list", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_vector_each(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    int t;
    Toy_Type *result;
    Toy_Type *o;
    Toy_Type *block;
    int pos;

    if (arglen != 0) goto error;

    o = SELF(interp);
    if (GET_TAG(o) != VECTOR) goto error2;

    block = hash_get_and_unset_t(nameargs, const_do);
    if (hash_get_length(nameargs) > 0) goto error;
    if ((block == NULL) || (GET_TAG(block) != CLOSURE)) goto error;

loop_retry:
    pos = 0;
    result = const_Nil;

loop:
    if (pos >= array_get_size(o->u.vector)) goto fin;
    result = toy_yield(interp, block,
		       new_list(array_get(o->u.vector, pos)));
    t = GET_TAG(result);
    if (t == EXCEPTION) return result;
    if (t == CONTROL) {
	switch (result->u.control.code) {
	case CTRL_RETURN: case CTRL_GOTO:
	    return result;
	    break;
	case CTRL_BREAK:
	    return result->u.control.ret_value;
	    break;
	case CTRL_CONTINUE:
	    result = const_Nil;
	    goto loop_continue;
	    break;
	case CTRL_REDO:
	    result = const_Nil;
	    goto loop;
	    break;
	case CTRL_RETRY:
	    result = const_Nil;
	    goto loop_retry;
	    break;
	}
    }

loop_continue:
    pos++;
    goto loop;

fin:
    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'list', syntax: Vector each do: {| var | block}", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_vector_swap(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *index1, *index2;
    Toy_Type *o;
    int idx1, idx2;

    if (arglen != 2) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    index1  = list_get_item(posargs);
    if (GET_TAG(index1) != INTEGER) goto error;
    posargs = list_next(posargs);

    index2  = list_get_item(posargs);
    if (GET_TAG(index2) != INTEGER) goto error;

    o = SELF(interp);
    if (GET_TAG(o) != VECTOR) goto error2;


    idx1 = mpz_get_si(index1->u.biginteger);
    idx2 = mpz_get_si(index2->u.biginteger);
    
    if (! array_swap(o->u.vector, idx1, idx2)) {
	return new_exception(TE_ARRAYBOUNDARY, L"Array boundary error.", interp);
    }

    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'swap', syntax: Vector swap pos1 pos2", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_vector_resize(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *new_size, *o;
    int size;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    new_size = list_get_item(posargs);
    if (GET_TAG(new_size) != INTEGER) goto error;

    o = SELF(interp);
    if (GET_TAG(o) != VECTOR) goto error2;

    size = mpz_get_si(new_size->u.biginteger);
    
    if (NULL == array_resize(o->u.vector, size)) {
	return new_exception(TE_ARRAYBOUNDARY, L"Bad array size specified.", interp);
    }

    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'resize', syntax: Vector resize new-size", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_coro_next(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;
    Toy_Coroutine *co;
    int restore_co_id = 0;

    self = SELF(interp);
    if (GET_TAG(self) != COROUTINE) goto error2;
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    co = self->u.coroutine;
    if (0 == co->coro_id) {
	return new_exception(TE_COOUTOFLIFE, L"Co-routine out of life.", interp);
    }
    if (! cstack_isalive(co->interp->cstack_id)) {
	return new_exception(TE_COOUTOFLIFE, L"Co-routine out of life.", interp);
    };

    interp->co_value = const_Nil;
    switch (co->state) {
    case CO_STS_INIT:
	co->state = CO_STS_RUN;
	/* fall thru */
    case CO_STS_RUN:
	interp->co_calling = 1;
	restore_co_id = cstack_enter(co->interp->cstack_id);
	co_call(co->coro_id);
	cstack_leave(restore_co_id);
	interp->co_calling = 0;
	break;

    default:
	return new_exception(TE_COOUTOFLIFE, L"Co-routine out of life.", interp);
    }

    if (interp->co_value) {
	return interp->co_value;
    }

    return const_Nil;
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'next', syntax: Coro next", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_coro_release(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;
    Toy_Coroutine *co;

    self = SELF(interp);
    if (GET_TAG(self) != COROUTINE) goto error2;
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    co = self->u.coroutine;
    co->state = CO_STS_DONE;

    if (0 == co->coro_id) {
	return new_exception(TE_COOUTOFLIFE, L"Co-routine is already done.", interp);
    }
    co_delete(co->coro_id);
    co->coro_id = 0;
    cstack_release_clear(co->interp->cstack_id);
    co->interp->cstack_id = 0;
    co->interp->cstack = NULL;
    co->interp = NULL;
    co->script = NULL;
    
    return const_Nil;
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'release', syntax: Coro release", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_coro_stat(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;
    Toy_Coroutine *co;

    self = SELF(interp);
    if (GET_TAG(self) != COROUTINE) goto error2;
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    co = self->u.coroutine;
    
    switch (co->state) {
    case CO_STS_INIT:
	return new_symbol(L"INIT");
    case CO_STS_RUN:
	return new_symbol(L"RUN");
    default:
	return new_symbol(L"DONE");
    }
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'stat', syntax: Coro stat", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_coro_eval(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self, *body;
    Toy_Coroutine *co;

    self = SELF(interp);
    if (GET_TAG(self) != COROUTINE) goto error2;
    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    co = self->u.coroutine;
    
    body = list_get_item(posargs);
    if (GET_TAG(body) != CLOSURE) goto error;

    if (CO_STS_DONE == co->state) {
	return new_exception(TE_COOUTOFLIFE, L"Co-routine out of life.", interp);
    }
    
    return toy_eval_script(co->interp, body->u.closure.block_body);
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'eval', syntax: Coro eval {body}", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_bulk_append(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self, *c;
    struct _binbulk *bulk;
    wchar_t v;
    
    self = SELF(interp);
    if (GET_TAG(self) != BULK) goto error2;
    bulk = self->u.bulk;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    c = list_get_item(posargs);
    
    if (GET_TAG(c) == LIST) {
	while (! IS_LIST_NULL(c)) {
	    Toy_Type *i;
	    i = list_get_item(c);
	    if (GET_TAG(i) != INTEGER) goto error;
	    binbulk_add_char(bulk, (wchar_t)mpz_get_si(i->u.biginteger));
	    c = list_next(c);
	}
	return const_T;
    }
    
    if (GET_TAG(c) != INTEGER) goto error;
    v = (wchar_t)mpz_get_si(c->u.biginteger);

    binbulk_add_char(bulk, v);

    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'append', syntax: Bulk append int-val | (int-val ...)", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_bulk_get(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;
    struct _binbulk *bulk;
    wchar_t v;

    self = SELF(interp);
    if (GET_TAG(self) != BULK) goto error2;
    bulk = self->u.bulk;

    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    
    if ((v = binbulk_get_char(bulk)) == -1) {
	return const_Nil;
    }
    return new_integer_si((int)v);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'get', syntax: Bulk get", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_bulk_set(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self, *c;
    struct _binbulk *bulk;
    wchar_t v;
    
    self = SELF(interp);
    if (GET_TAG(self) != BULK) goto error2;
    bulk = self->u.bulk;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    c = list_get_item(posargs);
    if (GET_TAG(c) != INTEGER) goto error;
    v = (wchar_t)mpz_get_si(c->u.biginteger);

    if (binbulk_set_char(bulk, v) == -1) {
	return const_Nil;
    }
    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'set', syntax: Bulk set int-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_bulk_seek(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self, *c;
    struct _binbulk *bulk;
    int v;
    
    self = SELF(interp);
    if (GET_TAG(self) != BULK) goto error2;
    bulk = self->u.bulk;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    c = list_get_item(posargs);
    if (GET_TAG(c) != INTEGER) goto error;
    v = mpz_get_si(c->u.biginteger);

    if ((v = binbulk_seek(bulk, v)) == -1) {
	return const_Nil;
    }

    return new_integer_si(v);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'seek', syntax: Bulk seek int-val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_bulk_position(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;
    struct _binbulk *bulk;
    int v;

    self = SELF(interp);
    if (GET_TAG(self) != BULK) goto error2;
    bulk = self->u.bulk;

    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    
    v = binbulk_get_position(bulk);
    return new_integer_si(v);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'position', syntax: Bulk position", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_bulk_len(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;
    struct _binbulk *bulk;
    int v;

    self = SELF(interp);
    if (GET_TAG(self) != BULK) goto error2;
    bulk = self->u.bulk;

    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    
    v = binbulk_get_length(bulk);
    return new_integer_si(v);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'len', syntax: Bulk len", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_bulk_capacity(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;
    struct _binbulk *bulk;
    int v;

    self = SELF(interp);
    if (GET_TAG(self) != BULK) goto error2;
    bulk = self->u.bulk;

    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    
    v = binbulk_get_capacity(bulk);
    return new_integer_si(v);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'capacity', syntax: Bulk capacity", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_bulk_truncate(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self, *c;
    struct _binbulk *bulk;
    int v;

    self = SELF(interp);
    if (GET_TAG(self) != BULK) goto error2;
    bulk = self->u.bulk;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    
    c = list_get_item(posargs);
    if (GET_TAG(c) != INTEGER) goto error;
    v = mpz_get_si(c->u.biginteger);

    if ((v = binbulk_truncate(bulk, v)) == -1) {
	return const_Nil;
    }
    return new_integer_si(v);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'truncate', syntax: Bulk truncate val", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_bulk_read(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self, *c;
    struct _binbulk *bulk;
    int fd, size;

    self = SELF(interp);
    if (GET_TAG(self) != BULK) goto error2;
    bulk = self->u.bulk;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    
    c = list_get_item(posargs);
    if (GET_TAG(c) != INTEGER) goto error;
    fd = mpz_get_si(c->u.biginteger);

    if ((size = binbulk_read(bulk, fd)) == -1) {
	if (errno != 0) {
	    return new_exception(TE_FILEACCESS, to_wchar(strerror(errno)), interp);
	}
	return new_exception(TE_FILEACCESS, L"Bad file type.", interp);	
    }
    return new_integer_si(size);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'read', syntax: Bulk read fd", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_bulk_write(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self, *c;
    struct _binbulk *bulk;
    int fd, size, from, to;

    self = SELF(interp);
    if (GET_TAG(self) != BULK) goto error2;
    bulk = self->u.bulk;

    if (arglen != 3) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    
    c = list_get_item(posargs);
    if (GET_TAG(c) != INTEGER) goto error;
    fd = mpz_get_si(c->u.biginteger);

    posargs = list_next(posargs);
    c = list_get_item(posargs);
    if (GET_TAG(c) != INTEGER) goto error;
    from = mpz_get_si(c->u.biginteger);

    posargs = list_next(posargs);
    c = list_get_item(posargs);
    if (GET_TAG(c) != INTEGER) goto error;
    to = mpz_get_si(c->u.biginteger);

    if ((size = binbulk_write(bulk, fd, from, to)) == -1) {
	if (errno != 0) {
	    return new_exception(TE_FILEACCESS, to_wchar(strerror(errno)), interp);
	}
	return new_exception(TE_FILEACCESS, L"Bad file type or bad parameter specified.", interp);	
    }
    return new_integer_si(size);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'write', syntax: Bulk write fd start end", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_bulk_base64encode(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;
    BinBulk *bulk;
    Toy_Type *body;
    Cell *b64str;
    Toy_Type *b64obj, *arg, *sts;

    self = SELF(interp);
    if (GET_TAG(self) != BULK) goto error2;
    bulk = self->u.bulk;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    
    body = list_get_item(posargs);
    if (GET_TAG(body) != CLOSURE) goto error;

    sts = const_Nil;
    binbulk_seek(bulk, 0);
    while (! binbulk_is_eof(bulk)) {
	b64str = binbulk_base64_encode(bulk, 57);
	b64obj = new_string_cell(b64str);
	arg = new_list(b64obj);
	sts = toy_yield(interp, body, arg);
	if (GET_TAG(sts) == EXCEPTION) return  sts;
    }
    
    return sts;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'base64encode', syntax: Bulk base64encode {| b64str | body}", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_bulk_base64decode(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;
    BinBulk *bulk;
    Toy_Type *b64str;
    int sts;

    self = SELF(interp);
    if (GET_TAG(self) != BULK) goto error2;
    bulk = self->u.bulk;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    
    b64str = list_get_item(posargs);
    if (GET_TAG(b64str) != STRING) goto error;

    sts = binbulk_base64_decode(bulk, b64str->u.string);
    if (! sts) {
	return new_exception(TE_SYNTAX, L"Bad base64 string.", interp);
    }
    
    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'base64decode', syntax: Bulk base64decode string", interp);

error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}


int
toy_add_methods(Toy_Interp* interp) {
    toy_add_method(interp, L"Object", L"vars", 		mth_object_vars,	NULL);
    toy_add_method(interp, L"Object", L"method",	mth_object_method,	L"method-name,argspec,body");
    toy_add_method(interp, L"Object", L"var?", 		mth_object_get, 	L"var");
    toy_add_method(interp, L"Object", L"get", 		mth_object_get, 	L"var");
    toy_add_method(interp, L"Object", L"set!",		mth_object_set, 	L"var,val");
    toy_add_method(interp, L"Object", L"type?", 	mth_object_type, 	NULL);
    toy_add_method(interp, L"Object", L"delegate?", 	mth_object_delegate,	NULL);
#if 0 // replace to command eq?
    toy_add_method(interp, L"Object", L"eq", 		mth_object_eq, 		L"val");
#endif
    toy_add_method(interp, L"Object", L"string", 	mth_object_tostring, 	NULL);
    toy_add_method(interp, L"Object", L"literal", 	mth_object_toliteral, 	NULL);
    toy_add_method(interp, L"Object", L"method?",	mth_object_getmethod,	L"name");
    toy_add_method(interp, L"Object", L"apply", 	mth_object_apply, 	L"body");

    toy_add_method(interp, L"Integer", L"+", 		mth_integer_plus, 	L"val");
    toy_add_method(interp, L"Integer", L"-", 		mth_integer_minus, 	L"val");
    toy_add_method(interp, L"Integer", L"*", 		mth_integer_mul, 	L"val");
    toy_add_method(interp, L"Integer", L"/", 		mth_integer_div, 	L"val");
    toy_add_method(interp, L"Integer", L"%", 		mth_integer_mod, 	L"val");
    toy_add_method(interp, L"Integer", L"=", 		mth_integer_eq, 	L"val");
    toy_add_method(interp, L"Integer", L"!=", 		mth_integer_neq, 	L"val");
    toy_add_method(interp, L"Integer", L">", 		mth_integer_gt, 	L"val");
    toy_add_method(interp, L"Integer", L"<", 		mth_integer_lt, 	L"val");
    toy_add_method(interp, L"Integer", L">=", 		mth_integer_ge, 	L"val");
    toy_add_method(interp, L"Integer", L"<=", 		mth_integer_le,	 	L"val");
    toy_add_method(interp, L"Integer", L"++", 		mth_integer_inc, 	L"incr-val");
    toy_add_method(interp, L"Integer", L"--", 		mth_integer_dec,	L"decr-val");
    toy_add_method(interp, L"Integer", L"each", 	mth_integer_each, 	L"to:,val,do:,body");
    toy_add_method(interp, L"Integer", L"..", 		mth_integer_list, 	L"do:,body");
    toy_add_method(interp, L"Integer", L"real", 	mth_integer_toreal,	NULL);
    toy_add_method(interp, L"Integer", L"<<", 		mth_integer_rol,	L"val");
    toy_add_method(interp, L"Integer", L">>", 		mth_integer_ror, 	L"val");
    toy_add_method(interp, L"Integer", L"||", 		mth_integer_or, 	L"val");
    toy_add_method(interp, L"Integer", L"&&", 		mth_integer_and, 	L"val");
    toy_add_method(interp, L"Integer", L"^^", 		mth_integer_xor, 	L"val");
    toy_add_method(interp, L"Integer", L"~~", 		mth_integer_not, 	NULL);
    toy_add_method(interp, L"Integer", L"^", 		mth_integer_power, 	NULL);
    toy_add_method(interp, L"Integer", L"nextprime", 	mth_integer_nextprime,	NULL);
    toy_add_method(interp, L"Integer", L"sqrt", 	mth_integer_sqrt, 	NULL);
    toy_add_method(interp, L"Integer", L"neg", 		mth_integer_neg, 	NULL);
    toy_add_method(interp, L"Integer", L"abs", 		mth_integer_abs, 	NULL);

    toy_add_method(interp, L"Real", L"+", 		mth_real_plus, 		L"val");
    toy_add_method(interp, L"Real", L"-", 		mth_real_minus, 	L"val");
    toy_add_method(interp, L"Real", L"*", 		mth_real_mul, 		L"val");
    toy_add_method(interp, L"Real", L"/", 		mth_real_div, 		L"val");
    toy_add_method(interp, L"Real", L"=", 		mth_real_eq, 		L"val");
    toy_add_method(interp, L"Real", L"!=", 		mth_real_neq, 		L"val");
    toy_add_method(interp, L"Real", L">", 		mth_real_gt, 		L"val");
    toy_add_method(interp, L"Real", L"<", 		mth_real_lt, 		L"val");
    toy_add_method(interp, L"Real", L">=", 		mth_real_ge, 		L"val");
    toy_add_method(interp, L"Real", L"<=", 		mth_real_le, 		L"val");
    toy_add_method(interp, L"Real", L"int", 		mth_real_tointeger, 	NULL);
    toy_add_method(interp, L"Real", L"sqrt", 		mth_real_sqrt, 		NULL);
    toy_add_method(interp, L"Real", L"sin", 		mth_real_sin, 		NULL);
    toy_add_method(interp, L"Real", L"cos", 		mth_real_cos, 		NULL);
    toy_add_method(interp, L"Real", L"tan", 		mth_real_tan, 		NULL);
    toy_add_method(interp, L"Real", L"asin", 		mth_real_asin, 		NULL);
    toy_add_method(interp, L"Real", L"acos", 		mth_real_acos, 		NULL);
    toy_add_method(interp, L"Real", L"atan", 		mth_real_atan, 		NULL);
    toy_add_method(interp, L"Real", L"log", 		mth_real_log, 		NULL);
    toy_add_method(interp, L"Real", L"log10", 		mth_real_log10, 	NULL);
    toy_add_method(interp, L"Real", L"exp", 		mth_real_exp, 		NULL);
    toy_add_method(interp, L"Real", L"exp10", 		mth_real_exp10, 	NULL);
    toy_add_method(interp, L"Real", L"pow", 		mth_real_pow, 		L"val");
    toy_add_method(interp, L"Real", L"neg", 		mth_real_neg, 		NULL);
    toy_add_method(interp, L"Real", L"abs", 		mth_real_abs, 		NULL);
    toy_add_method(interp, L"Real", L"inf?", 		mth_real_isinf, 	NULL);
    toy_add_method(interp, L"Real", L"nan?", 		mth_real_isnan, 	NULL);

    toy_add_method(interp, L"List", L"last", 		mth_list_last, 		NULL);
    toy_add_method(interp, L"List", L"item", 		mth_list_item, 		NULL);
    toy_add_method(interp, L"List", L"car", 		mth_list_item, 		NULL);
    toy_add_method(interp, L"List", L"cdr", 		mth_list_cdr, 		NULL);
    toy_add_method(interp, L"List", L"next", 		mth_list_cdr, 		NULL);
    toy_add_method(interp, L"List", L"append!", 	mth_list_append, 	L"val");
    toy_add_method(interp, L"List", L"add", 		mth_list_add, 		L"val");
    toy_add_method(interp, L"List", L"each", 		mth_list_each, 		L"do:,body");
    toy_add_method(interp, L"List", L"len", 		mth_list_len, 		NULL);
    toy_add_method(interp, L"List", L"null?", 		mth_list_isnull, 	NULL);
    toy_add_method(interp, L"List", L"join", 		mth_list_join, 		L"sep:,val");
    toy_add_method(interp, L"List", L"eval", 		mth_list_eval, 		NULL);
    toy_add_method(interp, L"List", L".", 		mth_list_new_append, 	L"val");
    toy_add_method(interp, L"List", L"get", 		mth_list_get, 		L"val");
    toy_add_method(interp, L"List", L"filter", 		mth_list_filter, 	L"body");
    toy_add_method(interp, L"List", L"map", 		mth_list_map, 		L"body");
    toy_add_method(interp, L"List", L"concat", 		mth_list_concat, 	L"body");
    toy_add_method(interp, L"List", L"seek", 		mth_list_seek, 		L"val");
    toy_add_method(interp, L"List", L"split", 		mth_list_split, 	L"val");
    toy_add_method(interp, L"List", L"<<", 		mth_list_unshift, 	L"val");
    toy_add_method(interp, L"List", L">>", 		mth_list_shift, 	NULL);
    toy_add_method(interp, L"List", L"<<-", 		mth_list_push, 		L"val");
    toy_add_method(interp, L"List", L"->>", 		mth_list_pop, 		NULL);
    toy_add_method(interp, L"List", L"inject", 		mth_list_inject, 	L"val,do:,body");
    toy_add_method(interp, L"List", L"set-car!", 	mth_list_setcar, 	L"val");
    toy_add_method(interp, L"List", L"set-cdr!", 	mth_list_setcdr, 	L"val");
    toy_add_method(interp, L"List", L"create-block", 	mth_list_block, 	NULL);

    toy_add_method(interp, L"String", L"len", 		mth_string_len, 	NULL);
    toy_add_method(interp, L"String", L"=", 		mth_string_equal, 	L"val");
    toy_add_method(interp, L"String", L"!=", 		mth_string_nequal, 	L"val");
    toy_add_method(interp, L"String", L"eval", 		mth_string_eval, 	NULL);
    toy_add_method(interp, L"String", L"append!", 	mth_string_append, 	L"val");
    toy_add_method(interp, L"String", L".", 		mth_string_concat, 	L"val");
    toy_add_method(interp, L"String", L"=~", 		mth_string_match, 	L"reg-exp");
    toy_add_method(interp, L"String", L"sub", 		mth_string_sub, 	L"val,val");
    toy_add_method(interp, L"String", L">", 		mth_string_gt, 		L"val");
    toy_add_method(interp, L"String", L"<", 		mth_string_lt, 		L"val");
    toy_add_method(interp, L"String", L">=", 		mth_string_ge, 		L"val");
    toy_add_method(interp, L"String", L"<=", 		mth_string_le, 		L"val");
    toy_add_method(interp, L"String", L"split", 	mth_string_split, 	L"sep:,val");
    toy_add_method(interp, L"String", L"int", 		mth_string_toint, 	NULL);
    toy_add_method(interp, L"String", L"real", 		mth_string_toreal, 	NULL);
    toy_add_method(interp, L"String", L"number", 	mth_string_tonumber, 	NULL);
    toy_add_method(interp, L"String", L"rquote", 	mth_string_torquote, 	NULL);
    toy_add_method(interp, L"String", L"fmt", 		mth_string_format, 	L"val-list");
    toy_add_method(interp, L"String", L"clean", 	mth_string_clean, 	NULL);
    toy_add_method(interp, L"String", L"upper", 	mth_string_upper, 	NULL);
    toy_add_method(interp, L"String", L"lower", 	mth_string_lower, 	NULL);
    toy_add_method(interp, L"String", L"uexport", 	mth_string_uexport, 	NULL);
    toy_add_method(interp, L"String", L"uimport!", 	mth_string_uimport, 	L"val-list");
    toy_add_method(interp, L"String", L"at", 		mth_string_at, 		L"val");

    toy_add_method(interp, L"File", L"init", 		mth_file_init, 		L"mode:,mode,file-path");
    toy_add_method(interp, L"File", L"open", 		mth_file_open, 		L"mode:,mode,file-path");
    toy_add_method(interp, L"File", L"close", 		mth_file_close, 	NULL);
    toy_add_method(interp, L"File", L"gets", 		mth_file_gets, 		L":nonewline,:nocontrol");
    toy_add_method(interp, L"File", L"puts", 		mth_file_puts, 		L":nonewline,val-list");
    toy_add_method(interp, L"File", L"flush", 		mth_file_flush, 	NULL);
    toy_add_method(interp, L"File", L"stat", 		mth_file_stat, 		NULL);
    toy_add_method(interp, L"File", L"set-newline", 	mth_file_setnewline, 	L"val");
    toy_add_method(interp, L"File", L"eof?", 		mth_file_iseof, 	NULL);
    toy_add_method(interp, L"File", L"fd?", 		mth_file_getfd, 	NULL);
    toy_add_method(interp, L"File", L"set!", 		mth_file_set, 		L"val");
    toy_add_method(interp, L"File", L"ready?", 		mth_file_isready, 	NULL);
    toy_add_method(interp, L"File", L"clear", 		mth_file_clear, 	NULL);
    toy_add_method(interp, L"File", L"set-nobuffer", 	mth_file_setnobuffer, 	NULL);
    toy_add_method(interp, L"File", L"set-noblock", 	mth_file_setnoblock, 	L"val");
    toy_add_method(interp, L"File", L"set-encoding", 	mth_file_setencoding, 	L"val");
    toy_add_method(interp, L"File", L"set-input-encoding", 	mth_file_setinputencoding,	L"val");
    toy_add_method(interp, L"File", L"set-output-encoding", 	mth_file_setoutputencoding,	L"val");

    toy_add_method(interp, L"Block", L"eval", 		mth_block_eval, 	NULL);

    toy_add_method(interp, L"Dict", L"set", 		mth_dict_set, 		L"var,val");
    toy_add_method(interp, L"Dict", L"get", 		mth_dict_get, 		L"var");
    toy_add_method(interp, L"Dict", L"set?", 		mth_dict_isset, 	L"val");
    toy_add_method(interp, L"Dict", L"len", 		mth_dict_len, 		NULL);
    toy_add_method(interp, L"Dict", L"stat",		mth_dict_stat, 		NULL);
    toy_add_method(interp, L"Dict", L"unset", 		mth_dict_unset, 	L"val");
    toy_add_method(interp, L"Dict", L"keys", 		mth_dict_keys, 		NULL);
    toy_add_method(interp, L"Dict", L"pairs", 		mth_dict_pairs, 	NULL);

    toy_add_method(interp, L"Vector", L"append!", 	mth_vector_append, 	L"val");
    toy_add_method(interp, L"Vector", L"set", 		mth_vector_set, 	L"val,val");
    toy_add_method(interp, L"Vector", L"get", 		mth_vector_get, 	L"val,val");
    toy_add_method(interp, L"Vector", L"len", 		mth_vector_len, 	NULL);
    toy_add_method(interp, L"Vector", L"last", 		mth_vector_last,	NULL);
    toy_add_method(interp, L"Vector", L"list", 		mth_vector_list, 	NULL);
    toy_add_method(interp, L"Vector", L"each", 		mth_vector_each, 	L"do:,body");
    toy_add_method(interp, L"Vector", L"swap", 		mth_vector_swap, 	L"val,val");
    toy_add_method(interp, L"Vector", L"resize",	mth_vector_resize, 	L"val");

    toy_add_method(interp, L"Coro", L"next", 		mth_coro_next, 		NULL);
    toy_add_method(interp, L"Coro", L"release", 	mth_coro_release, 	NULL);
    toy_add_method(interp, L"Coro", L"stat", 		mth_coro_stat, 		NULL);
    toy_add_method(interp, L"Coro", L"eval", 		mth_coro_eval, 		L"body");

    toy_add_method(interp, L"Bulk", L"append", 		mth_bulk_append,	L"val");
    toy_add_method(interp, L"Bulk", L"get", 		mth_bulk_get,		NULL);
    toy_add_method(interp, L"Bulk", L"set", 		mth_bulk_set,		L"val");
    toy_add_method(interp, L"Bulk", L"seek", 		mth_bulk_seek,		L"val");
    toy_add_method(interp, L"Bulk", L"position",	mth_bulk_position,	NULL);
    toy_add_method(interp, L"Bulk", L"len", 		mth_bulk_len,		NULL);
    toy_add_method(interp, L"Bulk", L"capacity", 	mth_bulk_capacity,	NULL);
    toy_add_method(interp, L"Bulk", L"truncate", 	mth_bulk_truncate,	L"val");
    toy_add_method(interp, L"Bulk", L"read", 		mth_bulk_read,		L"val");
    toy_add_method(interp, L"Bulk", L"write", 		mth_bulk_write,		L"val");
    toy_add_method(interp, L"Bulk", L"base64encode",	mth_bulk_base64encode,	L"body");
    toy_add_method(interp, L"Bulk", L"base64decode",	mth_bulk_base64decode,	L"body");

    return 0;
}
