#ifndef __INTERP__
#define __INTERP__

#include <t_gc.h>
#include "types.h"
#include "hash.h"

Toy_Interp*	new_interp(wchar_t* name, int stack_size, Toy_Interp* parent,
			   int argc, char** argv, char** envp, char *dir);
Toy_Interp*	interp_setup(Toy_Interp* interp, int argc, char** argv, char** envp, char *dir);
int		toy_add_class(Toy_Interp* interp, wchar_t* name, Hash *slot, Toy_Type *delegate) ;
void		toy_add_func(Toy_Interp* interp,
			     wchar_t* name,
			     Toy_Type* native(Toy_Interp* interp, Toy_Type* posargs,
					      Hash* nameargs, int arglen),
			     wchar_t* argspec);
void		toy_add_method(Toy_Interp* interp,
			       wchar_t* class,
			       wchar_t* method,
			       Toy_Type* native(Toy_Interp* interp, Toy_Type* posargs,
						Hash* nameargs, int arglen),
			       wchar_t* argspec);

Toy_Obj_Env*	toy_new_obj_env(Toy_Interp* interp, Toy_Type *cur_object, Toy_Type *self);
int		toy_push_new_obj_env(Toy_Interp* interp, wchar_t* class, Toy_Type* self);
int		toy_push_obj_env(Toy_Interp* interp, Toy_Obj_Env *env);
Toy_Obj_Env*	toy_pop_obj_env(Toy_Interp* interp);
int		toy_push_func_env(Toy_Interp* interp, Hash *localvar, Toy_Func_Env *upenv,
				  Toy_Type *tobe_bind_val, Toy_Func_Trace_Info* trace_info);
Toy_Func_Env*	toy_pop_func_env(Toy_Interp* interp);
Toy_Env*	new_closure_env(Toy_Interp* interp);
int		toy_add_commands(Toy_Interp *interp);
int		toy_add_methods(Toy_Interp *interp);

wchar_t*	get_stack_trace(Toy_Interp* interp, Cell* stack);
wchar_t*	get_script_path(Toy_Interp* interp, int id);
Toy_Type*       script_apply_trace_info(Toy_Type *script, Toy_Func_Trace_Info *trace_info);
#ifdef HAS_GCACHE
void		gcache_clear(Toy_Interp* interp);
#endif

#endif /* __INTERP__ */
