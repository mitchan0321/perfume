#ifndef __TOY__
#define __TOY__

#include "config.h"
#include "types.h"
#include "cell.h"
#include "hash.h"
#include "bulk.h"
#include "binbulk.h"
#include "parser.h"
#include "interp.h"
#include "error.h"
#include "cstack.h"
#include "util.h"
#include "t_gc.h"

Toy_Type *toy_eval_script(Toy_Interp* interp, Toy_Type *script);
Toy_Type *toy_eval(Toy_Interp* interp, Toy_Type *statement, Toy_Env** env);
Toy_Type *toy_expand(Toy_Interp* interp, Toy_Type* obj, Toy_Env** env, Toy_Func_Trace_Info *trace_info);
Toy_Type *toy_expand_list(Toy_Interp* interp, Toy_Type* list, Toy_Env** env, Toy_Func_Trace_Info *trace_info);
Toy_Type *toy_resolv_var(Toy_Interp* interp, Toy_Type* var, int stack_trace, Toy_Func_Trace_Info *trace_info);
Toy_Type *set_closure_var(Toy_Interp *interp, Toy_Type *var, Toy_Type *val);
Toy_Type *toy_resolv_function(Toy_Interp* interp, Toy_Type* obj);
Toy_Type *toy_resolv_object(Toy_Interp* interp, Toy_Type* obj);
Toy_Type *toy_apply_func(Toy_Interp* interp, Toy_Type* func, Toy_Type* posargs, Hash* nameargs, int arglen);
Toy_Type *toy_call_function(Toy_Interp* interp, Toy_Type *script,
		  Toy_Type *posargs, Hash *nameargs, Toy_Type *argspec_list, int arglen);
Toy_Type *toy_resolv_object(Toy_Interp *interp, Toy_Type *object);
Toy_Type *eval_closure(Toy_Interp *interp, Toy_Type *closure, Toy_Func_Trace_Info *trace_info);
Toy_Type *toy_call_init(Toy_Interp *interp, Toy_Type *object, Toy_Type *args);
Toy_Type *toy_call(Toy_Interp *interp, Toy_Type *list);
Toy_Type *toy_yield(Toy_Interp *interp, Toy_Type *closure, Toy_Type *args);
Toy_Type *search_method(Toy_Interp *interp, Toy_Type *object, Toy_Type *method);
Toy_Type *toy_symbol_conv(Toy_Type *atom);
wchar_t	 *to_string_call(Toy_Interp *interp, Toy_Type *obj);
void	  def_global();

/* define control code for return, break, continue, redo, retry and goto */
#define CTRL_RETURN		1
#define CTRL_BREAK		2
#define CTRL_CONTINUE		3
#define CTRL_REDO		4
#define CTRL_RETRY		5
#define CTRL_GOTO		6

/* define co-routine status */
#define CO_STS_INIT		0
#define CO_STS_RUN		1
#define CO_STS_DONE		2

/* define mprotect page size */
#define MP_ALIGN		(0x0fff)
#define MP_PAGESIZE		(0x1000)
#define MP_SPARE		(4096)

#endif /* __TOY__ */
