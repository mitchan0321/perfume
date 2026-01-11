#ifndef __INTERP__
#define __INTERP__

#include <t_gc.h>
#include <unistd.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "types.h"
#include "hash.h"


#define _SAFE_OUT_BUF_SIZE (512)
#define _SAFE_BACKUP_SIZE (128*1024)
#define ALLOC_OUT(x)   write(2, x, strlen(x));
#define ALLOC_SAFE(x)  if (! x) _alloc_safe((char*)__FILE__ , (char*)__FUNCTION__ , (int)__LINE__);

#define INIT_ALLOC_HOOK(x) {                                                                \
    extern jmp_buf *_safe_jmp_buf;                                                          \
    extern char _safe_out_buf[_SAFE_OUT_BUF_SIZE];                                          \
    extern char *_safe_backup;                                                              \
    if (NULL == _safe_jmp_buf) {                                                            \
        _safe_jmp_buf = malloc(sizeof(jmp_buf));                                            \
        if (NULL == _safe_jmp_buf) {                                                        \
            ALLOC_OUT("at INIT_ALLOC_HOOK, failed set jumb buffer.\n");                     \
            exit(255);                                                                      \
        }                                                                                   \
    }                                                                                       \
    x = setjmp(*_safe_jmp_buf);                                                             \
    if (0 != x) {                                                                           \
        if (_safe_backup == NULL) {                                                         \
            ALLOC_OUT("at INIT_ALLOC_HOOK, no reserved memory, exit.\n");                   \
            exit(254);                                                                      \
        }                                                                                   \
        free(_safe_jmp_buf);                                                                \
        _safe_jmp_buf = NULL;                                                               \
        _safe_backup = NULL;                                                                \
        GC_gcollect();                                                                      \
        ALLOC_OUT("at INIT_ALLOC_HOOK, return errored.\n");                                 \
    }                                                                                       \
}

void            _alloc_safe(char *file_name, char *func_name, int line);
Toy_Interp*     new_interp(wchar_t* name, int stack_size, Toy_Interp* parent,
                           int argc, char** argv, char** envp, char *dir);
Toy_Interp*     interp_setup(Toy_Interp* interp, int argc, char** argv, char** envp, char *dir);
int             toy_add_class(Toy_Interp* interp, wchar_t* name, Hash *slot, Toy_Type *delegate) ;
void            toy_add_func(Toy_Interp* interp,
                             wchar_t* name,
                             Toy_Type* native(Toy_Interp* interp, Toy_Type* posargs,
                                              Hash* nameargs, int arglen),
                             wchar_t* argspec);
void            toy_add_method(Toy_Interp* interp,
                               wchar_t* class,
                               wchar_t* method,
                               Toy_Type* native(Toy_Interp* interp, Toy_Type* posargs,
                                                Hash* nameargs, int arglen),
                               wchar_t* argspec);

Toy_Obj_Env*    toy_new_obj_env(Toy_Interp* interp, Toy_Type *cur_object, Toy_Type *self);
int             toy_push_new_obj_env(Toy_Interp* interp, wchar_t* class, Toy_Type* self);
int             toy_push_obj_env(Toy_Interp* interp, Toy_Obj_Env *env);
Toy_Obj_Env*    toy_pop_obj_env(Toy_Interp* interp);
int             toy_push_func_env(Toy_Interp* interp, Hash *localvar, Toy_Func_Env *upenv,
                                  Toy_Type *tobe_bind_val, Toy_Func_Trace_Info* trace_info);
Toy_Func_Env*   toy_pop_func_env(Toy_Interp* interp);
Toy_Env*        new_closure_env(Toy_Interp* interp);
int             toy_add_commands(Toy_Interp *interp);
int             toy_add_methods(Toy_Interp *interp);

wchar_t*        get_stack_trace(Toy_Interp* interp, Cell* stack);
wchar_t*        get_script_path(Toy_Interp* interp, int id);
Toy_Type*       script_apply_trace_info(Toy_Type *script, Toy_Func_Trace_Info *trace_info);

#endif /* __INTERP__ */
