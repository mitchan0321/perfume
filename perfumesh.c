#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <util.h>
#include "toy.h"
#include "encoding.h"

#define BUFFSIZE	(4096)
#define COMMAND		"perfumesh"

int main(int argc, char **argv, char **envp) {
    char buff[BUFFSIZE*sizeof(char)];
    Toy_Type *any, *result;
    Toy_Type *script;
    Toy_Type *err;
    Bulk *b;
    Cell *c;
    Toy_Interp *interp;

    interp = new_interp(L"main", STACKSIZE, NULL, argc, argv, envp);
    b = new_bulk();

    if ((argc >= 2) && (strcmp(argv[1], "-") != 0)) {
	/* batch mode */

	if (strcmp(argv[1], "-c") == 0) {
	    if (argc == 3) {
		bulk_set_string(b, to_wchar(argv[2]));
	    } else {
		fwprintf(stderr, L"no specified command.\n");
		exit(1);
	    }
	} else {
	    if (0 == bulk_load_file(b, argv[1], NENCODE_UTF8)) {
		fwprintf(stderr, L"file not open \"%ls\".\n", to_wchar(argv[1]));
		exit(1);
	    }
	}
	any = toy_parse_start(b);
	if (NULL == any) {
	    fwprintf(stderr, L"no memory\n");
	    exit(1);
	}
	switch (GET_TAG(any)) {
	case EXCEPTION:
	    err = any;
	    fwprintf(stderr, L"parse error: %ls\n",
		     to_string(list_get_item(err->u.exception.msg_list)));
	    exit(0);

	case SCRIPT:
	    script = any;
	    result = toy_eval_script(interp, script);

	    if (GET_TAG(result) != EXCEPTION) {
		/* Do not print result at batch mode */

	    } else {
		fwprintf(stdout, L"EXCEPTION: %ls\n", to_string(result));
	    }
	    exit(0);

	default:
	    fwprintf(stderr, L"parse error: type=%ls\n", toy_get_type_str(any));
	    exit(1);
	}
    }

    /* interpriter mode */
    while (! feof(stdin)) {
    next_loop:
	fputws(L"> ", stderr);
	fflush(stderr);
	if (NULL == fgets(buff, BUFFSIZE, stdin)) break;
	buff[BUFFSIZE-1] = 0;

	if (buff[0] == '!') {
	    buff[strlen(buff)-1] = 0;
	    if (0 == bulk_load_file(b, &buff[1], NENCODE_UTF8)) {
		fwprintf(stderr, L"file not open: %ls\n", to_wchar(&buff[1]));
		continue;
	    }
	} else {
	    bulk_set_string(b, to_wchar(buff));
	}

	any = toy_parse_start(b);
	if (NULL == any) {
	    fwprintf(stderr, L"no memory\n");

	} else {
	    switch (GET_TAG(any)) {
	    case EXCEPTION:
		err = any;
		fwprintf(stderr, L"parse error: %ls\n",
			 to_string(list_get_item(err->u.exception.msg_list)));
		if (cell_eq_str(err->u.exception.code, TE_PARSEBADCHAR) == 0) break;
		if (buff[0] == '!') break;

		c = new_cell(to_wchar(buff));
		while (1) {
		    fputws(L"=> ", stderr);
		    fflush(stderr);
		    if (NULL == fgets(buff, BUFFSIZE, stdin)) goto exit_loop;

		    buff[BUFFSIZE-1] = 0;

		    cell_add_str(c, to_wchar(buff));
		    bulk_set_string(b, cell_get_addr(c));
		    any = toy_parse_start(b);

		    if (GET_TAG(any) == EXCEPTION) {
			err = any;
			if (cell_eq_str(err->u.exception.code, TE_PARSEBADCHAR) == 0) {
			    fwprintf(stderr, L"parse error: %ls\n",
				     to_string(list_get_item(err->u.exception.msg_list)));
			    goto next_loop;
			}
		    } else {
			break;
		    }
		}

	    case SCRIPT:
		script = any;
		result = toy_eval_script(interp, script);

		if (GET_TAG(result) != EXCEPTION) {
		    wchar_t *p;
		    fwprintf(stdout, L"result[%ls]=> ", toy_get_type_str(result));
		    p = to_print(result);
		    if (wcslen(p) > 512) {
			fwprintf(stdout, L"%-.512ls ...\n", p);
		    } else {
			fwprintf(stdout, L"%ls\n", p);
		    }
		} else {
		    fwprintf(stdout, L"EXCEPTION: %ls\n", to_string(result));
		}
		break;

	    default:
		fwprintf(stderr, L"parse error: type=%ls\n", toy_get_type_str(any));
		
	    }
	}
    }
exit_loop:

    return 0;
}
