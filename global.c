/* $Id: global.c,v 1.11 2010/01/16 04:31:43 mit-sato Exp $ */

#include <unistd.h>
#include "toy.h"
#include "types.h"
#define _DEF_GLOBAL_
#include "global.h"

void def_global() {
    const_Symbol = new_symbol("Symbol");
    const_NilClass = new_symbol("Nil");
    const_Block = new_symbol("Block");
    const_List = new_symbol("List");
    const_Real = new_symbol("Real");
    const_Integer = new_symbol("Integer");
    const_String = new_symbol("String");
    const_Exception = new_symbol("Exception");
    const_RQuote = new_symbol("RQuote");
    const_Object = new_symbol("Object");
    const_CallCC = new_symbol("CallCC");
    const_Dict = new_symbol("Dict");
    const_Vector = new_symbol("Vector");
    const_Coro = new_symbol("Coro");
    const_Nil = new_nil();
    const_T = new_symbol("t");
    const_Question = new_symbol("?");
    const_Holder = new_symbol("@holder");
    const_Init = new_symbol("init");
    const_Hash = new_symbol("Hash");
    const_File = new_symbol("File");
    const_Args = new_symbol("args:");
    const_Array = new_symbol("Array");
    const_Get = new_symbol("get");
    const_atout = new_symbol("@out");
    const_atin = new_symbol("@in");
    const_stdin = new_symbol("stdin");
    const_stdout = new_symbol("stdout");
    const_stderr = new_symbol("stderr");
    const_filec = new_symbol("file:");
    const_gets = new_symbol("gets");
    const_puts = new_symbol("puts");
    const_atname = new_symbol("@name");
    const_atstacktrace = new_symbol("@stack-trace");
    const_SIGHUP = new_symbol("SIGHUP");
    const_SIGINT = new_symbol("SIGINT");
    const_SIGQUIT = new_symbol("SIGQUIT");
    const_SIGPIPE = new_symbol("SIGPIPE");
    const_SIGALRM = new_symbol("SIGALRM");
    const_SIGTERM = new_symbol("SIGTERM");
    const_SIGURG = new_symbol("SIGURG");
    const_SIGCHLD = new_symbol("SIGCHLD");
    const_SIGUSR1 = new_symbol("SIGUSR1");
    const_SIGUSR2 = new_symbol("SIGUSR2");
    const_attrap = new_symbol("@trap");
    const_atscriptid = new_symbol("@script-id");
    const_CWD = new_symbol("CWD");
    const_ENV = new_symbol("ENV");
    const_up = new_symbol("up:");
    const_VERSION = new_symbol("VERSION");
    const_nocase = new_symbol("nocase:");
    const_grep = new_symbol("grep:");
    const_all = new_symbol("all:");
    const_string = new_symbol("string");
    const_delegate = new_symbol("delegate:");
    const_init = new_symbol("init:");
    const_int1 = new_integer_si(1);
    const_new = new_symbol("new");
    const_unknown = new_symbol("unknown");
    const_out = new_symbol("out:");
    const_local = new_symbol("local:");
    const_rebase = new_symbol("rebase:");
    const_NotImpliment = new_exception(TE_NOTIMPLIMENT, "Not Impliment.", NULL);
    const_then = new_symbol("then:");
    const_else = new_symbol("else:");
    const_do = new_symbol("do:");
    const_catch = new_symbol("catch:");
    const_fin = new_symbol("fin:");
    const_to = new_symbol("to:");
    const_sep = new_symbol("sep:");
    const_mode = new_symbol("mode:");
    const_last = new_symbol("last:");
    const_nonewline = new_symbol("nonewline:");
    const_nocontrol = new_symbol("nocontrol:");
    const_nullstring = new_string_str("");
    const_silent = new_symbol("silent:");
    const_debug_hook = new_symbol("debug-hook");
    const_ast = new_symbol("*");
    const_regex_cache = "@REGEX_CACHE";
    PageSize = sysconf(_SC_PAGESIZE);
#ifdef PROF
    void *GC_stackbottom = 0;
#endif /* PROF */
}
