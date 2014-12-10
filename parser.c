/* $Id: parser.c,v 1.20 2012/03/01 12:33:31 mit-sato Exp $ */

#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <global.h>
#include <string.h>
#include "parser.h"
#include "error.h"
#include "global.h"
#include "cell.h"

#ifndef ishexnumber
#define ishexnumber(x)	(((x>='0')&&(x<='9'))||((x>='a')&&(x<='f'))||((x>='A')&&(x<='F')))
#endif /* ishexnumber */

static Toy_Type* toy_parse_getmacro(Toy_Type *statement);
static Toy_Type* toy_parse_initmacro(Toy_Type *statement);
static Toy_Type* toy_parse_join_statement(Toy_Type *statement, int line);
static Toy_Type* toy_parse_set_paramno(Toy_Type *statement);

Toy_Type*
toy_parse_start(Bulk *src) {
    Toy_Type *script;

    script = toy_parse_script(src, 0);
    if (NULL == script) goto assert;

    return script;

assert:
    /* may be not enough memory */
    return NULL;
}

Toy_Type*
toy_parse_script(Bulk *src, char endc) {
    Toy_Type *script;
    Toy_Type *statement;
    Toy_Type *l;
    int c;
    char *buff;
    Cell *msg;

    buff = GC_MALLOC(256);
    ALLOC_SAFE(buff);

    script = new_script(NULL);
    if (NULL == script) goto assert;
    
    l = new_list(NULL);
    script->u.statement_list = l;

    while (EOF != bulk_is_eof(src)) {
	c = bulk_getchar(src);
	if (0 == endc) {
	    if ((EOF == c) || (endc == c)) {
		return script;
	    }
	} else {
	    if (endc == c) {
		return script;
	    }
	    if (EOF == c) {
		snprintf(buff, 100, "Imbalanced close char '%c' at line %d.",
			 endc, bulk_get_line(src));
		msg = new_cell(buff);
		return new_exception(TE_PARSECLOSE, cell_get_addr(msg), NULL);
	    }
	}
	bulk_ungetchar(src);

	statement = toy_parse_statement(src, endc);
	if (statement == NULL) goto assert;

	if (GET_TAG(statement) != STATEMENT) goto parse_error;

	statement->u.statement_list = toy_parse_initmacro(statement->u.statement_list);
	statement->u.statement_list = toy_parse_getmacro(statement->u.statement_list);
	statement->u.statement_list = toy_parse_join_statement(statement->u.statement_list,
								    bulk_get_line(src));
	toy_parse_set_paramno(statement);

	if (list_length(statement->u.statement_list) > 0) {
	    l = list_append(l, statement);
	}

    }

    return script; 

parse_error:
    return statement;

assert:
    /* may be not enough memory */
    return NULL;
}

Toy_Type*
toy_parse_statement(Bulk *src, char endc) {
    char c, c_tmp;
    Toy_Type *newstatement;
    Toy_Type *any;
    Toy_Type *l;
    char *buff;
    Cell *msg;

    buff = GC_MALLOC(256);
    ALLOC_SAFE(buff);

    l = new_list(NULL);
    if (NULL == l) goto assert;

    newstatement = new_statement(l, bulk_get_line(src));
    if (NULL == newstatement) goto assert;

    while (EOF != (c = bulk_getchar(src))) {
	if (endc == c) {
	    bulk_ungetchar(src);
	    return newstatement;
	}
	if (';' == c) return newstatement;

	switch (c) {
	case ' ': case '\t': case '\r':	case '\n':
	    /* skip white space */
	    break;

	case '#':
	    /* skip comment */
	    c = bulk_getchar(src);
	    while (1) {
		if (EOF == c) goto parse_end;
		if ('\n' == c) break;
		c = bulk_getchar(src);
	    }
	    bulk_ungetchar(src);
	    break;

	case ')': case '}': case ']':
	    snprintf(buff, 100, "Illegal close char '%c' at line %d.",
		     c, bulk_get_line(src));
	    msg = new_cell(buff);
	    any = new_exception(TE_PARSEBADCHAR, cell_get_addr(msg), NULL);
	    goto parse_error;

	case '(':
	    any = toy_parse_list(src, ')');
	    if (NULL == any) goto assert;
	    if (GET_TAG(any) != LIST) goto parse_error;
	    l = list_append(l, any);
	    break;

	case '{':
	    any = toy_parse_block(src, '}');
	    if (NULL == any) goto assert;
	    if (GET_TAG(any) != BLOCK) goto parse_error;
	    l = list_append(l, any);
	    break;
	    
	case '[':
	    any = toy_parse_eval(src, ']');
	    if (NULL == any) goto assert;
	    if (GET_TAG(any) != EVAL) goto parse_error;
	    l = list_append(l, any);
	    break;
	    
	case '\"':
	    any = toy_parse_string(src, '\"');
	    if (NULL == any) goto assert;
	    if (GET_TAG(any) != STRING) goto parse_error;
	    l = list_append(l, any);
	    break;

	case '\'':
	    any = toy_parse_rquote(src, '\'');
	    if (NULL == any) goto assert;
	    if (GET_TAG(any) != RQUOTE) goto parse_error;
	    l = list_append(l, any);
	    break;

	case ';':
	    return newstatement;

	case ',':
	    l = list_append(l, new_symbol(","));
	    break;

	case '`':
	    l = list_append(l, new_symbol("`"));
	    break;

	case '|':
	    c_tmp = bulk_getchar(src);
	    if (index(ENDCHAR, c_tmp) != NULL) {
		any = toy_parse_list(src, '|');
		if (GET_TAG(any) != LIST) goto parse_error;
		l = list_append(l, new_bind(any));
		return newstatement;
	    }
	    bulk_ungetchar(src);

	    /* FALL THRU */

	default:
	    /* parse symbol */

	    bulk_ungetchar(src);
	    any = toy_parse_symbol(src, ENDCHAR);
	    if (NULL == any) goto assert;
	    if (GET_TAG(any) != SYMBOL) goto parse_error;
	    l = list_append(l, toy_symbol_conv(any));
	}

	newstatement->u.statement.trace_info->line = bulk_get_line(src);
    }

parse_end:
    newstatement->u.statement.trace_info->line = bulk_get_line(src);
    if (0 == endc) {
	return newstatement;
    } else {
	snprintf(buff, 100, "Imbalanced close char '%c' at line %d.",
		 endc, bulk_get_line(src));
	msg = new_cell(buff);
	return new_exception(TE_PARSECLOSE, cell_get_addr(msg), NULL);
    }

parse_error:
    /* parse error */
    return any;

assert:
    /* may be not enough memory */
    return NULL;
}

Toy_Type*
toy_parse_symbol(Bulk *src, char *endc) {
    Toy_Type *symbol;
    Cell *newcell;
    int c;
    char *buff;

    buff = GC_MALLOC(128);
    ALLOC_SAFE(buff);

    newcell = new_cell("");
    if (NULL == newcell) goto assert;
    
    c = bulk_getchar(src);
    while (1) {
	if (EOF == c) {
	    goto complite;
	}

	buff[0] = c; buff[1] = 0;
	if (NULL != strstr((const char*)endc, (const char*)buff)) {
	    bulk_ungetchar(src);
	    goto complite;
	}

	if (NULL == cell_add_char(newcell, c)) goto assert;

	c = bulk_getchar(src);
    }

complite:
    symbol = new_symbol(cell_get_addr(newcell));
    return symbol;

assert:
    return NULL;
}

Toy_Type*
toy_parse_string(Bulk *src, char endc) {
    Toy_Type *str;
    Cell *newcell;
    int c;
    Cell *msg;
    char *buff;

    buff = GC_MALLOC(256);
    ALLOC_SAFE(buff);

    str = new_string_str("");
    if (NULL == str) goto assert;

    newcell = str->u.string;

    c = bulk_getchar(src);
    while (endc != c) {
	if (EOF == c) goto parse_error;
	if ('\\' == c) {
	    c = bulk_getchar(src);
	    if (EOF == c) goto parse_error;
	    switch (c) {
	    case 't':
		c = '\t';
		break;
	    case 'r':
		c = '\r';
		break;
	    case 'n':
		c = '\n';
		break;
	    }
	}

	if (NULL == cell_add_char(newcell, c)) goto assert;
	c = bulk_getchar(src);
    }
    return str;

assert:
    return NULL;

parse_error:
    snprintf(buff, 100, "Imbalanced close string '%c' at line %d.",
	     endc, bulk_get_line(src));
    msg = new_cell(buff);
    return new_exception(TE_PARSESTRING, cell_get_addr(msg), NULL);
}

Toy_Type*
toy_parse_rquote(Bulk *src, char endc) {
    Toy_Type *str;
    Cell *newcell;
    int c;
    Cell *msg;
    char *buff;

    buff = GC_MALLOC(256);
    ALLOC_SAFE(buff);

    str = new_rquote("");
    if (NULL == str) goto assert;

    newcell = str->u.rquote;

    c = bulk_getchar(src);
    while (endc != c) {
	if (EOF == c) goto parse_error;
	if ('\\' == c) {
	    c = bulk_getchar(src);
	    if (EOF == c) goto parse_error;
	    switch (c) {
	    case '\\':
		c = '\\';
		break;
	    case '\'':
		c = '\'';
		break;
	    default:
		bulk_ungetchar(src);
		c = '\\';
	    }
	}

	if (NULL == cell_add_char(newcell, c)) goto assert;
	c = bulk_getchar(src);
    }
    return str;

assert:
    return NULL;

parse_error:
    snprintf(buff, 100, "Imbalanced close rquote '%c' at line %d.",
	     endc, bulk_get_line(src));
    msg = new_cell(buff);
    return new_exception(TE_PARSESTRING, cell_get_addr(msg), NULL);
}

Toy_Type*
toy_parse_list(Bulk *src, char endc) {
    char c;
    Toy_Type *newlist;
    Toy_Type *any;
    Toy_Type *l;
    char *buff;
    Cell *msg;

    buff = GC_MALLOC(256);
    ALLOC_SAFE(buff);

    newlist = new_list(NULL);
    if (NULL == newlist) goto assert;

    l = newlist;

    while (EOF != (c = bulk_getchar(src))) {
	if (endc == c) return newlist;

	switch (c) {
	case ' ': case '\t': case '\r': case ';': case '\n': 
	    /* skip white space */
	    break;

	case '#':
	    /* skip comment */
	    c = bulk_getchar(src);
	    while (1) {
		if (EOF == c) goto parse_end;
		if ('\n' == c) break;
		c = bulk_getchar(src);
	    }
	    bulk_ungetchar(src);
	    break;

	case ')': case '}': case ']':
	    snprintf(buff, 100, "Illegal close list char '%c' at line %d.",
		     c, bulk_get_line(src));
	    msg = new_cell(buff);
	    any = new_exception(TE_PARSEBADCHAR, cell_get_addr(msg), NULL);
	    goto parse_error;

	case '(':
	    any = toy_parse_list(src, ')');
	    if (NULL == any) goto assert;
	    if (GET_TAG(any) != LIST) goto parse_error;
	    l = list_append(l, any);
	    break;

	case '{':
	    any = toy_parse_block(src, '}');
	    if (NULL == any) goto assert;
	    if (GET_TAG(any) != BLOCK) goto parse_error;
	    l = list_append(l, any);
	    break;
	    
	case '[':
	    any = toy_parse_eval(src, ']');
	    if (NULL == any) goto assert;
	    if (GET_TAG(any) != EVAL) goto parse_error;
	    l = list_append(l, any);
	    break;
	    
	case '\"':
	    any = toy_parse_string(src, '\"');
	    if (NULL == any) goto assert;
	    if (GET_TAG(any) != STRING) goto parse_error;
	    l = list_append(l, any);
	    break;

	case '\'':
	    any = toy_parse_rquote(src, '\'');
	    if (NULL == any) goto assert;
	    if (GET_TAG(any) != RQUOTE) goto parse_error;
	    l = list_append(l, any);
	    break;

	case ',':
	    l = list_append(l, new_symbol(","));
	    break;

	case '`':
	    l = list_append(l, new_symbol("`"));
	    break;

	default:
	    /* parse symbol */

	    bulk_ungetchar(src);
	    any = toy_parse_symbol(src, ENDCHAR);
	    if (NULL == any) goto assert;
	    if (GET_TAG(any) != SYMBOL) goto parse_error;

	    if (strcmp(cell_get_addr(any->u.symbol.cell), ".") == 0) {
		any = toy_parse_list(src, ')');
		if (NULL == any) goto assert;

		list_set_cdr(l, toy_symbol_conv(list_get_item(any)));
		if (NULL == list_get_item(l)) list_set_car(l, new_nil());
		bulk_ungetchar(src);

	    } else {
		l = list_append(l, toy_symbol_conv(any));
	    }
	}
    }

parse_end:
    snprintf(buff, 100, "Imbalanced close list char '%c' at line %d.",
	     endc, bulk_get_line(src));
    msg = new_cell(buff);
    return new_exception(TE_PARSECLOSE, cell_get_addr(msg), NULL);

parse_error:
    /* parse error */
    return any;

assert:
    /* may be not enough memory */
    return NULL;
}

Toy_Type*
toy_parse_block(Bulk *src, char endc) {
    Toy_Type *script;
    Toy_Type *newblock;

    script = toy_parse_script(src, '}');
    if (NULL == script) goto assert;
    if (GET_TAG(script) != SCRIPT) return script;

    newblock = new_block(script);
    if (NULL == newblock) goto assert;

    return newblock;

assert:
    return NULL;
}

Toy_Type*
toy_parse_eval(Bulk* src, char endc) {
    Toy_Type *script;
    Toy_Type *neweval;

    script = toy_parse_script(src, ']');
    if (NULL == script) goto assert;
    if (GET_TAG(script) != SCRIPT) return script;

    neweval = new_eval(script);
    if (NULL == neweval) goto assert;

    return neweval;

assert:
    return NULL;
}

Toy_Type* 
toy_symbol_conv(Toy_Type *a) {
    char *addr, *p;
    mpz_t s;

    if (NULL == a) return new_nil();
    if (GET_TAG(a) != SYMBOL) return a;

    addr = cell_get_addr(a->u.symbol.cell);
    if (strcmp(addr, S_NIL) == 0) {
	return new_nil();
    }
    if (strcmp(addr, ".") == 0) {
	return a;
    }
    if (strcmp(addr, "..") == 0) {
	return a;
    }
    if (strcmp(addr, "-") == 0) {
	return a;
    }
    if (addr[0] == '$') {
	return new_ref(&addr[1]);
    }

    /* is integer ? */

    p = cell_get_addr(a->u.symbol.cell);
    if (! ((p[0] == '-') || (isdigit(p[0])))) goto non_integer;

    if (p[0] == '-') p++;
    while (*p) {
	if (! isdigit(*p)) goto non_integer;
	p++;
    }

    mpz_init(s);
    mpz_set_str(s, cell_get_addr(a->u.symbol.cell), 10);
    return new_integer(s);

non_integer:

    /* is real ? */

    p = cell_get_addr(a->u.symbol.cell);
    if (! ((p[0] == '-') || (p[0] == '.') || (isdigit(p[0]))))
	return a;

    if ((p[0] == '-') || (p[0] == '.')) p++;
    while (isdigit(*p)) {
	p++;
    }

    if (*p == '.') {
	p++;
	while (isdigit(*p)) {
	    p++;
	}
    }

    if ((*p == 'E') || (*p == 'e')) {
	p++;
	if ((*p == '+') || (*p == '-') || isdigit(*p)) {
	    p++;
	    while (*p) {
		if (! isdigit(*p)) goto non_real;
		p++;
	    }
	    return new_real(atof(cell_get_addr(a->u.symbol.cell)));
	}
    } else if (*p == 0) {
	return new_real(atof(cell_get_addr(a->u.symbol.cell)));
    }

non_real:

    /* is hex decimal ? */

    p = cell_get_addr(a->u.symbol.cell);
    if (strlen((const char*)p) < 3) goto non_hex;

    if ((p[0] == '0') && ((p[1] == 'x') || (p[1] == 'X'))) {
	p++; p++;
	while (*p) {
	    if (! ishexnumber(*p)) goto non_hex;
	    p++;
	}

	mpz_init(s);
	mpz_set_str(s, &(cell_get_addr(a->u.symbol.cell)[2]), 16);
	return new_integer(s);
    }

non_hex:

    return a;
}

/** for debug **/

int print_script(Toy_Type *script, int indent);
int print_statement(Toy_Type *st, int indent);
int print_indent(int indent);
int print_object(Toy_Type *obj, int indent);
int print_list(Toy_Type *l, int indent);

void parser_debug_dump(Toy_Type *script) {
    int indent = 0;

    print_script(script, indent);
}

int
print_script(Toy_Type *script, int indent) {
    Toy_Type *l, *st;

    l = script->u.statement_list;
    do {
	st = list_get_item(l);
	print_statement(st, indent);

	l = list_next(l);

    } while (l);

    return 0;
}

int print_statement(Toy_Type *st, int indent) {
    Toy_Type *item;

    print_indent(indent);
    if (NULL == st) {
	printf("statement is nil.\n");
	return 0;
    }
    printf("--- statement, line=%d\n", st->u.statement.trace_info->line);
    item = st->u.statement.item_list;
    do {
	print_object(list_get_item(item), indent);
	item = list_next(item);
    } while (item);

    return 0;
}

int print_indent(int indent) {
    int i;
    for (i=0; i<(indent*4); i++) {
	putchar(' ');
    }
    return 1;
}

int print_object(Toy_Type *obj, int indent) {
    Cell *ocell;
    Toy_Type *olist;
    Toy_Type *script;
    
    print_indent(indent);
    
    if (! IS_TOY_OBJECT(obj)) {
	printf("not a toy object\n");
	return 0;
    }

    printf("type=%s: ", toy_get_type_str(obj));

    switch GET_TAG(obj) {
    case NIL:
	printf("%s\n", to_string(obj));
	break;

    case SYMBOL:
	ocell = obj->u.symbol.cell;
	printf("%s\n", cell_get_addr(ocell));
	break;

    case REF:
	ocell = obj->u.ref.cell;
	printf("%s\n", cell_get_addr(ocell));
	break;

    case INTEGER:
	printf("%s\n", to_string(obj));
	break;

    case REAL:
	printf("%s\n", to_string(obj));
	break;

    case STRING:
	ocell = obj->u.string;
	printf("\"%s\"\n", cell_get_addr(ocell));
	break;

    case STATEMENT:
	printf("not yet\n");
	break;

    case SCRIPT:
	printf("not yet\n");
	break;

    case LIST:
	printf("\n");
	olist = obj;
	indent++;
	print_list(olist, indent);

	break;

    case BLOCK:
	printf("\n");
	script = obj->u.block_body;
	indent++;
	print_script(script, indent);

	break;
	
    case EVAL:
	printf("\n");
	script = obj->u.eval_body;
	indent++;
	print_script(script, indent);

	break;
	
    case NATIVE:
    case CLOSURE:
    case EXCEPTION:
    case FUNC:
    case OBJECT:
	break;

    default:
	assert(0);
    }

    return 1;
}

int
print_list(Toy_Type *l, int indent) {
    if (IS_LIST_NULL(l)) {
	print_indent(indent);
	printf("nil.\n");
	return 1;
    }

    while (l) {
	print_object(list_get_item(l), indent);
	l = list_next(l);
	if (l && (GET_TAG(l) != LIST)) {
	    print_object(l, indent);
	    break;
	}
    }

    return 1;
}

/** end debug **/

static Toy_Type*
toy_parse_getmacro(Toy_Type *statement) {
    Toy_Type *l, *result, *cur, *prev;

    result = l = new_list(NULL);
    cur = NULL; prev = NULL;
    while (statement) {
	cur = list_get_item(statement);

	if ((GET_TAG(cur) == SYMBOL) &&
	    strcmp(cell_get_addr(cur->u.symbol.cell), ",") == 0) {
	    
	    if (prev != NULL) {
		statement = list_next(statement);
		if (IS_LIST_NULL(statement)) {
		    l = list_append(l, cur);		    
		} else {
		    cur = list_get_item(statement);
		    list_set_car(l, new_getmacro(prev, cur));
		}
	    } else {
		l = list_append(l, cur);
	    }
	} else {
	    if (GET_TAG(cur) == LIST) {
		l = list_append(l, toy_parse_getmacro(cur));
	    } else {
		l = list_append(l, cur);
	    }
	}

	statement = list_next(statement);
	if (GET_TAG(statement) != LIST) {
	    list_set_cdr(l, statement);
	    break;
	}
	prev = cur;
    }

    return result;
}

static Toy_Type*
toy_parse_initmacro(Toy_Type *statement) {
    Toy_Type *l, *result, *cur, *class, *param;

    result = l = new_list(NULL);
    
    while (statement) {
	cur = list_get_item(statement);

	if ((GET_TAG(cur) == SYMBOL) &&
	    strcmp(cell_get_addr(cur->u.symbol.cell), "`") == 0) {

	    if (list_length(statement) >= 2) {

		statement = list_next(statement);
		class = list_get_item(statement);
		statement = list_next(statement);
		param = list_get_item(statement);

		if (GET_TAG(param) == LIST) {
		    l = list_append(l, new_initmacro(class, toy_parse_initmacro(param)));
		} else {
		    l = list_append(l, new_initmacro(class, param));
		}

	    } else {

		l = list_append(l, cur);
	    }
	} else {
	    l = list_append(l, cur);
	}

	statement = list_next(statement);
    }

    return result;
}

static Toy_Type*
toy_parse_set_paramno(Toy_Type *o) {
    int n = 0;
    Toy_Type *l;
    Toy_Type *i;

    l = list_next(o->u.statement.item_list);
    while (l) {
	i = list_get_item(l);
	if (GET_TAG(i) == SYMBOL) {
	    if (IS_NAMED_SYM(i) || IS_SWITCH_SYM(i)) {
		n = TAG_MAX_PARAMNO;
		break;
	    }
	}
	n ++;
	l = list_next(l);
    }

    if (n < TAG_MAX_PARAMNO) {
	CLEAR_PARAMNO(o);
	SET_PARAMNO(o, n);
    } else {
	SET_PARAMNO(o, TAG_MAX_PARAMNO);
    }

    return o;
}

static Toy_Type*
toy_parse_join_statement(Toy_Type *statement, int line) {
    Toy_Type *result, *src, *cur;

    result = new_list(NULL);
    src = new_list(NULL);

    while (statement) {
	cur = list_get_item(statement);

	if ((GET_TAG(cur) == SYMBOL) &&
	    strcmp(cell_get_addr(cur->u.symbol.cell), "\\") == 0) {

	    if (IS_LIST_NULL(result)) {
		result = src;

	    } else {

		result = new_eval(new_script(new_list(new_statement(result, line))));
		list_append(src, result);
		result = src;
	    }

	    src = new_list(NULL);

	} else if ((GET_TAG(cur) == SYMBOL) &&
	    strcmp(cell_get_addr(cur->u.symbol.cell), ":") == 0) {

	    src = new_list(new_eval(new_script(new_list(new_statement(src, line)))));

	} else {

	    list_append(src, cur);
	}

	statement = list_next(statement);
    }

    if (! IS_LIST_NULL(src)) {
	if (IS_LIST_NULL(result)) {

	    return src;

	} else {

	    result = new_eval(new_script(new_list(new_statement(result, line))));
	    list_append(src, result);
	    result = src;
	}
    }

    return result;
}
