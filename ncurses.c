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

    setlocale(LC_ALL, "");
    wmain = initscr();
    start_color();
    /* ad-hoc definition for color */
    init_pair( 1, COLOR_BLACK, 	COLOR_BLACK);
    init_pair( 2, COLOR_RED, 	COLOR_BLACK);
    init_pair( 3, COLOR_GREEN, 	COLOR_BLACK);
    init_pair( 4, COLOR_YELLOW, COLOR_BLACK);
    init_pair( 5, COLOR_BLUE, 	COLOR_BLACK);
    init_pair( 6, COLOR_MAGENTA,COLOR_BLACK);
    init_pair( 7, COLOR_CYAN,	COLOR_BLACK);
    init_pair( 8, COLOR_WHITE,	COLOR_BLACK);
    init_pair( 9, COLOR_BLACK, 	COLOR_RED);
    init_pair(10, COLOR_RED, 	COLOR_RED);
    init_pair(11, COLOR_GREEN, 	COLOR_RED);
    init_pair(12, COLOR_YELLOW, COLOR_RED);
    init_pair(13, COLOR_BLUE, 	COLOR_RED);
    init_pair(14, COLOR_MAGENTA,COLOR_RED);
    init_pair(15, COLOR_CYAN,	COLOR_RED);
    init_pair(16, COLOR_WHITE,	COLOR_RED);
    init_pair(17, COLOR_BLACK, 	COLOR_GREEN);
    init_pair(18, COLOR_RED, 	COLOR_GREEN);
    init_pair(19, COLOR_GREEN, 	COLOR_GREEN);
    init_pair(20, COLOR_YELLOW, COLOR_GREEN);
    init_pair(21, COLOR_BLUE, 	COLOR_GREEN);
    init_pair(22, COLOR_MAGENTA,COLOR_GREEN);
    init_pair(23, COLOR_CYAN,	COLOR_GREEN);
    init_pair(24, COLOR_WHITE,	COLOR_GREEN);
    init_pair(25, COLOR_BLACK, 	COLOR_YELLOW);
    init_pair(26, COLOR_RED, 	COLOR_YELLOW);
    init_pair(27, COLOR_GREEN, 	COLOR_YELLOW);
    init_pair(28, COLOR_YELLOW, COLOR_YELLOW);
    init_pair(29, COLOR_BLUE, 	COLOR_YELLOW);
    init_pair(30, COLOR_MAGENTA,COLOR_YELLOW);
    init_pair(31, COLOR_CYAN,	COLOR_YELLOW);
    init_pair(32, COLOR_WHITE,	COLOR_YELLOW);
    init_pair(33, COLOR_BLACK, 	COLOR_BLUE);
    init_pair(34, COLOR_RED, 	COLOR_BLUE);
    init_pair(35, COLOR_GREEN, 	COLOR_BLUE);
    init_pair(36, COLOR_YELLOW, COLOR_BLUE);
    init_pair(37, COLOR_BLUE, 	COLOR_BLUE);
    init_pair(38, COLOR_MAGENTA,COLOR_BLUE);
    init_pair(39, COLOR_CYAN,	COLOR_BLUE);
    init_pair(40, COLOR_WHITE,	COLOR_BLUE);
    init_pair(41, COLOR_BLACK, 	COLOR_MAGENTA);
    init_pair(42, COLOR_RED, 	COLOR_MAGENTA);
    init_pair(43, COLOR_GREEN, 	COLOR_MAGENTA);
    init_pair(44, COLOR_YELLOW, COLOR_MAGENTA);
    init_pair(45, COLOR_BLUE, 	COLOR_MAGENTA);
    init_pair(46, COLOR_MAGENTA,COLOR_MAGENTA);
    init_pair(47, COLOR_CYAN,	COLOR_MAGENTA);
    init_pair(48, COLOR_WHITE,	COLOR_MAGENTA);
    init_pair(49, COLOR_BLACK, 	COLOR_CYAN);
    init_pair(50, COLOR_RED, 	COLOR_CYAN);
    init_pair(51, COLOR_GREEN, 	COLOR_CYAN);
    init_pair(52, COLOR_YELLOW, COLOR_CYAN);
    init_pair(53, COLOR_BLUE, 	COLOR_CYAN);
    init_pair(54, COLOR_MAGENTA,COLOR_CYAN);
    init_pair(55, COLOR_CYAN,	COLOR_CYAN);
    init_pair(56, COLOR_WHITE,	COLOR_CYAN);
    init_pair(57, COLOR_BLACK, 	COLOR_WHITE);
    init_pair(58, COLOR_RED, 	COLOR_WHITE);
    init_pair(59, COLOR_GREEN, 	COLOR_WHITE);
    init_pair(60, COLOR_YELLOW, COLOR_WHITE);
    init_pair(61, COLOR_BLUE, 	COLOR_WHITE);
    init_pair(62, COLOR_MAGENTA,COLOR_WHITE);
    init_pair(63, COLOR_CYAN,	COLOR_WHITE);
    init_pair(64, COLOR_WHITE,	COLOR_WHITE);
    /* enf of color definition */
    
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
    int y, x, nline, ncol;
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

    wsub = subwin(w, nline, ncol, y, x);
    if (NULL == wsub) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-create-window', May be bad parameter.", interp);
    }

    return new_container(wsub, L"CURSES");

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-create-window', syntax: curs-create-window window y x lines columns", interp);
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
    Toy_Type *message;
    int iencoder;
    Cell *c;
    encoder_error_info *enc_error_info;

    if (hash_get_length(nameargs) > 0) goto error;
    if ((arglen != 2) && (arglen != 4)) goto error;

    container = list_get_item(posargs);
    if (GET_TAG(container) != CONTAINER) goto error;
    if (wcscmp(cell_get_addr(container->u.container.desc), L"CURSES") != 0) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-print', Bad descriptor.", interp);
    }
    w = container->u.container.data;
    posargs = list_next(posargs);

    message = list_get_item(posargs);
    if (STRING != GET_TAG(message)) goto error;
    
    if (arglen == 4) {
	posargs = list_next(posargs);
	arg = list_get_item(posargs);
	if (GET_TAG(arg) != INTEGER) goto error;
	y = mpz_get_si(arg->u.biginteger);

	posargs = list_next(posargs);
	arg = list_get_item(posargs);
	if (GET_TAG(arg) != INTEGER) goto error;
	x = mpz_get_si(arg->u.biginteger);

	wmove(w, y, x);
    }

    iencoder = get_encoding_index(L"UTF-8");
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
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-print', syntax: curs-print window message [ y x ]", interp);
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

int
toy_add_func_ncurses(Toy_Interp* interp) {
    toy_add_func(interp, L"curs-init",		func_curses_init,		NULL);
    toy_add_func(interp, L"curs-get-screen-size",func_curses_getscreensize,	L"window");
    toy_add_func(interp, L"curs-terminate",	func_curses_terminate,		NULL);
    toy_add_func(interp, L"curs-create-window",	func_curses_createwindow,	L"window,y,x,line,column");
    toy_add_func(interp, L"curs-clear",		func_curses_clear,		L"window");
    toy_add_func(interp, L"curs-print",		func_curses_print,		L"window,message,y,x");
    toy_add_func(interp, L"curs-refresh",	func_curses_refresh,		L"window");
    toy_add_func(interp, L"curs-color",		func_curses_color,		L"window,y,x,len,color,attr");
    toy_add_func(interp, L"curs-render-line",	func_curses_render_line,	L"window,window-y,view-x,string,tab-width,encoding");

    return 0;
}

#endif /* NCURSES */
