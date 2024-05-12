#ifndef _GLOBAL_
#define _GLOBAL_

#include <signal.h>
#include <sys/types.h>
#include "toy.h"
#include "types.h"

#ifndef _DEF_GLOBAL_
extern Toy_Type *const_Symbol;
extern Toy_Type *const_Bool;
extern Toy_Type *const_Block;
extern Toy_Type *const_List;
extern Toy_Type *const_Real;
extern Toy_Type *const_Integer;
extern Toy_Type *const_String;
extern Toy_Type *const_Exception;
extern Toy_Type *const_RQuote;
extern Toy_Type *const_Object;
extern Toy_Type *const_Dict;
extern Toy_Type *const_Vector;
extern Toy_Type *const_Coro;
extern Toy_Type *const_Bulk;
extern Toy_Type *const_Intr;
extern Toy_Type *const_Container;
extern Toy_Type *const_Nil;
extern Toy_Type *const_T;
extern Toy_Type *const_Question;
extern Toy_Type *const_Holder;
extern Toy_Type *const_Init;
extern Toy_Type *const_Hash;
extern Toy_Type *const_File;
extern Toy_Type *const_Args;
extern Toy_Type *const_Array;
extern Toy_Type *const_Get;
extern Toy_Type *const_atout;
extern Toy_Type *const_atin;
extern Toy_Type *const_stdin;
extern Toy_Type *const_stdout;
extern Toy_Type *const_stderr;
extern Toy_Type *const_filec;
extern Toy_Type *const_gets;
extern Toy_Type *const_puts;
extern Toy_Type *const_atname;
extern Toy_Type *const_atstacktrace;
extern Toy_Type *const_SIGHUP;
extern Toy_Type *const_SIGINT;
extern Toy_Type *const_SIGQUIT;
extern Toy_Type *const_SIGPIPE;
extern Toy_Type *const_SIGALRM;
extern Toy_Type *const_SIGTERM;
extern Toy_Type *const_SIGURG;
extern Toy_Type *const_SIGCHLD;
extern Toy_Type *const_SIGUSR1;
extern Toy_Type *const_SIGUSR2;
extern Toy_Type *const_attrap;
extern Toy_Type *const_atscriptid;
extern Toy_Type *const_CWD;
extern Toy_Type *const_ENV;
extern Toy_Type *const_ARGV;
extern Toy_Type *const_LIB_PATH;
extern Toy_Type *const_BASE_PATH;
extern Toy_Type *const_DEFAULT_FILE_ENCODING;
extern Toy_Type *const_DEFAULT_SCRIPT_ENCODING;
extern Toy_Type *const_DEFAULT_DIRENT_ENCODING;
extern Toy_Type *const_DEFAULT_ERRSTR_ENCODING;
extern Toy_Type *const_up;
extern Toy_Type *const_VERSION;
extern Toy_Type *const_BUILD;
extern Toy_Type *const_nocase;
extern Toy_Type *const_grep;
extern Toy_Type *const_text;
extern Toy_Type *const_all;
extern Toy_Type *const_string;
extern Toy_Type *const_string_key;
extern Toy_Type *const_delegate;
extern Toy_Type *const_init;
extern Toy_Type *const_int1;
extern Toy_Type *const_new;
extern Toy_Type *const_unknown;
extern Toy_Type *const_out;
extern Toy_Type *const_local;
extern Toy_Type *const_rebase;
extern Toy_Type *const_NotImpliment;
extern Toy_Type *const_then;
extern Toy_Type *const_else;
extern Toy_Type *const_do;
extern Toy_Type *const_catch;
extern Toy_Type *const_fin;
extern Toy_Type *const_to;
extern Toy_Type *const_sep;
extern Toy_Type *const_mode;
extern Toy_Type *const_last;
extern Toy_Type *const_nonewline;
extern Toy_Type *const_nocontrol;
extern Toy_Type *const_nullstring;
extern Toy_Type *const_silent;
extern Toy_Type *const_timeout;
extern Toy_Type *const_debug_hook;
extern Toy_Type *const_ast;
extern Toy_Type *const_default;
extern wchar_t  *const_regex_cache;
#ifdef PROF
extern void *GC_stackbottom;
#endif /* PROF */

extern volatile sig_atomic_t sig_flag;
extern int PageSize;

#else

Toy_Type *const_Symbol;
Toy_Type *const_Bool;
Toy_Type *const_Block;
Toy_Type *const_List;
Toy_Type *const_Real;
Toy_Type *const_Integer;
Toy_Type *const_String;
Toy_Type *const_Exception;
Toy_Type *const_RQuote;
Toy_Type *const_Object;
Toy_Type *const_Dict;
Toy_Type *const_Vector;
Toy_Type *const_Coro;
Toy_Type *const_Bulk;
Toy_Type *const_Intr;
Toy_Type *const_Container;
Toy_Type *const_Nil;
Toy_Type *const_T;
Toy_Type *const_Question;
Toy_Type *const_Holder;
Toy_Type *const_Init;
Toy_Type *const_Hash;
Toy_Type *const_File;
Toy_Type *const_Args;
Toy_Type *const_Array;
Toy_Type *const_Get;
Toy_Type *const_atout;
Toy_Type *const_atin;
Toy_Type *const_stdin;
Toy_Type *const_stdout;
Toy_Type *const_stderr;
Toy_Type *const_filec;
Toy_Type *const_gets;
Toy_Type *const_puts;
Toy_Type *const_atname;
Toy_Type *const_atstacktrace;
Toy_Type *const_SIGHUP;
Toy_Type *const_SIGINT;
Toy_Type *const_SIGQUIT;
Toy_Type *const_SIGPIPE;
Toy_Type *const_SIGALRM;
Toy_Type *const_SIGTERM;
Toy_Type *const_SIGURG;
Toy_Type *const_SIGCHLD;
Toy_Type *const_SIGUSR1;
Toy_Type *const_SIGUSR2;
Toy_Type *const_attrap;
Toy_Type *const_atscriptid;
Toy_Type *const_CWD;
Toy_Type *const_ENV;
Toy_Type *const_ARGV;
Toy_Type *const_LIB_PATH;
Toy_Type *const_BASE_PATH;
Toy_Type *const_DEFAULT_FILE_ENCODING;
Toy_Type *const_DEFAULT_SCRIPT_ENCODING;
Toy_Type *const_DEFAULT_DIRENT_ENCODING;
Toy_Type *const_DEFAULT_ERRSTR_ENCODING;
Toy_Type *const_up;
Toy_Type *const_VERSION;
Toy_Type *const_BUILD;
Toy_Type *const_nocase;
Toy_Type *const_grep;
Toy_Type *const_text;
Toy_Type *const_all;
Toy_Type *const_string;
Toy_Type *const_string_key;
Toy_Type *const_delegate;
Toy_Type *const_init;
Toy_Type *const_int1;
Toy_Type *const_new;
Toy_Type *const_unknown;
Toy_Type *const_out;
Toy_Type *const_local;
Toy_Type *const_rebase;
Toy_Type *const_NotImpliment;
Toy_Type *const_then;
Toy_Type *const_else;
Toy_Type *const_do;
Toy_Type *const_catch;
Toy_Type *const_fin;
Toy_Type *const_to;
Toy_Type *const_sep;
Toy_Type *const_mode;
Toy_Type *const_last;
Toy_Type *const_nonewline;
Toy_Type *const_nocontrol;
Toy_Type *const_nullstring;
Toy_Type *const_silent;
Toy_Type *const_timeout;
Toy_Type *const_debug_hook;
Toy_Type *const_ast;
Toy_Type *const_default;
wchar_t  *const_regex_cache;
int PageSize;
#ifdef PROF
void *GC_stackbottom;
#endif /* PROF */

volatile sig_atomic_t sig_flag;

#endif /* _DEF_GLOBAL_ */

#endif /* _GLOBAL_ */
