/* $Id: methods.c,v 1.68 2011/12/09 12:54:40 mit-sato Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <setjmp.h>
#include <ctype.h>
#include <sys/select.h>
#include <oniguruma.h>
#include <math.h>
#include "toy.h"
#include "interp.h"
#include "types.h"
#include "cell.h"
#include "global.h"
#include "array.h"
#include "cstack.h"

Toy_Type* cmd_fun(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen);

Toy_Type*
mth_object_vars(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *selfh;
    
    if (arglen > 0) goto error;
    if (hash_get_length(nameargs)) goto error;

    selfh = SELF_HASH(interp);

    return hash_get_keys(selfh);

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'vars', syntax: Object vars", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'method', syntax: Object method name (argspec) {block}\n\t\
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
	c = new_cell("No such variable in slot, '");
	cell_add_str(c, cell_get_addr(var->u.symbol.cell));
	cell_add_str(c, "'.");
	return new_exception(TE_NOVAR, cell_get_addr(c), interp);
    }
    return o;

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'var?', syntax: Object var? | get name", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'set', syntax: Object set name value", interp);
}

Toy_Type*
mth_object_type(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 0) goto error;

    return new_symbol(toy_get_type_str(SELF(interp)));
    
error:
    return new_exception(TE_SYNTAX, "Syntax error at 'type?', syntax: Object type?", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'delegate?', syntax: Object delegate?", interp);
}

#if 0
Toy_Type*
mth_object_eq(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    if (SELF_OBJ(interp) == list_get_item(posargs)) return SELF_OBJ(interp);
    return const_Nil;
    
error:
    return new_exception(TE_SYNTAX, "Syntax error at 'eq', syntax: Object eq object", interp);
}
#endif

Toy_Type*
mth_object_tostring(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 0) goto error;

    return new_string_str(to_string(SELF(interp)));
    
error:
    return new_exception(TE_SYNTAX, "Syntax error at 'string', syntax: Object string", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'method?', syntax: Object method? symbol", interp);
}

Toy_Type*
mth_object_apply(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *block;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen < 1) goto error;

    block = list_get_item(posargs);
    if (GET_TAG(block) != CLOSURE) goto error;

    posargs = list_next(posargs);
    
    return toy_yield(interp, block, posargs);

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'apply', syntax: Object apply {| var ... | block} val ...", interp);
}

Toy_Type*
mth_integer_plus(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;
    mpz_t s;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);
    switch (GET_TAG(arg)) {
    case INTEGER:
	mpz_init(s);
	mpz_set(s, SELF(interp)->u.biginteger);
	mpz_add(s, s, arg->u.biginteger);
	return new_integer(s);
	
    case REAL:
	return new_real(mpz_get_d(SELF(interp)->u.biginteger)
			+ arg->u.real);
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '+', syntax: Integer + number-val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}


Toy_Type*
mth_integer_minus(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;
    mpz_t s;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);
    switch (GET_TAG(arg)) {
    case INTEGER:
	mpz_init(s);
	mpz_set(s, SELF(interp)->u.biginteger);
	mpz_sub(s, s, arg->u.biginteger);
	return new_integer(s);
	
    case REAL:
	return new_real(mpz_get_d(SELF(interp)->u.biginteger)
			- arg->u.real);
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '-', syntax: Integer - number-val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_integer_mul(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;
    mpz_t s;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);
    switch (GET_TAG(arg)) {
    case INTEGER:
	mpz_init(s);
	mpz_set(s, SELF(interp)->u.biginteger);
	mpz_mul(s, s, arg->u.biginteger);
	return new_integer(s);
	
    case REAL:
	return new_real(mpz_get_d(SELF(interp)->u.biginteger)
			* arg->u.real);
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '*', syntax: Integer * number-val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_integer_div(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;
    mpz_t s;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);
    switch (GET_TAG(arg)) {
    case INTEGER:
	if (0 == mpz_cmp_si(arg->u.biginteger, 0)) {
	    return new_exception(TE_ZERODIV, "Zero divide.", interp);
	}
	mpz_init(s);
	mpz_set(s, SELF(interp)->u.biginteger);
	mpz_div(s, s, arg->u.biginteger);
	return new_integer(s);
	
    case REAL:
	if (arg->u.real == 0.0) {
	    return new_exception(TE_ZERODIV, "Zero divide.", interp);
	}
	return new_real(mpz_get_d((SELF(interp)->u.biginteger))
			/ arg->u.real);
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '/', syntax: Integer / number-val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_integer_mod(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;
    mpz_t s;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);
    switch (GET_TAG(arg)) {
    case INTEGER:
	mpz_init(s);
	mpz_set(s, SELF(interp)->u.biginteger);
	mpz_mod(s, s, arg->u.biginteger);
	return new_integer(s);
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '%', syntax: Integer % integer-val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}


Toy_Type*
mth_integer_eq(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);
    switch (GET_TAG(arg)) {
    case INTEGER:
	if (0 == (mpz_cmp(SELF(interp)->u.biginteger,
			  arg->u.biginteger))) {
	    return SELF(interp);
	}
	return const_Nil;
	
    case REAL:
	if (mpz_get_d(SELF(interp)->u.biginteger) == arg->u.real) {
	    return SELF(interp);
	}
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '=', syntax: Integer = number-val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_integer_neq(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);
    switch (GET_TAG(arg)) {
    case INTEGER:
	if (0 != (mpz_cmp(SELF(interp)->u.biginteger,
			  arg->u.biginteger))) {
	    return SELF(interp);
	}
	return const_Nil;
	
    case REAL:
	if (mpz_get_d(SELF(interp)->u.biginteger) != arg->u.real) {
	    return SELF(interp);
	}
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '!=', syntax: Integer != number-val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_integer_gt(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);
    switch (GET_TAG(arg)) {
    case INTEGER:
	if (mpz_cmp(SELF(interp)->u.biginteger, arg->u.biginteger) > 0) {
	    return SELF(interp);
	}
	return const_Nil;
	
    case REAL:
	if (mpz_get_d(SELF(interp)->u.biginteger) > arg->u.real) {
	    return SELF(interp);
	}
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '>', syntax: Integer > number-val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_integer_lt(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);
    switch (GET_TAG(arg)) {
    case INTEGER:
	if (mpz_cmp(SELF(interp)->u.biginteger, arg->u.biginteger) < 0) {
	    return SELF(interp);
	}
	return const_Nil;
	
    case REAL:
	if (mpz_get_d(SELF(interp)->u.biginteger) < arg->u.real) {
	    return SELF(interp);
	}
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '<', syntax: Integer < number-val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_integer_ge(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);
    switch (GET_TAG(arg)) {
    case INTEGER:
	if (mpz_cmp(SELF(interp)->u.biginteger, arg->u.biginteger) >=0) {
	    return SELF(interp);
	}
	return const_Nil;
	
    case REAL:
	if (mpz_get_d(SELF(interp)->u.biginteger) >= arg->u.real) {
	    return SELF(interp);
	}
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '>=', syntax: Integer >= number-val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_integer_le(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    arg = list_get_item(posargs);
    switch (GET_TAG(arg)) {
    case INTEGER:
	if (mpz_cmp(SELF(interp)->u.biginteger, arg->u.biginteger) <= 0) {
	    return SELF(interp);
	}
	return const_Nil;
	
    case REAL:
	if (mpz_get_d(SELF(interp)->u.biginteger) <= arg->u.real) {
	    return SELF(interp);
	}
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '<=', syntax: Integer <= number-val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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

    if (GET_TAG(arg) == INTEGER) {
	mpz_add(SELF(interp)->u.biginteger,
		SELF(interp)->u.biginteger,
		arg->u.biginteger);
	return SELF(interp);
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '++', syntax: Integer ++ [number-val]", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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

    if (GET_TAG(arg) == INTEGER) {
	mpz_sub(SELF(interp)->u.biginteger,
		SELF(interp)->u.biginteger,
		arg->u.biginteger);
	return SELF(interp);
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '--', syntax: Integer -- [number-val]", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
	    goto loop_continue;
	    break;

	case CTRL_REDO:
	    goto loop;
	    break;

	case CTRL_RETRY:
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
    return new_exception(TE_SYNTAX, "Syntax error at 'each', syntax: Integer each to: number do: {| var | block}",
			 interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
	    goto loop_continue;
	    break;

	case CTRL_REDO:
	    goto loop;
	    break;

	case CTRL_RETRY:
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
    return new_exception(TE_SYNTAX, "Syntax error at '..', syntax: Integer .. last [do: {| var | block}]",
			 interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_integer_toreal(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != INTEGER) goto error2;

    return new_real(mpz_get_d(SELF(interp)->u.biginteger));
    
error:
    return new_exception(TE_SYNTAX, "Syntax error at 'real', syntax: Integer real", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at '<<', syntax: Integer << bit", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at '>>', syntax: Integer >> bit", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at '||', syntax: Integer || val", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at '&&', syntax: Integer && val", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at '^^', syntax: Integer ^^ val", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at '~~', syntax: Integer ~~ val", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at '^', syntax: Integer ^ number-val(>=0)", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_real_plus(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    arg = list_get_item(posargs);
    switch (GET_TAG(arg)) {
    case REAL:
	return new_real(SELF(interp)->u.real + arg->u.real);

    case INTEGER:
	return new_real(SELF(interp)->u.real
			+ mpz_get_d(arg->u.biginteger));
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '+', syntax: Real + number-val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}


Toy_Type*
mth_real_minus(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    arg = list_get_item(posargs);
    switch (GET_TAG(arg)) {
    case REAL:
	return new_real(SELF(interp)->u.real - arg->u.real);

    case INTEGER:
	return new_real(SELF(interp)->u.real
			- mpz_get_d(arg->u.biginteger));
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '-', syntax: Real - number-val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_real_mul(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    arg = list_get_item(posargs);
    switch (GET_TAG(arg)) {
    case REAL:
	return new_real(SELF(interp)->u.real * arg->u.real);

    case INTEGER:
	return new_real(SELF(interp)->u.real
			* mpz_get_d(arg->u.biginteger));
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '*', syntax: Real * number-val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_real_div(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    arg = list_get_item(posargs);
    switch (GET_TAG(arg)) {
    case REAL:
	if (arg->u.real == 0.0) {
	    return new_exception(TE_ZERODIV, "Zero divide.", interp);
	}
	return new_real(SELF(interp)->u.real / arg->u.real);

    case INTEGER:
	if (0 == (mpz_cmp_si(arg->u.biginteger, 0))) {
	    return new_exception(TE_ZERODIV, "Zero divide.", interp);
	}
	return new_real(SELF(interp)->u.real
			/ mpz_get_d(arg->u.biginteger));
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '/', syntax: Real / number-val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_real_eq(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    arg = list_get_item(posargs);
    switch (GET_TAG(arg)) {
    case REAL:
	if (SELF(interp)->u.real == arg->u.real) {
	    return SELF(interp);
	}
	return const_Nil;

    case INTEGER:
	if (SELF(interp)->u.real == mpz_get_d(arg->u.biginteger)) {
	    return SELF(interp);
	}
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '=', syntax: Real = number-val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_real_neq(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    arg = list_get_item(posargs);
    switch (GET_TAG(arg)) {
    case REAL:
	if (SELF(interp)->u.real != arg->u.real) {
	    return SELF(interp);
	}
	return const_Nil;

    case INTEGER:
	if (SELF(interp)->u.real != mpz_get_d(arg->u.biginteger)) {
	    return SELF(interp);
	}
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '!=', syntax: Real != number-val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_real_gt(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    arg = list_get_item(posargs);
    switch (GET_TAG(arg)) {
    case REAL:
	if (SELF(interp)->u.real > arg->u.real) {
	    return SELF(interp);
	}
	return const_Nil;

    case INTEGER:
	if (SELF(interp)->u.real > mpz_get_d(arg->u.biginteger)) {
	    return SELF(interp);
	}
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '>', syntax: Real > number-val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_real_lt(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    arg = list_get_item(posargs);
    switch (GET_TAG(arg)) {
    case REAL:
	if (SELF(interp)->u.real < arg->u.real) {
	    return SELF(interp);
	}
	return const_Nil;

    case INTEGER:
	if (SELF(interp)->u.real < mpz_get_d(arg->u.biginteger)) {
	    return SELF(interp);
	}
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '<', syntax: Real < number-val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_real_ge(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    arg = list_get_item(posargs);
    switch (GET_TAG(arg)) {
    case REAL:
	if (SELF(interp)->u.real >= arg->u.real) {
	    return SELF(interp);
	}
	return const_Nil;

    case INTEGER:
	if (SELF(interp)->u.real >= mpz_get_d(arg->u.biginteger)) {
	    return SELF(interp);
	}
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '>=', syntax: Real >= number-val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_real_le(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg;

    if (arglen > 1) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    arg = list_get_item(posargs);
    switch (GET_TAG(arg)) {
    case REAL:
	if (SELF(interp)->u.real <= arg->u.real) {
	    return SELF(interp);
	}
	return const_Nil;

    case INTEGER:
	if (SELF(interp)->u.real <= mpz_get_d(arg->u.biginteger)) {
	    return SELF(interp);
	}
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '<=', syntax: Real <= number-val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_real_tointeger(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_integer_d(SELF(interp)->u.real);
    
error:
    return new_exception(TE_SYNTAX, "Syntax error at 'int', syntax: Real int", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_real_sqrt(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(sqrt(SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, "Syntax error at 'sqrt', syntax: Real sqrt", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_real_sin(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(sin(SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, "Syntax error at 'sin', syntax: Real sin", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_real_cos(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(cos(SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, "Syntax error at 'cos', syntax: Real cos", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_real_tan(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(tan(SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, "Syntax error at 'tan', syntax: Real tan", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_real_asin(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(asin(SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, "Syntax error at 'asin', syntax: Real asin", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_real_acos(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(acos(SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, "Syntax error at 'acos', syntax: Real acos", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_real_atan(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(atan(SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, "Syntax error at 'atan', syntax: Real atan", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_real_log(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(log(SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, "Syntax error at 'log', syntax: Real log", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_real_log10(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(log10(SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, "Syntax error at 'log10', syntax: Real log10", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_real_exp(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(exp(SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, "Syntax error at 'exp', syntax: Real exp", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_real_exp10(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != REAL) goto error2;

    return new_real(pow(10.0, SELF(interp)->u.real));
    
error:
    return new_exception(TE_SYNTAX, "Syntax error at 'exp10', syntax: Real exp10", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'pow', syntax: Real pow real-value", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'last', syntax: List last", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_list_item(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *l;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != LIST) goto error2;

    l = SELF(interp);
    if (GET_TAG(l) != LIST) goto error;

    if (NULL == list_get_item(l)) return new_list(NULL);
    return list_get_item(l);

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'car', syntax: List car | item", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'cdr', syntax: List cdr | next", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'append', syntax: List append! | + [val ...]", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'add', syntax: List add [val ...]", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
	    goto loop_continue;
	    break;
	case CTRL_REDO:
	    goto loop;
	    break;
	case CTRL_RETRY:
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
    return new_exception(TE_SYNTAX, "Syntax error at 'each', syntax: List each do: {| var | block}", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'len', syntax: List len", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'null?', syntax: List null?", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_list_join(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *sep, *self;
    Cell *result;
    char *psep;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 1) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != LIST) goto error2;
    sep = hash_get_and_unset_t(nameargs, const_sep);
    if (sep == NULL) {
	sep = new_string_str("");
    } else {
	sep = new_string_str(to_string(sep));
    }
    psep = cell_get_addr(sep->u.string);

    result = new_cell("");

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
    return new_exception(TE_SYNTAX, "Syntax error at 'join', syntax: List join [sep: separator]", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_list_eval(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Env *env = NULL;
    Toy_Type *result;

    if (arglen > 0) goto error;

    result = toy_eval(interp, new_statement(SELF(interp), interp->trace_info->line), &env);
    return result;

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'eval', syntax: List eval", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at '.', syntax: List . [val ...]", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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

    return (GET_TAG(src)==LIST) ? list_get_item(src) : src;

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'get', syntax: List get index", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_list_filter(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *block, *self, *result, *l, *ret;
    Toy_Env *env;

    if (arglen != 1) goto error;
    if (hash_get_length(nameargs) != 0) goto error;

    block = list_get_item(posargs);
    if (GET_TAG(block) != CLOSURE) goto error;

    self = SELF(interp);
    if (GET_TAG(self) != LIST) goto error2;

    l = result = new_list(NULL);
    env = block->u.closure.env;

    while (! IS_LIST_NULL(self)) {
	if (GET_TAG(self) == LIST) {
	    ret = toy_yield(interp, block, new_list(list_get_item(self)));
	} else {
	    ret = toy_yield(interp, block, new_list(self));
	}
	if (GET_TAG(ret) == EXCEPTION) return ret;
	if (GET_TAG(ret) != NIL) {
	    l = list_append(l, list_get_item(self));
	}
	self = list_next(self);
	if (GET_TAG(self) != LIST) break;
    }

    return result;

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'filter', syntax: List filter {| var | block}", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'map', syntax: List map {| var | block}", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'concat', syntax: List concat (list) | var ...", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'seek', syntax: List seek index", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'split', syntax: List split index", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at '<<', syntax: List << val", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at '>>', syntax: List >>", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at '<<-', syntax: List <<- val", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at '->>', syntax: List ->>", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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

loop:
    if (! l) goto fin;
    if (IS_LIST_NULL(l)) goto fin;
    
    if (GET_TAG(l) == LIST) {
	item = list_get_item(l);
    } else {
	item = l;
    }

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
	    goto loop_continue;
	    break;
	case CTRL_REDO:
	    goto loop;
	    break;
	case CTRL_RETRY:
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
	"Syntax error at 'inject', syntax: List inject init-val do: {| sum-var each-var | block}", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'len', syntax: String len", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at '+', syntax: String + val ...", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_string_equal(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != STRING) goto error2;

    if (strcmp(cell_get_addr(self->u.string),
	       to_string_call(interp, list_get_item(posargs))) == 0) {
	return const_T;
    } else {
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '=', syntax: String = object", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_string_gt(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != STRING) goto error2;

    if (strcmp(cell_get_addr(self->u.string),
	       to_string_call(interp, list_get_item(posargs))) > 0) {
	return const_T;
    } else {
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '>', syntax: String > object", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_string_lt(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != STRING) goto error2;

    if (strcmp(cell_get_addr(self->u.string),
	       to_string_call(interp, list_get_item(posargs))) < 0) {
	return const_T;
    } else {
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '<', syntax: String < object", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_string_ge(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != STRING) goto error2;

    if (strcmp(cell_get_addr(self->u.string),
	       to_string_call(interp, list_get_item(posargs))) >= 0) {
	return const_T;
    } else {
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '>=', syntax: String >= object", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_string_le(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != STRING) goto error2;

    if (strcmp(cell_get_addr(self->u.string),
	       to_string_call(interp, list_get_item(posargs))) <= 0) {
	return const_T;
    } else {
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '<=', syntax: String <= object", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_string_nequal(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;
    self = SELF(interp);
    if (GET_TAG(self) != STRING) goto error2;

    if (strcmp(cell_get_addr(self->u.string),
	       to_string_call(interp, list_get_item(posargs))) != 0) {
	return const_T;
    } else {
	return const_Nil;
    }

error:
    return new_exception(TE_SYNTAX, "Syntax error at '!=', syntax: String != object", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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

    result = toy_eval_script(interp, script);

    return result;

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'eval', syntax: String eval", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at '+', syntax: String + [val ...]", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at '.', syntax: String . [val ...]", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_string_match(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *self, *tpattern;
    int r;
    unsigned char *start, *range, *end;
    regex_t *reg;
    OnigErrorInfo einfo;
    OnigRegion *region;
    unsigned char *pattern;
    unsigned char *str;
    Toy_Type *result, *resultl;
    int offs;
    int option, soption;
    Toy_Type *t_case = NULL;
    Toy_Type *t_all = NULL;

    if (arglen > 1) goto error;

    self = SELF(interp);
    if (GET_TAG(self) != STRING) goto error2;

    tpattern = list_get_item(posargs);
    if (GET_TAG(tpattern) != RQUOTE) goto error;
    if (cell_get_length(tpattern->u.rquote) <= 0) {
	return new_exception(TE_REGEX, "Null regex pattern.", interp);
    }

    t_case = hash_get_and_unset_t(nameargs, const_nocase);
    if (GET_TAG(t_case) == NIL) t_case = NULL;
    t_all = hash_get_and_unset_t(nameargs, const_all);
    if (GET_TAG(t_all) == NIL) t_all = NULL;

    if (hash_get_length(nameargs) > 0) goto error;

    str = (unsigned char*)cell_get_addr(self->u.string);
    pattern = (unsigned char*)cell_get_addr(tpattern->u.rquote);

    option = ONIG_OPTION_NONE;
    if (t_case) {
	option |= ONIG_OPTION_IGNORECASE;
    }
    r = onig_new(&reg, pattern, pattern + cell_get_length(tpattern->u.rquote),
		 option, ONIG_ENCODING_EUC_JP, ONIG_SYNTAX_DEFAULT, &einfo);
    /*
      ONIG_OPTION_NONE, ONIG_ENCODING_EUC_JP, ONIG_SYNTAX_DEFAULT, &einfo);
      ONIG_OPTION_NONE, ONIG_ENCODING_EUC_JP, ONIG_SYNTAX_GREP, &einfo);
      ONIG_OPTION_CAPTURE_GROUP, ONIG_ENCODING_EUC_JP, ONIG_SYNTAX_DEFAULT, &einfo);
    */
    if (r != ONIG_NORMAL) {
	char s[ONIG_MAX_ERROR_MESSAGE_LEN];
	onig_error_code_to_str(s, r, &einfo);
	return new_exception(TE_REGEX, s, interp);
    }

    region = onig_region_new();

    end = str + cell_get_length(self->u.string);
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
	Cell *s;

	max = -1;
	for (i=0; i<region->num_regs; i++) {
	    if (! ((region->beg[i] >= 0) && (region->end[i] >= 0))) continue;

	    ll = l = new_list(NULL);
	    s = new_cell("");
	    l = list_append(l, new_integer_si(region->beg[i] + offs));
	    l = list_append(l, new_integer_si(region->end[i] + offs));
	    l = list_append(l, new_string_cell(cell_sub(self->u.string,
							region->beg[i] + offs,
							region->end[i] + offs)));
	    //l = list_append(l, new_integer(region->beg[i]));
	    //l = list_append(l, new_integer(region->end[i]));
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
	char s[ONIG_MAX_ERROR_MESSAGE_LEN];
	onig_error_code_to_str(s, r, &einfo);
	result = new_exception(TE_REGEX, s, interp);
    }

    onig_region_free(region, 1);
    onig_free(reg);
    onig_end();

    return result;

error:
    return new_exception(TE_SYNTAX,
	 "Syntax error at '=~', syntax: String =~ [:nocase] [:all] 'pattern'", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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

    if (arglen == 2) {
	posargs = list_next(posargs);
	t = list_get_item(posargs);
	if (GET_TAG(t) != INTEGER) goto error;
	end = mpz_get_si(t->u.biginteger);
    }

    self = SELF(interp);
    if (GET_TAG(self) != STRING) goto error2;

    o = self->u.string;

    return new_string_cell(cell_sub(o, start, end));

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'sub', syntax: String sub start [end]", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

static int
is_sub_eq(char *src, char *dest);

Toy_Type*
mth_string_split(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *sep = NULL;
    Toy_Type *self, *result, *l;
    char *p, *csep;
    Cell *word;
    int seplen;

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
	    word = new_cell("");

	    /* skip white space */
	    while (*p && isspace(*p)) {
		p++;
	    }
	    if (! *p) break;

	    /* collect word, until white space */
	    while (*p) {
		if (! isspace(*p)) {
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
	seplen = strlen(csep);
	
	while (*p) {
	    word = new_cell("");

	    /* collect word, until separator */
	    while (*p) {
		if (! is_sub_eq(p, csep)) {
		    cell_add_char(word, *p);
		} else {
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
    }

    return result;

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'split', syntax: String split [sep: separator]", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

static int
is_sub_eq(char *src, char *dest) {
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
    if (GET_TAG(result) != INTEGER) return const_Nil;
    return result;

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'int', syntax: String int", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    if (GET_TAG(result) != REAL) return const_Nil;
    return result;

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'real', syntax: String real", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'number', syntax: String number", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_string_torquote(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (arglen != 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != STRING) goto error2;

    return new_rquote(cell_get_addr(SELF(interp)->u.string));

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'rquote', syntax: String rquote", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}


static char*
mth_string_format_fill(char* item, int fill, int trim) {
    Cell *result, *result2;
    int i, slen;

    slen = strlen(item);

    result = new_cell("");
    if ((fill == 0) || (slen >= abs(fill))) {
	cell_add_str(result, item);
    } else if (fill < 0) {
	/* lef align */
	cell_add_str(result, item);
	for (i=0 ; i<(-fill-slen) ; i++) {
	    cell_add_char(result, ' ');
	}
    } else if (fill > 0) {
	/* right align */
	for (i=0 ; i<(fill-slen) ; i++) {
	    cell_add_char(result, ' ');
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

static char*
mth_string_format_C(Toy_Type *item, Cell *fmt) {
    char *buff;

    buff = GC_MALLOC(64);
    ALLOC_SAFE(buff);

    switch (GET_TAG(item)) {
    case INTEGER:
	/* XXX: fix it for bit integer */
	snprintf(buff, 64, cell_get_addr(fmt),
		 mpz_get_si(item->u.biginteger));
	break;
    case REAL:
	snprintf(buff, 64, cell_get_addr(fmt),
		 item->u.real);
	break;
    default:
	return 0;
    }

    return buff;
}

Toy_Type*
mth_string_format(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    char *p, *f;
    int acc, neg, trim;
    int done;
    Toy_Type *result, *item;
    Cell *c, *sacc;

    if (hash_get_length(nameargs) > 0) goto error;
    if (GET_TAG(SELF(interp)) != STRING) goto error2;

    p = cell_get_addr(SELF(interp)->u.string);

    result = new_string_str("");
    c = result->u.string;
    while (*p) {
	switch (*p) {
	case '%':
	    acc = 0;
	    neg = 1;
	    trim = 0;
	    done = 0;
	    sacc = new_cell("%");
	    p++;
	    if (! *p) {
		return result;
	    }
	    while (*p) {
		switch (*p) {
		case '%':
		    cell_add_char(c, '%');
		    done = 1;
		    break;

		case '-':
		    neg = -1;
		    cell_add_char(sacc, *p);
		    break;

		case '!':
		    trim = 1;
		    break;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		    acc = acc*10 + (*p-'0');
		    cell_add_char(sacc, *p);
		    break;

		case '.':
		    cell_add_char(sacc, *p);
		    acc = 0;
		    break;

		case 'v':
		    item = list_get_item(posargs);
		    if (NULL == item) {
			item = const_Nil;
		    } else {
			posargs = list_next(posargs);
		    }
		    cell_add_str(c, mth_string_format_fill(to_string_call(interp, item), acc * neg, trim));
		    done = 1;
		    break;

		case 'd':
		case 'o': case 'u': case 'x': case 'X':
		    cell_add_char(sacc, 'l');
		    cell_add_char(sacc, 'l');
		    /* fall thru */

		case 'f': case 'F':
		case 'e': case 'E':
		case 'g': case 'G':
		    cell_add_char(sacc, *p);

		    item = list_get_item(posargs);
		    if (NULL == item) {
			item = const_Nil;
		    } else {
			posargs = list_next(posargs);
		    }
		    f = mth_string_format_C(item, sacc);
		    if (f) {
			cell_add_str(c, f);
		    } else {
			return new_exception(TE_TYPE, "Format indicator type error.", interp);
		    }
		    done = 1;
		    break;

		default:
		    return new_exception(TE_SYNTAX, "Bad format indicator.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'fmt', syntax: String fmt var ...", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_block_eval(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *closure, *result;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 2) goto error;
    if (GET_TAG(SELF(interp)) != CLOSURE) goto error2;

    closure = SELF(interp);
    result = eval_closure(interp, closure);
    return result;

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'eval', syntax: Block eval", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

#define FMODE_INPUT	(1)
#define FMODE_OUTPUT	(2)
#define FMODE_APPEND	(3)
#define FMODE_INOUT	(4)

typedef struct _toy_file {
    FILE *fd;
    int mode;
    Toy_Type *path;
} Toy_File;

static Toy_File*
new_file() {
    Toy_File *o;

    o = GC_MALLOC(sizeof(Toy_File));
    ALLOC_SAFE(o);
    memset(o, 0, sizeof(Toy_File));

    return o;
}

Toy_Type* mth_file_open(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen);

Toy_Type*
mth_file_init(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;

    self = SELF_HASH(interp);
    hash_set_t(self, const_Holder, new_container(new_file()));

    if (arglen > 0) {
	Toy_Type *cmd, *l;

	l = cmd = new_list(new_symbol("open"));
	posargs = list_get_item(posargs);
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
    char *pmode;

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
	if (strcmp(pmode, "i") == 0) {
	    f->mode = FMODE_INPUT;
	} else if (strcmp(pmode, "o") == 0) {
	    f->mode = FMODE_OUTPUT;
	} else if (strcmp(pmode, "a") == 0) {
	    f->mode = FMODE_APPEND;
	} else if (strcmp(pmode, "io") == 0) {
	    f->mode = FMODE_INOUT;
	} else goto error;
    }

    path = list_get_item(posargs);
    if (GET_TAG(path) != STRING) goto error;

    f->path = path;

    if (f->fd) {
	fclose(f->fd);
    }

    f->fd = fopen(cell_get_addr(f->path->u.string),
		  (f->mode==FMODE_INPUT)  ? "r"  :
		  (f->mode==FMODE_OUTPUT) ? "w"  :
		  (f->mode==FMODE_INOUT)  ? "r+" :
		   "a");

    if (NULL == f->fd) {
	return new_exception(TE_FILENOTOPEN, strerror(errno), interp);
    }

    return const_T;

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'open', syntax: File oepn [mode: i | o | a | io] \"path\"", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'close', syntax: File close", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_file_gets(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    Toy_File *f;
    Toy_Type *container;
    Cell *cbuff;
    int c;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    f = container->u.container;

    if (NULL == f->fd) {
	return new_exception(TE_FILEACCESS, "File not open.", interp);
    }
    if ((FMODE_INPUT != f->mode) && (FMODE_INOUT != f->mode)) {
	return new_exception(TE_FILEACCESS, "Bad file access mode.", interp);
    }

    if (feof(f->fd)) return const_Nil;

    cbuff = new_cell("");
    while (1) {
	c = fgetc(f->fd);
	if (EOF == c) {
	    if (cell_get_length(cbuff) == 0) {
		return new_string_str("");
	    } else {
		return new_string_cell(cbuff);
	    }
	}
	cell_add_char(cbuff, c);
	if ('\n' == c) {
	    return new_string_cell(cbuff);
	}
    }


error:
    return new_exception(TE_SYNTAX, "Syntax error at 'gets', syntax: File gets", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_file_puts(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    Toy_File *f;
    Toy_Type *container;
    int c;

    if (arglen == 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    f = container->u.container;

    if (NULL == f->fd) {
	return new_exception(TE_FILEACCESS, "File not open.", interp);
    }
    if ((FMODE_OUTPUT != f->mode) && (FMODE_APPEND != f->mode) && (FMODE_INOUT != f->mode)) {
	return new_exception(TE_FILEACCESS, "Bad file access mode.", interp);
    }

    while (posargs) {
	char *p;

	p = to_string_call(interp, list_get_item(posargs));
	c = fputs(p, f->fd);
	if (EOF == c) {
	    return new_exception(TE_FILEACCESS, strerror(errno), interp);
	}

	posargs = list_next(posargs);
    }

    return const_T;

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'puts', syntax: File puts val ...", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
	return new_exception(TE_FILEACCESS, "File not open.", interp);
    }
    if ((FMODE_OUTPUT != f->mode) && (FMODE_APPEND != f->mode) && (FMODE_INOUT != f->mode)) {
	return new_exception(TE_FILEACCESS, "Bad file access mode.", interp);
    }

    fflush(f->fd);

    return const_T;

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'flush', syntax: File flush", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_file_stat(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    Toy_File *f;
    Toy_Type *container;
    Toy_Type *l, *mode;

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    f = container->u.container;

    l = new_list(NULL);

    list_append(l, new_cons(new_symbol("fd"),
			    f->fd?new_integer_si(fileno(f->fd)):const_Nil));
    switch (f->mode) {
    case FMODE_INPUT:
	mode = new_symbol("i");
	break;
    case FMODE_OUTPUT:
	mode = new_symbol("o");
	break;
    case FMODE_APPEND:
	mode = new_symbol("a");
	break;
    case FMODE_INOUT:
	mode = new_symbol("io");
	break;
    default:
	mode = const_Nil;
    }
    list_append(l, new_cons(new_symbol("mode"), mode));
    list_append(l, new_cons(new_symbol("path"), f->path?f->path:const_Nil));
    if (f->fd) {
	if (feof(f->fd)) {
	    list_append(l, new_cons(new_symbol("eof"), const_T));
	} else {
	    list_append(l, new_cons(new_symbol("eof"), const_Nil));
	}
    } else {
	list_append(l, new_cons(new_symbol("eof"), const_Nil));
    }

    return l;

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'stat', syntax: File stat", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
	return new_exception(TE_FILEACCESS, "File not open.", interp);
    }

    if (feof(f->fd)) {
	return const_T;
    } else {
	return const_Nil;
    }


error:
    return new_exception(TE_SYNTAX, "Syntax error at 'eof?', syntax: File eof?", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    } else if (strcmp(cell_get_addr(mode->u.symbol.cell), "i") == 0) {
	imode = "r";
	f->mode = FMODE_INPUT;
    } else if (strcmp(cell_get_addr(mode->u.symbol.cell), "o") == 0) {
	imode = "w";
	f->mode = FMODE_OUTPUT;
    } else if (strcmp(cell_get_addr(mode->u.symbol.cell), "a") == 0) {
	imode = "a";
	f->mode = FMODE_APPEND;
    } else if (strcmp(cell_get_addr(mode->u.symbol.cell), "io") == 0) {
	imode = "r+";
	f->mode = FMODE_INOUT;
    } else goto error;
	
    f->fd = fdopen(mpz_get_si(tfd->u.biginteger), imode);
    f->path = const_Nil;

    if (NULL == f->fd) {
	return new_exception(TE_FILENOTOPEN, strerror(errno), interp);
    }

    return const_T;

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'set!', syntax: File set! [mode: iomode] integer-fd", interp);
error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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

    if (arglen > 0) goto error;
    if (hash_get_length(nameargs) > 0) goto error;

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    f = container->u.container;

    fd = f->fd;
    if (NULL == fd) goto error2;

    fdesc = fileno(fd);
    if (fdesc < 0) goto error2;

    FD_ZERO(&fds);
    FD_SET(fdesc, &fds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

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
    return new_exception(TE_SYNTAX, "Syntax error at 'ready?', syntax: File ready?", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_dict_set(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *o, *val;
    char *key;

    if (arglen != 2) goto error;

    o = SELF(interp);
    if (GET_TAG(o) != DICT) goto error2;

    key = to_string_call(interp, list_get_item(posargs));
    val = list_get_item(list_next(posargs));

    hash_set(o->u.dict, key, val);

    return val;

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'set', syntax: Dict set key value", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_dict_get(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *o, *result;
    char *key;

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
    return new_exception(TE_SYNTAX, "Syntax error at 'get', syntax: Dict get key", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_dict_isset(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *o, *result;
    char *key;

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
    return new_exception(TE_SYNTAX, "Syntax error at 'set?', syntax: Dict set? key", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_dict_len(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *o;

    if (arglen != 0) goto error;

    o = SELF(interp);
    if (GET_TAG(o) != DICT) goto error2;

    return new_integer_si(hash_get_length(o->u.dict));

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'len', syntax: Dict len", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_dict_unset(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *o, *result;
    char *key;

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
    return new_exception(TE_SYNTAX, "Syntax error at 'unset', syntax: Dict unset key", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_dict_keys(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *o;

    if (arglen != 0) goto error;

    o = SELF(interp);
    if (GET_TAG(o) != DICT) goto error2;

    return hash_get_keys(o->u.dict);

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'keys', syntax: Dict keys", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

Toy_Type*
mth_dict_pairs(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *o;

    if (arglen != 0) goto error;

    o = SELF(interp);
    if (GET_TAG(o) != DICT) goto error2;

    return hash_get_pairs(o->u.dict);

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'pairs', syntax: Dict keys", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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

    array_append(o->u.vector, item);
    return item;

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'append', syntax: Vector append val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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

    if (! array_set(o->u.vector, item, mpz_get_si(index->u.biginteger))) {
	return new_exception(TE_ARRAYBOUNDARY, "Array boudary error.", interp);
    }

    return item;

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'set', syntax: Vector set pos val", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
	return new_exception(TE_ARRAYBOUNDARY, "Array boudary error.", interp);
    }

    return item;

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'get', syntax: Vector get pos", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'len', syntax: Vector len", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'last', syntax: Vector last", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    return new_exception(TE_SYNTAX, "Syntax error at 'list', syntax: Vector list", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
	    goto loop_continue;
	    break;
	case CTRL_REDO:
	    goto loop;
	    break;
	case CTRL_RETRY:
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
    return new_exception(TE_SYNTAX, "Syntax error at 'list', syntax: Vector each do: {| var | block}", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
	return new_exception(TE_ARRAYBOUNDARY, "Array boudary error.", interp);
    }

    return const_T;

error:
    return new_exception(TE_SYNTAX, "Syntax error at 'swap', syntax: Vector swap pos1 pos2", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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
    if (! cstack_isalive(co->interp->cstack_id)) {
	return new_exception(TE_COOUTOFLIFE, "Co-routine out of life.", interp);
    };

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
	return new_exception(TE_COOUTOFLIFE, "Co-routine out of life.", interp);
    }

    if (interp->co_value) {
	return interp->co_value;
    }

    return const_Nil;
    
error:
    return new_exception(TE_SYNTAX, "Syntax error at 'next', syntax: Coro next", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
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

    co_delete(co->coro_id);
    cstack_release(co->interp->cstack_id);
    co->interp->cstack_id = 0;
    co->interp->cstack = NULL;

    return const_Nil;
    
error:
    return new_exception(TE_SYNTAX, "Syntax error at 'release', syntax: Coro release", interp);

error2:
    return new_exception(TE_TYPE, "Type error.", interp);
}

int
toy_add_methods(Toy_Interp* interp) {
    toy_add_method(interp, "Object", "vars", mth_object_vars, NULL);
    toy_add_method(interp, "Object", "method", mth_object_method, NULL);
    toy_add_method(interp, "Object", "var?", mth_object_get, NULL);
    toy_add_method(interp, "Object", "get", mth_object_get, NULL);
    toy_add_method(interp, "Object", "set!", mth_object_set, NULL);
    toy_add_method(interp, "Object", "type?", mth_object_type, NULL);
    toy_add_method(interp, "Object", "delegate?", mth_object_delegate, NULL);
#if 0 // replace to command eq?
    toy_add_method(interp, "Object", "eq", mth_object_eq, NULL);
#endif
    toy_add_method(interp, "Object", "string", mth_object_tostring, NULL);
    toy_add_method(interp, "Object", "method?", mth_object_getmethod, NULL);
    toy_add_method(interp, "Object", "apply", mth_object_apply, NULL);

    toy_add_method(interp, "Integer", "+", mth_integer_plus, NULL);
    toy_add_method(interp, "Integer", "-", mth_integer_minus, NULL);
    toy_add_method(interp, "Integer", "*", mth_integer_mul, NULL);
    toy_add_method(interp, "Integer", "/", mth_integer_div, NULL);
    toy_add_method(interp, "Integer", "%", mth_integer_mod, NULL);
    toy_add_method(interp, "Integer", "=", mth_integer_eq, NULL);
    toy_add_method(interp, "Integer", "!=", mth_integer_neq, NULL);
    toy_add_method(interp, "Integer", ">", mth_integer_gt, NULL);
    toy_add_method(interp, "Integer", "<", mth_integer_lt, NULL);
    toy_add_method(interp, "Integer", ">=", mth_integer_ge, NULL);
    toy_add_method(interp, "Integer", "<=", mth_integer_le, NULL);
    toy_add_method(interp, "Integer", "++", mth_integer_inc, NULL);
    toy_add_method(interp, "Integer", "--", mth_integer_dec, NULL);
    toy_add_method(interp, "Integer", "each", mth_integer_each, NULL);
    toy_add_method(interp, "Integer", "..", mth_integer_list, NULL);
    toy_add_method(interp, "Integer", "real", mth_integer_toreal, NULL);
    toy_add_method(interp, "Integer", "<<", mth_integer_rol, NULL);
    toy_add_method(interp, "Integer", ">>", mth_integer_ror, NULL);
    toy_add_method(interp, "Integer", "||", mth_integer_or, NULL);
    toy_add_method(interp, "Integer", "&&", mth_integer_and, NULL);
    toy_add_method(interp, "Integer", "^^", mth_integer_xor, NULL);
    toy_add_method(interp, "Integer", "~~", mth_integer_not, NULL);
    toy_add_method(interp, "Integer", "^", mth_integer_power, NULL);

    toy_add_method(interp, "Real", "+", mth_real_plus, NULL);
    toy_add_method(interp, "Real", "-", mth_real_minus, NULL);
    toy_add_method(interp, "Real", "*", mth_real_mul, NULL);
    toy_add_method(interp, "Real", "/", mth_real_div, NULL);
    toy_add_method(interp, "Real", "=", mth_real_eq, NULL);
    toy_add_method(interp, "Real", "!=", mth_real_neq, NULL);
    toy_add_method(interp, "Real", ">", mth_real_gt, NULL);
    toy_add_method(interp, "Real", "<", mth_real_lt, NULL);
    toy_add_method(interp, "Real", ">=", mth_real_ge, NULL);
    toy_add_method(interp, "Real", "<=", mth_real_le, NULL);
    toy_add_method(interp, "Real", "int", mth_real_tointeger, NULL);
    toy_add_method(interp, "Real", "sqrt", mth_real_sqrt, NULL);
    toy_add_method(interp, "Real", "sin", mth_real_sin, NULL);
    toy_add_method(interp, "Real", "cos", mth_real_cos, NULL);
    toy_add_method(interp, "Real", "tan", mth_real_tan, NULL);
    toy_add_method(interp, "Real", "asin", mth_real_asin, NULL);
    toy_add_method(interp, "Real", "acos", mth_real_acos, NULL);
    toy_add_method(interp, "Real", "atan", mth_real_atan, NULL);
    toy_add_method(interp, "Real", "log", mth_real_log, NULL);
    toy_add_method(interp, "Real", "log10", mth_real_log10, NULL);
    toy_add_method(interp, "Real", "exp", mth_real_exp, NULL);
    toy_add_method(interp, "Real", "exp10", mth_real_exp10, NULL);
    toy_add_method(interp, "Real", "pow", mth_real_pow, NULL);

    toy_add_method(interp, "List", "last", mth_list_last, NULL);
    toy_add_method(interp, "List", "item", mth_list_item, NULL);
    toy_add_method(interp, "List", "car", mth_list_item, NULL);
    toy_add_method(interp, "List", "cdr", mth_list_cdr, NULL);
    toy_add_method(interp, "List", "next", mth_list_cdr, NULL);
    toy_add_method(interp, "List", "append!", mth_list_append, NULL);
    toy_add_method(interp, "List", "add", mth_list_add, NULL);
    toy_add_method(interp, "List", "+", mth_list_append, NULL);
    toy_add_method(interp, "List", "each", mth_list_each, NULL);
    toy_add_method(interp, "List", "len", mth_list_len, NULL);
    toy_add_method(interp, "List", "null?", mth_list_isnull, NULL);
    toy_add_method(interp, "List", "join", mth_list_join, NULL);
    toy_add_method(interp, "List", "eval", mth_list_eval, NULL);
    toy_add_method(interp, "List", ".", mth_list_new_append, NULL);
    toy_add_method(interp, "List", "get", mth_list_get, NULL);
    toy_add_method(interp, "List", "filter", mth_list_filter, NULL);
    toy_add_method(interp, "List", "map", mth_list_map, NULL);
    toy_add_method(interp, "List", "concat", mth_list_concat, NULL);
    toy_add_method(interp, "List", "seek", mth_list_seek, NULL);
    toy_add_method(interp, "List", "split", mth_list_split, NULL);
    toy_add_method(interp, "List", "<<", mth_list_unshift, NULL);
    toy_add_method(interp, "List", ">>", mth_list_shift, NULL);
    toy_add_method(interp, "List", "<<-", mth_list_push, NULL);
    toy_add_method(interp, "List", "->>", mth_list_pop, NULL);
    toy_add_method(interp, "List", "inject", mth_list_inject, NULL);

    toy_add_method(interp, "String", "len", mth_string_len, NULL);
    toy_add_method(interp, "String", "=", mth_string_equal, NULL);
    toy_add_method(interp, "String", "!=", mth_string_nequal, NULL);
    toy_add_method(interp, "String", "eval", mth_string_eval, NULL);
    toy_add_method(interp, "String", "+", mth_string_append, NULL);
    toy_add_method(interp, "String", ".", mth_string_concat, NULL);
    toy_add_method(interp, "String", "=~", mth_string_match, NULL);
    toy_add_method(interp, "String", "sub", mth_string_sub, NULL);
    toy_add_method(interp, "String", ">", mth_string_gt, NULL);
    toy_add_method(interp, "String", "<", mth_string_lt, NULL);
    toy_add_method(interp, "String", ">=", mth_string_ge, NULL);
    toy_add_method(interp, "String", "<=", mth_string_le, NULL);
    toy_add_method(interp, "String", "split", mth_string_split, NULL);
    toy_add_method(interp, "String", "int", mth_string_toint, NULL);
    toy_add_method(interp, "String", "real", mth_string_toreal, NULL);
    toy_add_method(interp, "String", "number", mth_string_tonumber, NULL);
    toy_add_method(interp, "String", "rquote", mth_string_torquote, NULL);
    toy_add_method(interp, "String", "fmt", mth_string_format, NULL);

    toy_add_method(interp, "File", "init", mth_file_init, NULL);
    toy_add_method(interp, "File", "open", mth_file_open, NULL);
    toy_add_method(interp, "File", "close", mth_file_close, NULL);
    toy_add_method(interp, "File", "gets", mth_file_gets, NULL);
    toy_add_method(interp, "File", "puts", mth_file_puts, NULL);
    toy_add_method(interp, "File", "flush", mth_file_flush, NULL);
    toy_add_method(interp, "File", "stat", mth_file_stat, NULL);
    toy_add_method(interp, "File", "eof?", mth_file_iseof, NULL);
    toy_add_method(interp, "File", "set!", mth_file_set, NULL);
    toy_add_method(interp, "File", "ready?", mth_file_isready, NULL);

    toy_add_method(interp, "Block", "eval", mth_block_eval, NULL);

    toy_add_method(interp, "Dict", "set", mth_dict_set, NULL);
    toy_add_method(interp, "Dict", "get", mth_dict_get, NULL);
    toy_add_method(interp, "Dict", "set?", mth_dict_isset, NULL);
    toy_add_method(interp, "Dict", "len", mth_dict_len, NULL);
    toy_add_method(interp, "Dict", "unset", mth_dict_unset, NULL);
    toy_add_method(interp, "Dict", "keys", mth_dict_keys, NULL);    
    toy_add_method(interp, "Dict", "pairs", mth_dict_pairs, NULL);    

    toy_add_method(interp, "Vector", "+", mth_vector_append, NULL);
    toy_add_method(interp, "Vector", "append", mth_vector_append, NULL);
    toy_add_method(interp, "Vector", "set", mth_vector_set, NULL);
    toy_add_method(interp, "Vector", "get", mth_vector_get, NULL);
    toy_add_method(interp, "Vector", "len", mth_vector_len, NULL);
    toy_add_method(interp, "Vector", "last", mth_vector_last, NULL);
    toy_add_method(interp, "Vector", "list", mth_vector_list, NULL);
    toy_add_method(interp, "Vector", "each", mth_vector_each, NULL);
    toy_add_method(interp, "Vector", "swap", mth_vector_swap, NULL);

    toy_add_method(interp, "Coro", "next", mth_coro_next, NULL);
    toy_add_method(interp, "Coro", "release", mth_coro_release, NULL);

    return 0;
}
