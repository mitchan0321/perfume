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
func_curses_init(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *wmain;
	
    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 0) goto error;

    setenv("ESCDELAY", "100", 1);
    setlocale(LC_ALL, "");
    wmain = initscr();
    start_color();
    cbreak();
    noecho();
    nonl();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);

    return new_container(wmain, L"CURSES");

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-init', syntax: curs-init", interp);
}

Toy_Type*
func_curses_getscreensize(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *w;
    Toy_Type *container;
    int y, x;
    Toy_Type *result;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    container = list_get_item(posargs);
    if (GET_TAG(container) != CONTAINER) goto error;
    if (wcscmp(cell_get_addr(container->u.container.desc), L"CURSES") != 0) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-get-screen-size', Bad descriptor.", interp);
    }
    w = container->u.container.data;

    getmaxyx(w, y, x);
    result = new_list(NULL);
    list_append(result, new_integer_si(y));
    list_append(result, new_integer_si(x));

    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-get-screen-size', syntax: curs-get-screen-size window", interp);
}

Toy_Type*
func_curses_terminate(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 0) goto error;

    endwin();
    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-terminate', syntax: terminate", interp);
}

Toy_Type*
func_curses_createwindow(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *w, *wsub;
    Toy_Type *container;
    int y, x, py, px, nline, ncol;
    Toy_Type *arg;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 5) goto error;
    
    container = list_get_item(posargs);
    if (GET_TAG(container) != CONTAINER) goto error;
    if (wcscmp(cell_get_addr(container->u.container.desc), L"CURSES") != 0) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-create-window', Bad descriptor.", interp);
    }
    w = container->u.container.data;
    posargs = list_next(posargs);

    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    y = mpz_get_si(arg->u.biginteger);

    posargs = list_next(posargs);
    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    x = mpz_get_si(arg->u.biginteger);

    posargs = list_next(posargs);
    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    nline = mpz_get_si(arg->u.biginteger);

    posargs = list_next(posargs);
    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    ncol = mpz_get_si(arg->u.biginteger);

    getbegyx(w, py, px);
    if (-1 == py) py = 0;
    if (-1 == px) px = 0;
    wsub = subwin(w, nline, ncol, y + py, x + px);
    if (NULL == wsub) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-create-window', May be bad parameter.", interp);
    }
    notimeout(wsub, FALSE);
    intrflush(wsub, FALSE);
    keypad(wsub, TRUE);

    return new_container(wsub, L"CURSES");

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-create-window', syntax: curs-create-window window y x lines columns", interp);
}

Toy_Type*
func_curses_newwindow(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *w, *wsub;
    Toy_Type *container;
    int y, x, py, px, nline, ncol;
    Toy_Type *arg;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 5) goto error;
    
    container = list_get_item(posargs);
    if (GET_TAG(container) != CONTAINER) goto error;
    if (wcscmp(cell_get_addr(container->u.container.desc), L"CURSES") != 0) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-new-window', Bad descriptor.", interp);
    }
    w = container->u.container.data;
    posargs = list_next(posargs);

    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    y = mpz_get_si(arg->u.biginteger);

    posargs = list_next(posargs);
    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    x = mpz_get_si(arg->u.biginteger);

    posargs = list_next(posargs);
    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    nline = mpz_get_si(arg->u.biginteger);

    posargs = list_next(posargs);
    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    ncol = mpz_get_si(arg->u.biginteger);

    getbegyx(w, py, px);
    if (-1 == py) py = 0;
    if (-1 == px) px = 0;
    wsub = newwin(nline, ncol, y + py, x + px);
    if (NULL == wsub) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-new-window', May be bad parameter.", interp);
    }
    notimeout(wsub, FALSE);
    intrflush(wsub, FALSE);
    keypad(wsub, TRUE);

    return new_container(wsub, L"CURSES");

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-new-window', syntax: curs-new-window window y x lines columns", interp);
}

Toy_Type*
func_curses_clear(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *w;
    Toy_Type *container;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    container = list_get_item(posargs);
    if (GET_TAG(container) != CONTAINER) goto error;
    if (wcscmp(cell_get_addr(container->u.container.desc), L"CURSES") != 0) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-clear', Bad descriptor.", interp);
    }
    w = container->u.container.data;
    posargs = list_next(posargs);

    wclear(w);
    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-clear', syntax: curs-clear window", interp);
}

Toy_Type*
func_curses_print(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *w;
    Toy_Type *container;
    Toy_Type *arg;
    int y, x;
    Toy_Type *message, *enc;
    int iencoder;
    Cell *c;
    encoder_error_info *enc_error_info;

    if (hash_get_length(nameargs) > 0) goto error;
    if ((arglen != 3) && (arglen != 5)) goto error;

    container = list_get_item(posargs);
    if (GET_TAG(container) != CONTAINER) goto error;
    if (wcscmp(cell_get_addr(container->u.container.desc), L"CURSES") != 0) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-print', Bad descriptor.", interp);
    }
    w = container->u.container.data;
    posargs = list_next(posargs);

    message = list_get_item(posargs);
    if (STRING != GET_TAG(message)) goto error;
    posargs = list_next(posargs);

    enc = list_get_item(posargs);
    if (STRING != GET_TAG(enc)) goto error;
    posargs = list_next(posargs);
    
    if (arglen == 5) {
	arg = list_get_item(posargs);
	if (GET_TAG(arg) != INTEGER) goto error;
	y = mpz_get_si(arg->u.biginteger);
	posargs = list_next(posargs);

	arg = list_get_item(posargs);
	if (GET_TAG(arg) != INTEGER) goto error;
	x = mpz_get_si(arg->u.biginteger);

	wmove(w, y, x);
    }

    iencoder = get_encoding_index(cell_get_addr(enc->u.string));
    if (-1 == iencoder) {
	return new_exception(TE_BADENCODER, L"Bad encoder specified.", interp);
    }
    enc_error_info = GC_MALLOC(sizeof(encoder_error_info));
    ALLOC_SAFE(enc_error_info);
    c = encode_unicode_to_raw(message->u.string, iencoder, enc_error_info);
    if (NULL == c) {
	return new_exception(TE_BADENCODEBYTE, enc_error_info->message, interp);
    }
    wprintw(w, "%s", to_char(cell_get_addr(c)));

    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-print', syntax: curs-print window message encoding [ y x ]", interp);
}

Toy_Type*
func_curses_refresh(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *w;
    Toy_Type *container;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    container = list_get_item(posargs);
    if (GET_TAG(container) != CONTAINER) goto error;
    if (wcscmp(cell_get_addr(container->u.container.desc), L"CURSES") != 0) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-refresh', Bad descriptor.", interp);
    }
    w = container->u.container.data;
    posargs = list_next(posargs);

    wrefresh(w);
    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-refresh', syntax: curs-refresh window", interp);
}

Toy_Type*
func_curses_color(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *w;
    Toy_Type *container, *arg;
    int y, x, len, color, attr;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 6) goto error;

    container = list_get_item(posargs);
    if (GET_TAG(container) != CONTAINER) goto error;
    if (wcscmp(cell_get_addr(container->u.container.desc), L"CURSES") != 0) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-color', Bad descriptor.", interp);
    }
    w = container->u.container.data;
    posargs = list_next(posargs);

    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    y = mpz_get_si(arg->u.biginteger);
    posargs = list_next(posargs);
    
    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    x = mpz_get_si(arg->u.biginteger);
    posargs = list_next(posargs);
    
    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    len = mpz_get_si(arg->u.biginteger);
    posargs = list_next(posargs);

    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    color = mpz_get_si(arg->u.biginteger);
    posargs = list_next(posargs);

    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    attr = mpz_get_si(arg->u.biginteger);
    posargs = list_next(posargs);


    if ((x >= 0) && (y >= 0)) {
	wmove(w, y, x);
    }
    wchgat(w, len, attr, color, NULL);

    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-color', syntax: curs-color window y x len color attr", interp);
}

typedef struct _render_encode {
    wchar_t *display_char;
    int display_width;
    int display_position;
} Render_Encode;

Toy_Type*
func_curses_render_line(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *w;
    Toy_Type *container, *arg;
    int window_y, view_x, tab_width;
    Toy_Type *disp_string, *encoding;
    int win_size_y, win_size_x;
    int iencoder;
    int i, slen;
    wchar_t *p;
    Render_Encode *rendaring_data;
    encoder_error_info *enc_error_info;
    Cell *codep;
    Cell *result;
	
    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 6) goto error;

    container = list_get_item(posargs);
    if (GET_TAG(container) != CONTAINER) goto error;
    if (wcscmp(cell_get_addr(container->u.container.desc), L"CURSES") != 0) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-render-line', Bad descriptor.", interp);
    }
    w = container->u.container.data;
    posargs = list_next(posargs);

    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    window_y = mpz_get_si(arg->u.biginteger);
    posargs = list_next(posargs);
    
    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    view_x = mpz_get_si(arg->u.biginteger);
    posargs = list_next(posargs);
    
    arg = list_get_item(posargs);
    if (GET_TAG(arg) != STRING) goto error;
    disp_string = arg;
    posargs = list_next(posargs);

    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    tab_width = mpz_get_si(arg->u.biginteger);
    posargs = list_next(posargs);

    arg = list_get_item(posargs);
    if (GET_TAG(arg) != STRING) goto error;
    encoding = arg;
    posargs = list_next(posargs);

    getmaxyx(w, win_size_y, win_size_x);

    iencoder = get_encoding_index(cell_get_addr(encoding->u.string));
    if (-1 == iencoder) {
	return new_exception(TE_BADENCODER, L"Bad encoder specified.", interp);
    }
    
    slen = cell_get_length(disp_string->u.string);
    rendaring_data = GC_MALLOC(sizeof(Render_Encode) * slen);
    ALLOC_SAFE(rendaring_data);
    enc_error_info = GC_MALLOC(sizeof(encoder_error_info));
    ALLOC_SAFE(enc_error_info);
    p = cell_get_addr(disp_string->u.string);
    for (i=0; i<slen; i++) {
	if ((p[i] < 0x20) || (p[i] == 0x7f)) {
	    /* control character encoding */
	    rendaring_data[i].display_char = L" ";
	    rendaring_data[i].display_width = 1;
	}
	else if (p[i] < 0x7f) {
	    /* ASCII character encoding */
	    codep = new_cell(NULL);
	    cell_add_char(codep, p[i]);
	    rendaring_data[i].display_char = cell_get_addr(codep);
	    rendaring_data[i].display_width = 1;
	} else {
	    /* multi display width character */
	    codep = new_cell(NULL);
	    cell_add_char(codep, p[i]);
	    result = encode_unicode_to_raw(codep, iencoder, enc_error_info);
	    if (NULL == result) {
		return new_exception(TE_BADENCODEBYTE, enc_error_info->message, interp);
	    }
	    rendaring_data[i].display_char = cell_get_addr(result);
	    if ((p[i] >= 0xFF61) && (p[i] <= 0xFF9F)) {
		/* JISX0201 (single width) */
		rendaring_data[i].display_width = 1;
	    } else {
		/* otherwise (multi width) */
		rendaring_data[i].display_width = 2;
	    }
	}
    }

    rendaring_data[0].display_position = 0;
    for (i=1; i<slen; i++) {
	if (p[i-1] == 0x09) {
	    rendaring_data[i].display_position = 
		((rendaring_data[i-1].display_position + tab_width) / tab_width) * tab_width;
	} else {
	    rendaring_data[i].display_position = 
		rendaring_data[i-1].display_position + rendaring_data[i-1].display_width;
	}
    }
    for (i=0; i<slen; i++) {
	if (((rendaring_data[i].display_position - view_x) >= 0) &&
	    ((rendaring_data[i].display_position - view_x + rendaring_data[i].display_width) <= (win_size_x))) {
	    wmove(w, window_y, rendaring_data[i].display_position - view_x);
	    wprintw(w, "%s", to_char(rendaring_data[i].display_char));
	}
    }
    
    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-render-line', syntax: curs-render-line window window-y view-x string tab-width encoding", interp);
}

Toy_Type*
func_curses_box(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *w;
    Toy_Type *container;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    container = list_get_item(posargs);
    if (GET_TAG(container) != CONTAINER) goto error;
    if (wcscmp(cell_get_addr(container->u.container.desc), L"CURSES") != 0) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-box', Bad descriptor.", interp);
    }
    w = container->u.container.data;

    // box(w, ACS_VLINE, ACS_HLINE);
    // wborder(w, '|', '|', '-', '-', '+', '+', '+', '+');
    wborder(w, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');    
    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-box', syntax: curs-box window", interp);
}

Toy_Type*
func_curses_setcolor(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *w;
    Toy_Type *container, *colorp;
    int icolorp;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 2) goto error;

    container = list_get_item(posargs);
    if (GET_TAG(container) != CONTAINER) goto error;
    if (wcscmp(cell_get_addr(container->u.container.desc), L"CURSES") != 0) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-set-color', Bad descriptor.", interp);
    }
    w = container->u.container.data;
    posargs = list_next(posargs);

    colorp = list_get_item(posargs);
    if (GET_TAG(colorp) != INTEGER) goto error;
    icolorp = mpz_get_si(colorp->u.biginteger);

    wcolor_set(w, icolorp, NULL);
    
    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-set-color', syntax: curs-set-color window color-pair", interp);
}

Toy_Type*
func_curses_setbgcolor(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *w;
    Toy_Type *container, *colorp;
    int icolorp;
    int y, x, i, j;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 2) goto error;

    container = list_get_item(posargs);
    if (GET_TAG(container) != CONTAINER) goto error;
    if (wcscmp(cell_get_addr(container->u.container.desc), L"CURSES") != 0) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-set-bgcolor', Bad descriptor.", interp);
    }
    w = container->u.container.data;
    posargs = list_next(posargs);

    colorp = list_get_item(posargs);
    if (GET_TAG(colorp) != INTEGER) goto error;
    icolorp = mpz_get_si(colorp->u.biginteger);

    // bkgd(icolorp);

    wcolor_set(w, icolorp, NULL);
    getmaxyx(w, y, x);
    for (i=0; i<y; i++) {
	for (j=0; j<x; j++) {
	    wmove(w, i, j);
	    wprintw(w, " ");
	}
    }
    
    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-set-bgcolor', syntax: curs-bgcolor window color-number", interp);
}

Toy_Type*
func_curses_setoverlay(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *ow, *dw;
    Toy_Type *ocontainer, *dcontainer;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 2) goto error;

    ocontainer = list_get_item(posargs);
    if (GET_TAG(ocontainer) != CONTAINER) goto error;
    if (wcscmp(cell_get_addr(ocontainer->u.container.desc), L"CURSES") != 0) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-set-overlay', Bad descriptor.", interp);
    }
    ow = ocontainer->u.container.data;
    posargs = list_next(posargs);

    dcontainer = list_get_item(posargs);
    if (GET_TAG(dcontainer) != CONTAINER) goto error;
    if (wcscmp(cell_get_addr(dcontainer->u.container.desc), L"CURSES") != 0) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-set-overlay', Bad descriptor.", interp);
    }
    dw = dcontainer->u.container.data;
    posargs = list_next(posargs);

    overlay(ow, dw);
    
    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-set-overlay', syntax: curs-set-overlay over-window dest-window", interp);
}


Toy_Type*
func_curses_destroywindow(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *w;
    Toy_Type *container;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    container = list_get_item(posargs);
    if (GET_TAG(container) != CONTAINER) goto error;
    if (wcscmp(cell_get_addr(container->u.container.desc), L"CURSES") != 0) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-destroy-window', Bad descriptor.", interp);
    }
    w = container->u.container.data;
    posargs = list_next(posargs);

    delwin(w);
    container->u.container.desc = new_cell(L"CURSES-deleted");
    
    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-destroy-window', syntax: curs-destroy-window window", interp);
}

Toy_Type*
func_curses_move(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *w;
    Toy_Type *container;
    Toy_Type *arg;
    int y, x;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 3) goto error;

    container = list_get_item(posargs);
    if (GET_TAG(container) != CONTAINER) goto error;
    if (wcscmp(cell_get_addr(container->u.container.desc), L"CURSES") != 0) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-move', Bad descriptor.", interp);
    }
    w = container->u.container.data;
    posargs = list_next(posargs);

    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    y = mpz_get_si(arg->u.biginteger);
    posargs = list_next(posargs);

    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    x = mpz_get_si(arg->u.biginteger);
    posargs = list_next(posargs);

    wmove(w, y, x);

    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-move', syntax: curs-move window y x", interp);
}

Toy_Type*
func_curses_add_color(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    int pair, fg, bg;
    Toy_Type *arg;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 3) goto error;

    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    pair = mpz_get_si(arg->u.biginteger);
    posargs = list_next(posargs);

    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    fg = mpz_get_si(arg->u.biginteger);
    posargs = list_next(posargs);

    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    bg = mpz_get_si(arg->u.biginteger);
    posargs = list_next(posargs);

    init_pair(pair, fg,	bg);

    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-add-color', syntax: curs-add-color pair fg-color bg-color", interp);
}

Toy_Type*
func_curses_keyin(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *w;
    Toy_Type *container, *encoding;
    int itimeout, in, iencoder;
    Toy_Type *arg;
    wchar_t buff[32];

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 2) goto error;

    container = list_get_item(posargs);
    if (GET_TAG(container) != CONTAINER) goto error;
    if (wcscmp(cell_get_addr(container->u.container.desc), L"CURSES") != 0) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-keyin', Bad descriptor.", interp);
    }
    w = container->u.container.data;
    posargs = list_next(posargs);

    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    itimeout = mpz_get_si(arg->u.biginteger);
    posargs = list_next(posargs);

    encoding = list_get_item(posargs);
    if (GET_TAG(encoding) != STRING) goto error;
    posargs = list_next(posargs);

    iencoder = get_encoding_index(cell_get_addr(encoding->u.string));
    if (-1 == iencoder) {
	return new_exception(TE_BADENCODER, L"Bad encoder specified.", interp);
    }

    // to be continued...

    wtimeout(w, itimeout);
    in = wgetch(w);

    if (-1 == in) return const_Nil;

    if (in >= 256) {
	if ((in >= KEY_F(0)) && (in <= KEY_F(63))) {
	    swprintf(buff, 32, L"KEY_F%d", in-KEY_F(0));	    
	    return new_symbol(buff);
	}
	swprintf(buff, 32, L"%s", keyname(in));
	return new_symbol(buff);
    }

    return new_integer_si(in);
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-keyin', syntax: curs-keyin window timeout", interp);
}

int
toy_add_func_ncurses(Toy_Interp* interp) {
    toy_add_func(interp, L"curs-init",		func_curses_init,		NULL);
    toy_add_func(interp, L"curs-get-screen-size",func_curses_getscreensize,	L"window");
    toy_add_func(interp, L"curs-terminate",	func_curses_terminate,		NULL);
    toy_add_func(interp, L"curs-create-window",	func_curses_createwindow,	L"window,y,x,line,column");
    toy_add_func(interp, L"curs-new-window",	func_curses_newwindow,		L"window,y,x,line,column");
    toy_add_func(interp, L"curs-clear",		func_curses_clear,		L"window");
    toy_add_func(interp, L"curs-print",		func_curses_print,		L"window,message,encoding,y,x");
    toy_add_func(interp, L"curs-refresh",	func_curses_refresh,		L"window");
    toy_add_func(interp, L"curs-color",		func_curses_color,		L"window,y,x,len,color,attr");
    toy_add_func(interp, L"curs-render-line",	func_curses_render_line,	L"window,window-y,view-x,string,tab-width,encoding");
    toy_add_func(interp, L"curs-box",		func_curses_box,		L"window");
    toy_add_func(interp, L"curs-set-color",	func_curses_setcolor,		L"window,color-pair");
    toy_add_func(interp, L"curs-set-bgcolor",	func_curses_setbgcolor,		L"window,color-number");
    toy_add_func(interp, L"curs-set-overlay",	func_curses_setoverlay,		L"over-window,dest-window");
    toy_add_func(interp, L"curs-destroy-window",func_curses_destroywindow,	L"window");
    toy_add_func(interp, L"curs-move",		func_curses_move,		L"window,y,x");
    toy_add_func(interp, L"curs-add-color",	func_curses_add_color,		L"pair,fg-color,bg-color");
    toy_add_func(interp, L"curs-keyin",		func_curses_keyin,		L"window,timeout,encoding");
    
    return 0;
}

#endif /* NCURSES */
