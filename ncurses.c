#ifdef NCURSES

#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <locale.h>
#define __USE_XOPEN
#include <wchar.h>

#include "toy.h"
#include "interp.h"
#include "types.h"
#include "cell.h"
#include "global.h"
#include "array.h"
#include "cstack.h"
#include "util.h"
#include "encoding.h"
#include "sys/time.h"

Toy_Type*
func_curses_init(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *wmain;
	
    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen > 0) goto error;

    setenv("ESCDELAY", "100", 1);
    // setlocale(LC_ALL, "");
    wmain = initscr();
    start_color();
    raw();
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
    int i, j;
    int y, x;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    container = list_get_item(posargs);
    if (GET_TAG(container) != CONTAINER) goto error;
    if (wcscmp(cell_get_addr(container->u.container.desc), L"CURSES") != 0) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-clear', Bad descriptor.", interp);
    }
    w = container->u.container.data;
    posargs = list_next(posargs);

    werase(w);
    getmaxyx(w, y, x);
    for (i=0; i<y; i++) {
	for (j=0; j<x; j++) {
	    wmove(w, i, j);
	    wprintw(w, " ");
	}
    }
    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-clear', syntax: curs-clear window", interp);
}


Toy_Type*
func_curses_wipe(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *w;
    Toy_Type *container;
    int i, j;
    int y, x;

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
    getmaxyx(w, y, x);
    for (i=0; i<y; i++) {
	for (j=0; j<x; j++) {
	    wmove(w, i, j);
	    wprintw(w, " ");
	}
    }
    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-wipe', syntax: curs-wipe window", interp);
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

static wchar_t *control_character_font [35] = {
    L"\u2400",  // NUL  0x00
    L"\u2401",  // SOH  0x01
    L"\u2402",  // STX  0x02
    L"\u2403",  // ETX  0x03
    L"\u2404",  // EOT  0x04
    L"\u2405",  // ENQ  0x05
    L"\u2406",  // ACK  0x06
    L"\u2407",  // BEL  0x07
    L"\u2408",  // BS   0x08
    L" ",       // HT   0x09
    L"\u240a",  // LF   0x0a
    L"\u240b",  // VT   0x0b
    L"\u240c",  // FF   0x0c
    L"\u240d",  // CR   0x0d
    L"\u240e",  // SO   0x0e
    L"\u240f",  // SI   0x0f
    L"\u2410",  // DLE  0x10
    L"\u2411",  // DC1  0x11
    L"\u2412",  // DC2  0x12
    L"\u2413",  // DC3  0x13
    L"\u2414",  // DC4  0x14
    L"\u2415",  // NAK  0x15
    L"\u2416",  // SYN  0x16
    L"\u2417",  // ETB  0x17
    L"\u2418",  // CAN  0x18
    L"\u2419",  // EM   0x19
    L"\u241a",  // SUB  0x1a
    L"\u241b",  // ESC  0x1b
    L"\u241c",  // FS   0x1c
    L"\u241d",  // GS   0x1d
    L"\u241e",  // RS   0x1e
    L"\u241f",  // US   0x1f
    L"\u2421",  // DEL  0x7f
    L"\ufffd",  // Unknown character (width = -1)
    L"\u2423",  // Combining character (width = 0)
};

Toy_Type*
func_curses_render_line(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *w;
    Toy_Type *container, *arg;
    int window_y, view_x, offset_x, tab_width;
    Toy_Type *disp_string, *encoding;
    int win_size_y, win_size_x;
    int iencoder;
    int i, slen;
    wchar_t *p, *cp;
    Render_Encode *rendaring_data;
    encoder_error_info *enc_error_info;
    Cell *codep;
    Cell *result;
	
    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 7) goto error;

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
    if (GET_TAG(arg) != INTEGER) goto error;
    offset_x = mpz_get_si(arg->u.biginteger);
    posargs = list_next(posargs);

    arg = list_get_item(posargs);
    if (GET_TAG(arg) != STRING) goto error;
    disp_string = arg;
    posargs = list_next(posargs);

    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    tab_width = mpz_get_si(arg->u.biginteger);
    if (tab_width <= 0) tab_width = 1;
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
    rendaring_data = GC_MALLOC(sizeof(Render_Encode) * (slen+1));
    ALLOC_SAFE(rendaring_data);
    enc_error_info = GC_MALLOC(sizeof(encoder_error_info));
    ALLOC_SAFE(enc_error_info);
    p = cell_get_addr(disp_string->u.string);
    for (i=0; i<slen; i++) {
	if ((p[i] < 0x20) || (p[i] == 0x7f)) {
	    /* control character encoding */
	    if (p[i] != 0x7f) {
		cp = control_character_font[p[i]];
		rendaring_data[i].display_width = 1;
	    } else {
		cp = control_character_font[32];
		rendaring_data[i].display_width = 1;
	    }
	    codep = new_cell(NULL);
	    cell_add_char(codep, cp[0]);
	    result = encode_unicode_to_raw(codep, iencoder, enc_error_info);
	    if (NULL == result) {
		return new_exception(TE_BADENCODEBYTE, enc_error_info->message, interp);
	    }
	    rendaring_data[i].display_char = cell_get_addr(result);
	} else if (p[i] < 0x7f) {
	    /* ASCII character encoding */
	    codep = new_cell(NULL);
	    cell_add_char(codep, p[i]);
	    rendaring_data[i].display_char = cell_get_addr(codep);
	    rendaring_data[i].display_width = 1;
        } else {
            int w;
            w = (int)wcwidth((wchar_t)p[i]);
            // if (w < 0) {
            if (w <= 0) {
                if (w < 0) {
                    cp = control_character_font[33];
                    rendaring_data[i].display_width = 1; // if u+fffd font width is 2, set to 2
                } else {
                    cp = control_character_font[34];
                    rendaring_data[i].display_width = 1;
                }
                codep = new_cell(NULL);
                cell_add_char(codep, cp[0]);
                result = encode_unicode_to_raw(codep, iencoder, enc_error_info);
                if (NULL == result) {
                    return new_exception(TE_BADENCODEBYTE, enc_error_info->message, interp);
                }
                rendaring_data[i].display_char = cell_get_addr(result);
            } else {
                if (w == 0) {
                    rendaring_data[i].display_width = 2;
                    codep = new_cell(NULL);
                    cell_add_char(codep, L' ');
                    cell_add_char(codep, p[i]);
                } else {
                    rendaring_data[i].display_width = w;
                    codep = new_cell(NULL);
                    cell_add_char(codep, p[i]);
                }
                result = encode_unicode_to_raw(codep, iencoder, enc_error_info);
                if (NULL == result) {
                    return new_exception(TE_BADENCODEBYTE, enc_error_info->message, interp);
                }
                rendaring_data[i].display_char = cell_get_addr(result);
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
    win_size_x -= offset_x;
    for (i=0; i<slen; i++) {
	if (((rendaring_data[i].display_position - view_x) >= 0) &&
	    ((rendaring_data[i].display_position - view_x + rendaring_data[i].display_width) <= (win_size_x))) {
	    wmove(w, window_y, rendaring_data[i].display_position - view_x + offset_x);
	    wprintw(w, "%s", to_char(rendaring_data[i].display_char));
	}
    }
    
    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-render-line', syntax: curs-render-line window window-y view-x offset-x string tab-width encoding", interp);
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
    container->u.container.data = NULL;
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
func_curses_get_cursor(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *w;
    Toy_Type *container;
    int y, x;
    Toy_Type *result;

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    container = list_get_item(posargs);
    if (GET_TAG(container) != CONTAINER) goto error;
    if (wcscmp(cell_get_addr(container->u.container.desc), L"CURSES") != 0) {
	return new_exception(TE_CURSES, L"Curses error at 'curs-move', Bad descriptor.", interp);
    }
    w = container->u.container.data;

    getyx(w, y, x);
    result = new_list(NULL);
    list_append(result, new_integer_si(y));
    list_append(result, new_integer_si(x));

    return result;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-get-cursor', syntax: curs-get-cursor window", interp);
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

static Toy_Type*
key_conv(int in) {
    wchar_t buff[32];
    static Toy_Type *ctrlkey_defs[32] = {NULL,NULL,};
    Cell *c;

    if (ctrlkey_defs[0] == NULL) {
	ctrlkey_defs[0]  = new_symbol(L"CTRL_SP");
	ctrlkey_defs[1]  = new_symbol(L"CTRL_A");
	ctrlkey_defs[2]  = new_symbol(L"CTRL_B");
	ctrlkey_defs[3]  = new_symbol(L"CTRL_C");
	ctrlkey_defs[4]  = new_symbol(L"CTRL_D");
	ctrlkey_defs[5]  = new_symbol(L"CTRL_E");
	ctrlkey_defs[6]  = new_symbol(L"CTRL_F");
	ctrlkey_defs[7]  = new_symbol(L"CTRL_G");
	ctrlkey_defs[8]  = new_symbol(L"CTRL_H");
	ctrlkey_defs[9]  = new_symbol(L"CTRL_I");
	ctrlkey_defs[10] = new_symbol(L"CTRL_J");
	ctrlkey_defs[11] = new_symbol(L"CTRL_K");
	ctrlkey_defs[12] = new_symbol(L"CTRL_L");
	ctrlkey_defs[13] = new_symbol(L"CTRL_M");
	ctrlkey_defs[14] = new_symbol(L"CTRL_N");
	ctrlkey_defs[15] = new_symbol(L"CTRL_O");
	ctrlkey_defs[16] = new_symbol(L"CTRL_P");
	ctrlkey_defs[17] = new_symbol(L"CTRL_Q");
	ctrlkey_defs[18] = new_symbol(L"CTRL_R");
	ctrlkey_defs[19] = new_symbol(L"CTRL_S");
	ctrlkey_defs[20] = new_symbol(L"CTRL_T");
	ctrlkey_defs[21] = new_symbol(L"CTRL_U");
	ctrlkey_defs[22] = new_symbol(L"CTRL_V");
	ctrlkey_defs[23] = new_symbol(L"CTRL_W");
	ctrlkey_defs[24] = new_symbol(L"CTRL_X");
	ctrlkey_defs[25] = new_symbol(L"CTRL_Y");
	ctrlkey_defs[26] = new_symbol(L"CTRL_Z");
	ctrlkey_defs[27] = new_symbol(L"KEY_ESC");
	ctrlkey_defs[28] = new_symbol(L"KEY_FS");
	ctrlkey_defs[29] = new_symbol(L"KEY_GS");
	ctrlkey_defs[30] = new_symbol(L"KEY_RS");
	ctrlkey_defs[31] = new_symbol(L"KEY_US");
    }
    
    if (in > 256) {
	// detect function key input
	if ((in >= KEY_F(0)) && (in <= KEY_F(63))) {
	    swprintf(buff, 32, L"KEY_F%d", in - KEY_F(0));	    
	} else {
	    swprintf(buff, 32, L"%s", keyname(in));
	}
	return new_symbol(buff);
    }

    if ((in >= 0) && (in < 0x20)) {
	return ctrlkey_defs[in];
    }

    c = new_cell(L"");
    cell_add_char(c, in);
    return new_string_cell(c);
}

static double
get_now_time() {
    struct timeval s;
    double r;

    gettimeofday(&s, NULL);
    r = (double)s.tv_sec + ((double)s.tv_usec)/1000000.0;
    return r;
}

Toy_Type*
func_curses_keyin(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    WINDOW *w;
    Toy_Type *container, *encoding;
    int itimeout, iencoder;
    int in, cur_in;
    Toy_Type *arg;
    Cell *incell, *dstr;
    Toy_Type *inlist, *result, *result_list;
    encoder_error_info *enc_error_info;
    static int pending_key = -1;
    static unsigned long int curs_blink = 0;
    static double time_prev=0.0, time_now=0.0, time_on_repeat=0.0;
    static int no_input_count = 0;
    int blink, blink_fact = 3;
    Toy_Type *tblink, *tblink_fact;

    blink = 0;
    tblink = hash_get_and_unset_t(nameargs, new_symbol(L"blink:"));
    if (NULL != tblink) {
        if (IS_NIL(tblink)) {
            blink = 0;
        } else {
            blink = 1;
        }
    }

    tblink_fact = hash_get_and_unset_t(nameargs, new_symbol(L"blink-fact:"));
    if (NULL != tblink_fact) {
        if (GET_TAG(tblink_fact) == INTEGER) {
            blink_fact = mpz_get_si(tblink_fact->u.biginteger);
        }
    }

    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 3) goto error;

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

    enc_error_info = GC_MALLOC(sizeof(encoder_error_info));
    ALLOC_SAFE(enc_error_info);
    incell = new_cell(L"");
    inlist = result = new_list(NULL);

    curs_blink ++;
    switch (blink) {
    case 0:
        curs_set(1); // always cursor on
        break;
    case 1:
        // curs_set(((curs_blink >> 4) % 2) ? 0 : 1); // blink even
        curs_set(((curs_blink >> blink_fact) % 4) ? 1 : 0); // blink 3:1
        break;
    default:
        curs_set(1); // always cursor on
    };
    
    wtimeout(w, itimeout);

    cur_in = -1;
    if (pending_key != -1) {
	in = pending_key;
	pending_key = -1;
    } else {
	in = wgetch(w);
    }
    if (in == -1) {
	goto valid_return;
    }
    cur_in = in;
    
    //
    // detect function character.
    //
    if (in >= 256 || in == 27) {
	//
	// KEY_RESIZE onece occured.
	//
	if (in == KEY_RESIZE) {
	    wtimeout(w, 500);
	    in = wgetch(w);
	    while (in == KEY_RESIZE) {
		in = wgetch(w);
	    }
	    if (in == -1) {
		pending_key = -1;
	    } else {
		pending_key = in;
	    }
	    inlist = list_append(inlist, key_conv(KEY_RESIZE));
            cur_in = -1;
	    
            goto valid_return;
	}
	
	// if function key, return to caller soon.
	inlist = list_append(inlist, key_conv(in));
        
	wtimeout(w, itimeout / 4);
	in = wgetch(w);
	while (in != ERR) {
	    if (in >= 256) {
		pending_key = in;
                goto valid_return;
    	    }
	    if (in < 0x20) {
		pending_key = in;
		break;
	    }
	    cell_add_char(incell, in);
	    cur_in = in;
	    in = wgetch(w);
	}
	if (cell_get_length(incell) > 0) {
	    inlist = list_append(inlist, new_string_cell(incell));
	}
        
        goto valid_return;
	
    } else {
	//
	// detect character input
	//
	if ((in >= 0) && (in < 0x20)) {
	    // detect control caracter, return to caller soon.
	    inlist = list_append(inlist, key_conv(in));
	    cur_in = in;
            goto valid_return;
	}
	
	// assemble sequential character string.
	cell_add_char(incell, in);
	cur_in = in;
	
	// read remain character
	wtimeout(w, itimeout / 4);
	in = wgetch(w);
	while (in != -1) {
	    if (((in >= 0) && (in < 0x20)) || (in >= 256))  {
		pending_key = in;
		in = -1;
		break;
	    } {
		cell_add_char(incell, in);
	        cur_in = in;
	    }
	    in = wgetch(w);
	}
	// decode encoding
	dstr = decode_raw_to_unicode(incell, iencoder, enc_error_info);
	if (NULL == dstr) {
	    return new_exception(TE_BADENCODEBYTE, enc_error_info->message, interp);
	}
	inlist = list_append(inlist, new_string_cell(dstr));
    }

valid_return:
    if (cur_in != -1) {
        time_prev = time_now;
        time_now = get_now_time();
        if ((time_now - time_prev) < 0.1) {
            time_on_repeat += (time_now - time_prev);
        } else {
            time_on_repeat = 0.0;
        }
        no_input_count = 0;
    } else {
	no_input_count ++;
	if (no_input_count >= 3) {
            time_on_repeat = 0.0;
	}
    }
    
    result_list = new_list(NULL);
    list_append(result_list, new_real(time_on_repeat));
    list_append(result_list, result);
    
    return result_list;
    
error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-keyin', syntax: curs-keyin window timeout encoding [:blink] [blink-fact: n]", interp);
}

Toy_Type*
func_curses_pos_to_index(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg, *disp_string;
    int i;
    int pos;
    int tab_width;
    int slen;
    Render_Encode *rendaring_data;
    wchar_t *p;
    
    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 3) goto error;

    arg = list_get_item(posargs);
    if (GET_TAG(arg) != STRING) goto error;
    disp_string = arg;
    posargs = list_next(posargs);
    
    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    pos = mpz_get_si(arg->u.biginteger);
    posargs = list_next(posargs);

    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    tab_width = mpz_get_si(arg->u.biginteger);
    if (tab_width <= 0) tab_width = 1;
    posargs = list_next(posargs);

    slen = cell_get_length(disp_string->u.string);
    if (slen <= 0) {
	return new_integer_si(0);
    }
    slen ++;
    rendaring_data = GC_MALLOC(sizeof(Render_Encode) * (slen+1));
    ALLOC_SAFE(rendaring_data);

    p = GC_MALLOC((sizeof(wchar_t)) * slen);
    ALLOC_SAFE(p);
    wcsncpy(p, cell_get_addr(disp_string->u.string), slen);
    p[slen-1] = -1;

    for (i=0; i<slen; i++) {
	if (p[i] == -1) {
	    /* string last character (dummy data) */
	    rendaring_data[i].display_width = 1;
	}
	else if ((p[i] < 0x20) || (p[i] == 0x7f)) {
	    /* control character encoding */
	    rendaring_data[i].display_width = 1;
	}
	else if (p[i] < 0x7f) {
	    /* ASCII character encoding */
	    rendaring_data[i].display_width = 1;
        } else {
            int w;
            w = (int)wcwidth((wchar_t)p[i]);
            // if (w < 0) {
            if (w <= 0) {
                if (w < 0) {
                    rendaring_data[i].display_width = 1;  // if u+fffd font width is 2, set to 2
                } else {
                    rendaring_data[i].display_width = 1;
                }
            } else {
                if (w == 0) {
                    rendaring_data[i].display_width = 2;
                } else {
                    rendaring_data[i].display_width = w;
                }
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
    for (i=slen-1; i>=0; i--) {
	if (rendaring_data[i].display_position <= pos) {
	    return new_integer_si(i);
	}
    }
    return new_integer_si(0);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-pos-to-index', syntax: curs-pos-to-index string pos tab-width", interp);
}

Toy_Type*
func_curses_index_to_pos(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *arg, *disp_string;
    int i;
    int index;
    int tab_width;
    int slen;
    Render_Encode *rendaring_data;
    wchar_t *p;
    
    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 3) goto error;

    arg = list_get_item(posargs);
    if (GET_TAG(arg) != STRING) goto error;
    disp_string = arg;
    posargs = list_next(posargs);
    
    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    index = mpz_get_si(arg->u.biginteger);
    posargs = list_next(posargs);

    arg = list_get_item(posargs);
    if (GET_TAG(arg) != INTEGER) goto error;
    tab_width = mpz_get_si(arg->u.biginteger);
    if (tab_width <= 0) tab_width = 1;
    posargs = list_next(posargs);

    slen = cell_get_length(disp_string->u.string);
    if (slen <= 0) {
	return new_integer_si(0);
    }
    slen ++;
    rendaring_data = GC_MALLOC(sizeof(Render_Encode) * (slen+1));
    ALLOC_SAFE(rendaring_data);

    p = GC_MALLOC((sizeof(wchar_t)) * slen);
    ALLOC_SAFE(p);
    wcsncpy(p, cell_get_addr(disp_string->u.string), slen);
    p[slen-1] = -1;

    for (i=0; i<slen; i++) {
	if (p[i] == -1) {
	    /* string last character (dummy data) */
	    rendaring_data[i].display_width = 1;
	}
	else if ((p[i] < 0x20) || (p[i] == 0x7f)) {
	    /* control character encoding */
	    rendaring_data[i].display_width = 1;
	}
	else if (p[i] < 0x7f) {
	    /* ASCII character encoding */
	    rendaring_data[i].display_width = 1;
        } else {
            int w;
            w = (int)wcwidth((wchar_t)p[i]);
            // if (w < 0) {
            if (w <= 0) {
                if (w < 0) {
                    rendaring_data[i].display_width = 1;  // if u+fffd font width is 2, set to 2
                } else {
                    rendaring_data[i].display_width = 1;
                }
            } else {
                if (w == 0) {
                    rendaring_data[i].display_width = 2;
                } else {
                    rendaring_data[i].display_width = w;
                }
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
    if (slen == 0) {
	return new_integer_si(0);
    }
    if (index < 0) {
	return new_integer_si(0);
    }
    if (index >= slen) {
	return new_integer_si(rendaring_data[slen-1].display_position);
    }
    return new_integer_si(rendaring_data[index].display_position);

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-index-to-pos', syntax: curs-index-to-pos string index tab-width", interp);
}

Toy_Type*
func_curses_flash(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 0) goto error;

    flash();
    beep();

    return const_T;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-flash', syntax: curs-flash", interp);
}

Toy_Type*
func_curses_col(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *message;
    wchar_t *p, *cmessage;
    int i;
    
    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    message = list_get_item(posargs);
    if (GET_TAG(message) != STRING) goto error;
    
    p = GC_MALLOC(sizeof(wchar_t) * (cell_get_length(message->u.string) + 1));
    ALLOC_SAFE(p);
    cmessage = cell_get_addr(message->u.string);
    
    i = 0;
    while (*cmessage) {
        switch (*cmessage) {
        case 0x8:
            i --;
            if (i < 0) {i = 0;}
            break;
        case 0x2010:
        case 0x2014:
            p[i] = L'-';
            i ++;
            break;
        case 0x2500:
            p[i] = L'-';
            i ++;
            break;
        case 0x2502:
            p[i] = L'|';
            i ++;
            break;
        case 0x250c:
        case 0x252c:
        case 0x2510:
        case 0x251c:
        case 0x253c:
        case 0x2524:
        case 0x2514:
        case 0x2534:
        case 0x2518:
            p[i] = L'+';
            i ++;
            break;
        case 0x201c:
        case 0x201d:
            p[i] = L'\"';
            i ++;
            break;
        case 0x2018:
        case 0x2019:
            p[i] = L'\'';
            i ++;
            break;
        case 0x27e8:
        case 0x27ea:
            p[i] = L'<';
            i ++;
            break;
        case 0x27e9:
        case 0x27eb:
            p[i] = L'>';
            i ++;
            break;
        default:
            p[i] = *cmessage;
            i ++;
            break;
        }
        cmessage ++;
    }
    p[i] = 0;
    
    return new_string_cell(new_cell(p));

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-col', syntax: curs-col string", interp);
}

Toy_Type*
func_curses_set(Toy_Interp *interp, Toy_Type *posargs, Hash *nameargs, int arglen) {
    Toy_Type *ival;
    int i;
    
    if (hash_get_length(nameargs) > 0) goto error;
    if (arglen != 1) goto error;

    ival = list_get_item(posargs);
    if (GET_TAG(ival) != INTEGER) goto error;
    i = mpz_get_si(ival->u.biginteger);
    curs_set(i);
    
    return ival;

error:
    return new_exception(TE_SYNTAX, L"Syntax error at 'curs-set', syntax: curs-set 0 | 1", interp);
}

int
toy_add_func_ncurses(Toy_Interp* interp) {
    toy_add_func(interp, L"curs-init",		func_curses_init,		NULL);
    toy_add_func(interp, L"curs-get-screen-size",func_curses_getscreensize,	L"window");
    toy_add_func(interp, L"curs-terminate",	func_curses_terminate,		NULL);
    toy_add_func(interp, L"curs-create-window",	func_curses_createwindow,	L"window,y,x,line,column");
    toy_add_func(interp, L"curs-new-window",	func_curses_newwindow,		L"window,y,x,line,column");
    toy_add_func(interp, L"curs-clear",		func_curses_clear,		L"window");
    toy_add_func(interp, L"curs-wipe",		func_curses_wipe,		L"window");
    toy_add_func(interp, L"curs-print",		func_curses_print,		L"window,message,encoding,y,x");
    toy_add_func(interp, L"curs-refresh",	func_curses_refresh,		L"window");
    toy_add_func(interp, L"curs-color",		func_curses_color,		L"window,y,x,len,color,attr");
    toy_add_func(interp, L"curs-render-line",	func_curses_render_line,	L"window,window-y,view-x,offset-x,string,tab-width,encoding");
    toy_add_func(interp, L"curs-box",		func_curses_box,		L"window");
    toy_add_func(interp, L"curs-set-color",	func_curses_setcolor,		L"window,color-pair");
    toy_add_func(interp, L"curs-set-bgcolor",	func_curses_setbgcolor,		L"window,color-number");
    toy_add_func(interp, L"curs-set-overlay",	func_curses_setoverlay,		L"over-window,dest-window");
    toy_add_func(interp, L"curs-destroy-window",func_curses_destroywindow,	L"window");
    toy_add_func(interp, L"curs-move",		func_curses_move,		L"window,y,x");
    toy_add_func(interp, L"curs-get-cursor",	func_curses_get_cursor,		L"window");
    toy_add_func(interp, L"curs-add-color",	func_curses_add_color,		L"pair,fg-color,bg-color");
    toy_add_func(interp, L"curs-keyin",		func_curses_keyin,		L"window,timeout,encoding");
    toy_add_func(interp, L"curs-pos-to-index",	func_curses_pos_to_index,	L"string,pos,tab-width");
    toy_add_func(interp, L"curs-index-to-pos",	func_curses_index_to_pos,	L"string,index,tab-width");
    toy_add_func(interp, L"curs-flash",		func_curses_flash,		NULL);    
    toy_add_func(interp, L"curs-col",		func_curses_col,		L"string");    
    toy_add_func(interp, L"curs-set",		func_curses_set,		L"integer");    
    return 0;
}

#endif /* NCURSES */
