/* $Id: interp.c,v 1.43 2012/03/06 06:09:29 mit-sato Exp $ */

#include <assert.h>
#include <string.h>
#include <signal.h>
#include <wchar.h>
#include "types.h"
#include "cell.h"
#include "interp.h"
#include "config.h"
#include "bulk.h"
#include "parser.h"
#include "toy.h"
#include "global.h"
#include "cstack.h"

Toy_Type* cmd_pwd(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen);
static Toy_Type* parse_env(char* buff, char sep);

#ifndef PROF
void*
malloc_wrapper(size_t new) {
    void *p;

    p = GC_malloc(new);
    ALLOC_SAFE(p);

    return p;
}

void*
realloc_wrapper(void* ptr, size_t old, size_t new) {
    void *p;

    p = GC_malloc(new);
    ALLOC_SAFE(p);
    memcpy(p, ptr, (old<new)?old:new);
    GC_free(ptr);

    return p;
}

void
free_wrapper(void* ptr, size_t old) {
    return;
}
#endif /* PROF */

Toy_Interp*
new_interp(wchar_t* name, int stack_size, Toy_Interp* parent,
	   int argc, char **argv, char **envp) {

    Toy_Interp *interp;
    Toy_Type *l, *t;
    
    static int gc_init_once = 0;

    if (0 == gc_init_once) {
#ifndef PROF
	mp_set_memory_functions(malloc_wrapper,
				realloc_wrapper,
				free_wrapper);
#endif /* PROF */
	GC_INIT();
	init_cstack();
	gc_init_once = 1;
    }

    interp = GC_MALLOC(sizeof(Toy_Interp));
    ALLOC_SAFE(interp);
    memset(interp, 0, sizeof(Toy_Interp));

    if (NULL == parent) {

	def_global();
	
	interp->obj_stack = (Toy_Obj_Env**)(GC_MALLOC(sizeof(Toy_Obj_Env*) * stack_size));
	ALLOC_SAFE(interp->obj_stack);
	memset(interp->obj_stack, 0, (sizeof(Toy_Obj_Env*) * stack_size));

	interp->func_stack = (Toy_Func_Env**)(GC_MALLOC(sizeof(Toy_Func_Env*) * stack_size));
	ALLOC_SAFE(interp->func_stack);
	memset(interp->func_stack, 0, (sizeof(Toy_Func_Env*) * stack_size));

	interp->funcs = new_hash();
	if (NULL == interp->funcs) return NULL;

	interp->classes = new_hash();
	if (NULL == interp->classes) return NULL;

	interp->globals = new_hash();
	if (NULL == interp->globals) return NULL;

	interp->scripts = new_hash();
	if (NULL == interp->scripts) return NULL;

	interp->name = name;
	interp->stack_size = stack_size;
	interp->cur_obj_stack = -1;
	interp->cur_func_stack = -1;
	interp->script_id = 0;
	interp->trace = 0;
	interp->trace_fd = 2;
	interp->debug = 0;
	interp->debug_in = 0;
#ifdef HAS_GCACHE
	interp->gcache = new_hash();
	interp->cache_hit = 0;
	interp->cache_missing = 0;
#endif /* HAS_GCACHE */
	hash_set_t(interp->scripts, const_atscriptid,
		   new_integer_si(interp->script_id));

	interp->cstack = 0;
	interp->cstack_size = 0;
	interp->coroid = 0;
	interp->co_parent = 0;
	interp->co_value = 0;
	interp->co_calling = 0;
	interp->last_status = const_Nil;
	interp->trace_info = NULL;
	
	sig_flag = 0;

	if (argv && envp) {
	    interp_setup(interp, argc, argv, envp);
	}

    } else {

	interp->name = parent->name;
	interp->stack_size = stack_size;

	interp->cur_func_stack = 0;
	interp->func_stack = (Toy_Func_Env**)(GC_MALLOC(sizeof(Toy_Func_Env*) * stack_size));
	ALLOC_SAFE(interp->func_stack);
	memset(interp->func_stack, 0, sizeof(Toy_Func_Env*) * stack_size);
	interp->func_stack[interp->cur_func_stack] = parent->func_stack[parent->cur_func_stack];

	interp->cur_obj_stack = 0;
	interp->obj_stack = (Toy_Obj_Env**)(GC_MALLOC(sizeof(Toy_Obj_Env*) * stack_size));
	ALLOC_SAFE(interp->obj_stack);
	memset(interp->obj_stack, 0, sizeof(Toy_Func_Env*) * stack_size);

	interp->obj_stack[interp->cur_obj_stack] = parent->obj_stack[parent->cur_obj_stack];
	interp->funcs = parent->funcs;
	interp->classes = parent->classes;

	interp->globals = new_hash();
	if (NULL == interp->globals) return NULL;
	/* global dict mirroring */
	l = hash_get_pairs(parent->globals);
	while (l) {
	    t = list_get_item(l);
	    hash_set_t(interp->globals, list_get_item(t), list_next(t));
	    l = list_next(l);
	}
	
	interp->scripts = parent->scripts;
	interp->script_id = parent->script_id;
#ifdef HAS_GCACHE
	interp->gcache = parent->gcache;
	interp->cache_hit = parent->cache_hit;
	interp->cache_missing = parent->cache_missing;
#endif /* HAS_GCACHE */
	interp->trace = parent->trace;
	interp->trace_fd = parent->trace_fd;
	interp->debug = parent->debug;
	interp->debug_in = parent->debug_in;
	interp->cstack = 0;
	interp->cstack_size = 0;
	interp->coroid = 0;
	interp->co_parent = 0;
	interp->co_value = 0;
	interp->co_calling = 0;
	interp->last_status = const_Nil;
	interp->trace_info = NULL;
    }

    return interp;
}

Toy_Interp*
interp_setup(Toy_Interp* interp, int argc, char **argv, char **envp) {
    Toy_Type *delegate;
    Hash *gdict;
    int i;
    Toy_Type *argl, *l;
    Toy_Type *any;
    Toy_Type *env, *envl, *envv;
    Toy_Type *setupl;
    Toy_Type *obj;
    Toy_Func_Trace_Info *trace_info;

    delegate = new_list(const_Object);

    obj = new_object(L"Main", new_hash(), delegate);
    toy_push_new_obj_env(interp, L"Toplevel", obj);
    trace_info = GC_MALLOC(sizeof(Toy_Func_Trace_Info));
    ALLOC_SAFE(trace_info);
    trace_info->line = 0;
    trace_info->object_name = obj;
    trace_info->func_name = new_symbol(L"Main");
    trace_info->statement = new_statement(new_list(new_symbol(L"(perfumesh)")), 0);
    interp->trace_info = trace_info;

    toy_push_func_env(interp, new_hash(), NULL, NULL, trace_info);

    /* make world */

    toy_add_class(interp, L"Functions", interp->funcs, delegate);
    toy_add_class(interp, L"Nil", NULL, delegate);
    toy_add_class(interp, L"Integer", NULL, delegate);
    toy_add_class(interp, L"Real", NULL, delegate);
    toy_add_class(interp, L"String", NULL, delegate);
    toy_add_class(interp, L"List", NULL, delegate);
    toy_add_class(interp, L"Block", NULL, delegate);
    toy_add_class(interp, L"RQuote", NULL, delegate);
    toy_add_class(interp, L"Object", NULL, NULL);
    toy_add_class(interp, L"File", NULL, delegate);
    toy_add_class(interp, L"Dict", NULL, delegate);
    toy_add_class(interp, L"Vector", NULL, delegate);
    toy_add_class(interp, L"Coro", NULL, delegate);

    toy_add_commands(interp);
    toy_add_methods(interp);

    /* set VERSION */
    hash_set_t(interp->globals, const_VERSION, new_string_str(VERSION));

    /* set ARGV */
    l = argl = new_list(NULL);
    for (i=0; i<argc; i++) {
	l = list_append(l, new_string_str(to_wchar(argv[i])));
    }
    gdict = interp->globals;
    hash_set_t(gdict, const_ARGV, argl);
    hash_set_t(gdict, const_LIB_PATH, new_list(new_string_str(LIB_PATH)));
    hash_set_t(gdict, const_DEFAULT_FILE_ENCODING, new_symbol(DEFAULT_FILE_ENCODING));
    hash_set_t(gdict, const_DEFAULT_SCRIPT_ENCODING, new_symbol(DEFAULT_SCRIPT_ENCODING));
    
    /* set ENV */
    l = envl = new_list(new_symbol(L"dict"));
    env = toy_call(interp, envl);
    if (GET_TAG(env) == DICT) {
	for (; *envp; envp++) {
	    envv = parse_env(*envp, '=');
	    l = envl = new_list(env);
	    l = list_append(l, new_symbol(L"set"));
	    l = list_append(l, list_get_item(envv));
	    l = list_append(l, list_get_item(list_next(envv)));
	    toy_call(interp, envl);
	}
    }
    hash_set_t(interp->globals, const_ENV, env);

    cmd_pwd(interp, new_list(NULL), new_hash(), 0);

    /* load initial setup file "setup.prfm" */
    setupl = new_list(new_symbol(L"load"));
    list_append(setupl, new_string_str(SETUP_FILE));
    any = toy_eval_script(interp,
			  script_apply_trace_info(new_script(new_list(new_statement(setupl, 0))),
						  trace_info));
    if (GET_TAG(any) == EXCEPTION) {
	setupl = new_list(new_symbol(L"load"));
	list_append(setupl, new_string_str(SETUP_FILE2));
	any = toy_eval_script(interp, 
			      script_apply_trace_info(new_script(new_list(new_statement(setupl, 0))),
						      trace_info));

	if (GET_TAG(any) == EXCEPTION) {

	    fwprintf(stderr, L"Exception occurd in setup script file at load '%ls', %ls.\n",
		     SETUP_FILE2, to_string(any));
	}
    }

    return interp;
}

int
toy_add_class(Toy_Interp* interp, wchar_t* name, Hash* slot, Toy_Type *delegate) {
    Toy_Type *obj;
    Hash *h;

    if (slot == NULL) {
	if (NULL == (h = new_hash())) return 0;
    } else {
	h = slot;
    }

    obj = new_object(name, h, delegate);
    if (NULL == hash_set(interp->classes, name, obj)) return 0;

    return 1;
}

static Toy_Type* parse_argspec(wchar_t* argspec);

void
toy_add_func(Toy_Interp* interp,
	     wchar_t* name,
	     Toy_Type* native(Toy_Interp*, Toy_Type*, Hash*, int),
	     wchar_t* argspec) {

    Toy_Type *argspec_list;
    Toy_Type *o;

    argspec_list = parse_argspec(argspec);
    o = new_native(native, argspec_list);

    hash_set(interp->funcs, name, o);
}

void
toy_add_method(Toy_Interp* interp,
	       wchar_t* class,
	       wchar_t* method,
	       Toy_Type* native(Toy_Interp*, Toy_Type*, Hash*, int),
	       wchar_t* argspec) {

    Toy_Type *argspec_list;
    Toy_Type *o;
    Toy_Type *h;

    h = hash_get(interp->classes, class);
    if (NULL == h) {
	assert(0);
    }

    argspec_list = parse_argspec(argspec);
    o = new_native(native, argspec_list);

    hash_set(h->u.object.slots, method, o);
}

static Toy_Type*
parse_argspec(wchar_t* argspec) {
    Toy_Type *l;
    Cell *c;
    wchar_t *p;

    if (argspec == NULL) return NULL;

    p = argspec;
    c = new_cell(L"");
    l = new_list(NULL);

    if (wcslen(argspec) == 0) {
	list_append(l, const_Nil);
	return l;
    }

    while (*p) {
	if (*p == L',') {
	    list_append(l, new_symbol(cell_get_addr(c)));
	    c = new_cell(L"");
	} else {
	    cell_add_char(c, *p);
	}
	p++;
    }
    list_append(l, new_symbol(cell_get_addr(c)));

    return l;
}

static Toy_Type*
parse_env(char* buff, char sep) {
    Toy_Type *l;
    Cell *c;
    char *p;

    if (buff == NULL) return NULL;

    p = buff;
    c = new_cell(L"");
    l = new_list(NULL);

    if (strlen(buff) == 0) {
	list_append(l, const_Nil);
	return l;
    }

    while (*p) {
	if (*p == sep) {
	    list_append(l, new_string_str(cell_get_addr(c)));
	    c = new_cell(L"");
	    p++;
	    cell_add_str(c, to_wchar(p));
	    break;
	} else {
	    cell_add_char(c, (wchar_t)*p);
	}
	p++;
    }
    list_append(l, new_string_str(cell_get_addr(c)));

    return l;
}

Toy_Obj_Env*
toy_new_obj_env(Toy_Interp *interp, Toy_Type *cur_object, Toy_Type *self) {
    Toy_Obj_Env *env;

    env = GC_MALLOC(sizeof(Toy_Obj_Env));
    ALLOC_SAFE(env);

    env->cur_object = cur_object;
    env->cur_object_slot = cur_object->u.object.slots;
    env->self = self;

    return env;
}

int
toy_push_new_obj_env(Toy_Interp* interp, wchar_t* class, Toy_Type* self) {
    Toy_Obj_Env *env;
    Toy_Type *o; 

    if ((interp->cur_obj_stack+1) >= interp->stack_size) return 0;

    o = new_object(L"Object", new_hash(), new_list(const_Object));
    if (NULL == (env = toy_new_obj_env(interp, o, o))) {
	return 0;
    }

    interp->cur_obj_stack++;
    interp->obj_stack[interp->cur_obj_stack] = env;

    hash_set_t(interp->obj_stack[interp->cur_obj_stack]->cur_object_slot,
	       const_atname, new_symbol(class));

    return 1;
}

int
toy_push_obj_env(Toy_Interp *interp, Toy_Obj_Env *env) {
    if ((interp->cur_obj_stack+1) >= interp->stack_size) return 0;

    interp->cur_obj_stack++;
    interp->obj_stack[interp->cur_obj_stack] = env;

    return 1;
}

Toy_Obj_Env*
toy_pop_obj_env(Toy_Interp* interp) {
    Toy_Obj_Env *env;

    env = interp->obj_stack[interp->cur_obj_stack];
    interp->obj_stack[interp->cur_obj_stack] = NULL;
    interp->cur_obj_stack--;

    return env;
}

int
toy_push_func_env(Toy_Interp* interp, Hash *localvar, Toy_Func_Env *upenv, Toy_Type *tobe_bind_val, Toy_Func_Trace_Info *trace_info) {
    Toy_Func_Env *env;

    if ((interp->cur_func_stack+1) >= interp->stack_size) return 0;

    env = GC_MALLOC(sizeof(Toy_Func_Env));
    ALLOC_SAFE(env);

    env->localvar = localvar;
    env->upstack = upenv;
    if (trace_info) {
	env->trace_info = trace_info;
    } else {
	env->trace_info = interp->func_stack[interp->cur_func_stack]->trace_info;
    }
    env->script_id = interp->script_id;
    env->tobe_bind_val = tobe_bind_val;

    interp->cur_func_stack++;
    interp->func_stack[interp->cur_func_stack] = env;

    return 1;
}

Toy_Func_Env*
toy_pop_func_env(Toy_Interp* interp) {
    Toy_Func_Env *env;

    if (0 == interp->cur_func_stack) return NULL;
    
    env = interp->func_stack[interp->cur_func_stack];
    interp->func_stack[interp->cur_func_stack] = NULL;
    interp->cur_func_stack--;

    return env;
}

Toy_Env*
new_closure_env(Toy_Interp* interp) {
    Toy_Env *env;

    env = GC_MALLOC(sizeof(Toy_Env));
    ALLOC_SAFE(env);

    env->func_env = interp->func_stack[interp->cur_func_stack];
    env->object_env = interp->obj_stack[interp->cur_obj_stack];
    env->tobe_bind_val = NULL;

    return env;
}


wchar_t*
get_stack_trace(Toy_Interp *interp, Cell *stack) {
    wchar_t *buff;
    int i;

    buff = GC_MALLOC(256*sizeof(wchar_t));
    ALLOC_SAFE(buff);

    if (interp->trace_info) {
	swprintf(buff, 256, L"%ls:%d: %ls in %ls::%ls\n",
		 get_script_path(interp, interp->func_stack[interp->cur_func_stack]->script_id),
		 interp->trace_info->line,
		 to_print(interp->trace_info->statement),
		 (GET_TAG(interp->func_stack[interp->cur_func_stack]->trace_info->object_name) != NIL ?
		  to_string(interp->func_stack[interp->cur_func_stack]->trace_info->object_name) : L"self"),
		 (interp->func_stack[interp->cur_func_stack]->trace_info->func_name ?
		  to_string(interp->func_stack[interp->cur_func_stack]->trace_info->func_name) : L"???"));
	buff[255] = 0;
	cell_add_str(stack, buff);
    }

    for (i=interp->cur_func_stack; i>0; i--) {
	swprintf(buff, 256, L"%ls:%d: %ls in %ls::%ls\n",
		 get_script_path(interp, interp->func_stack[i-1]->script_id),
		 interp->func_stack[i]->trace_info->line,
		 to_print(interp->func_stack[i]->trace_info->statement),
		 (GET_TAG(interp->func_stack[i-1]->trace_info->object_name) != NIL ?
		  		to_string(interp->func_stack[i-1]->trace_info->object_name) : L"self"),
		 (interp->func_stack[i-1]->trace_info->func_name ?
		  		to_string(interp->func_stack[i-1]->trace_info->func_name) : L"???"));
	buff[255] = 0;
	cell_add_str(stack, buff);
    }

    return cell_get_addr(stack);
}

wchar_t*
get_script_path(Toy_Interp* interp, int script_id) {
    Hash *h;
    Toy_Type *cont;
    Toy_Script *s;

    if (script_id <= 0) return L"-";

    h = interp->scripts;
    cont = hash_get(h, to_string(new_integer_si(script_id)));
    if (NULL == cont) return L"???";

    s = (Toy_Script*)cont->u.container;
    return cell_get_addr(s->path->u.string);
}

Toy_Type*
script_apply_trace_info(Toy_Type *script, Toy_Func_Trace_Info *trace_info) {
    Toy_Type *l, *item, *e, *elem;
    
    l = script->u.statement_list;
    while (l && (item = list_get_item(l))) {
	item->u.statement.trace_info->line = trace_info->line;
	item->u.statement.trace_info->object_name = trace_info->object_name;
	item->u.statement.trace_info->func_name = trace_info->func_name;
	item->u.statement.trace_info->statement = trace_info->statement;

	e = item->u.statement.item_list;
	while (e && (elem = list_get_item(e))) {
	    if (GET_TAG(elem) == SCRIPT) {
		script_apply_trace_info(elem, trace_info);
	    }
	    e = list_next(e);
	}
	
	l = list_next(l);
    }

    return script;
}
