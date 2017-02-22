/* $Id: global.c,v 1.11 2010/01/16 04:31:43 mit-sato Exp $ */

#include <unistd.h>
#include "toy.h"
#include "types.h"
#define _DEF_GLOBAL_
#include "global.h"

void def_global() {
    const_Symbol = new_symbol(L"Symbol");
    const_NilClass = new_symbol(L"Nil");
    const_Block = new_symbol(L"Block");
    const_List = new_symbol(L"List");
    const_Real = new_symbol(L"Real");
    const_Integer = new_symbol(L"Integer");
    const_String = new_symbol(L"String");
    const_Exception = new_symbol(L"Exception");
    const_RQuote = new_symbol(L"RQuote");
    const_Object = new_symbol(L"Object");
    const_CallCC = new_symbol(L"CallCC");
    const_Dict = new_symbol(L"Dict");
    const_Vector = new_symbol(L"Vector");
    const_Coro = new_symbol(L"Coro");
    const_Nil = new_nil();
    const_T = new_symbol(L"t");
    const_Question = new_symbol(L"?");
    const_Holder = new_symbol(L"@holder");
    const_Init = new_symbol(L"init");
    const_Hash = new_symbol(L"Hash");
    const_File = new_symbol(L"File");
    const_Args = new_symbol(L"args:");
    const_Array = new_symbol(L"Array");
    const_Get = new_symbol(L"get");
    const_atout = new_symbol(L"@out");
    const_atin = new_symbol(L"@in");
    const_stdin = new_symbol(L"stdin");
    const_stdout = new_symbol(L"stdout");
    const_stderr = new_symbol(L"stderr");
    const_filec = new_symbol(L"file:");
    const_gets = new_symbol(L"gets");
    const_puts = new_symbol(L"puts");
    const_atname = new_symbol(L"@name");
    const_atstacktrace = new_symbol(L"@stack-trace");
    const_SIGHUP = new_symbol(L"SIGHUP");
    const_SIGINT = new_symbol(L"SIGINT");
    const_SIGQUIT = new_symbol(L"SIGQUIT");
    const_SIGPIPE = new_symbol(L"SIGPIPE");
    const_SIGALRM = new_symbol(L"SIGALRM");
    const_SIGTERM = new_symbol(L"SIGTERM");
    const_SIGURG = new_symbol(L"SIGURG");
    const_SIGCHLD = new_symbol(L"SIGCHLD");
    const_SIGUSR1 = new_symbol(L"SIGUSR1");
    const_SIGUSR2 = new_symbol(L"SIGUSR2");
    const_attrap = new_symbol(L"@trap");
    const_atscriptid = new_symbol(L"@script-id");
    const_CWD = new_symbol(L"CWD");
    const_ENV = new_symbol(L"ENV");
    const_ARGV = new_symbol(L"ARGV");
    const_LIB_PATH = new_symbol(L"LIB_PATH");
    const_DEFAULT_FILE_ENCODING = new_symbol(L"DEFAULT_FILE_ENCODING");
    const_DEFAULT_SCRIPT_ENCODING = new_symbol(L"DEFAULT_SCRIPT_ENCODING");
    const_up = new_symbol(L"up:");
    const_VERSION = new_symbol(L"VERSION");
    const_nocase = new_symbol(L"nocase:");
    const_grep = new_symbol(L"grep:");
    const_all = new_symbol(L"all:");
    const_string = new_symbol(L"string");
    const_delegate = new_symbol(L"delegate:");
    const_init = new_symbol(L"init:");
    const_int1 = new_integer_si(1);
    const_new = new_symbol(L"new");
    const_unknown = new_symbol(L"unknown");
    const_out = new_symbol(L"out:");
    const_local = new_symbol(L"local:");
    const_rebase = new_symbol(L"rebase:");
    const_NotImpliment = new_exception(TE_NOTIMPLIMENT, L"Not Impliment.", NULL);
    const_then = new_symbol(L"then:");
    const_else = new_symbol(L"else:");
    const_do = new_symbol(L"do:");
    const_catch = new_symbol(L"catch:");
    const_fin = new_symbol(L"fin:");
    const_to = new_symbol(L"to:");
    const_sep = new_symbol(L"sep:");
    const_mode = new_symbol(L"mode:");
    const_last = new_symbol(L"last:");
    const_nonewline = new_symbol(L"nonewline:");
    const_nocontrol = new_symbol(L"nocontrol:");
    const_nullstring = new_string_str(L"");
    const_silent = new_symbol(L"silent:");
    const_debug_hook = new_symbol(L"debug-hook");
    const_ast = new_symbol(L"*");
    const_regex_cache = L"@REGEX_CACHE";
    PageSize = sysconf(_SC_PAGESIZE);
#ifdef PROF
    void *GC_stackbottom = 0;
#endif /* PROF */
}
