#ifdef NCURSES

#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <locale.h>

#include "toy.h"
#include "interp.h"
#include "types.h"
#include "cell.h"
#include "global.h"
#include "array.h"
#include "cstack.h"
#include "util.h"
#include "encoding.h"

Toy_Type*
mth_curses_init(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Hash *self;
    WINDOW *wmain;
	
    self = SELF_HASH(interp);

    setlocale(LC_ALL, "");
    wmain = initscr();
    cbreak();
    noecho();
    nonl();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);

    hash_set_t(self, const_Holder, new_container(wmain));

    return const_T;
}

Toy_Type*
mth_curses_getscreensize(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *w;
    Toy_Type *container;
    int y, x;
    Toy_Type *result;
    Hash *self;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 0) goto error;

    self = SELF_HASH(interp);
    container = hash_get_t(self, const_Holder);
    if (NULL == container) goto error2;
    w = container->u.container;
    getmaxyx(w, y, x);
    result = new_list(NULL);
    list_append(result, new_integer_si(y));
    list_append(result, new_integer_si(x));

    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'get-screen-size', syntax: Screen get-screen-size", interp);
    
error2:
    return new_exception(TE_TYPE, L"Type error.", interp);
}

Toy_Type*
mth_curses_terminate(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 0) goto error;

    endwin();

    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'terminate', syntax: Screen terminate", interp);
}

int
toy_add_method_ncurses(Toy_Interp* interp) {
    toy_add_method(interp, L"Curses", L"init",			mth_curses_init,		NULL);
    toy_add_method(interp, L"Curses", L"get-screen-size",	mth_curses_getscreensize,	NULL);
    toy_add_method(interp, L"Curses", L"terminate",		mth_curses_terminate,		NULL);

    return 0;
}

#endif /* NCURSES */

