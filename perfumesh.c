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
    Toy_Type *cmdl;
    char *dir = NULL;
    char **argp;
    int i;
    Cell *encc;
    encoder_error_info *ence;
    int init_sts;

    /* detect -D configuration argument */

    argp = argv;
    if ((argc >= 3) && (strcmp(argv[1], "-D") == 0)) {
        dir = argv[2];
        argc = argc - 2;
        argp = malloc(sizeof(char*) * (argc+1));
        argp[0] = argv[0];
        argp[argc] = 0;
        for (i=1 ; i<argc ; i++) {
            argp[i] = argv[i+2]; 
        }
    }

    interp = new_interp(L"main", STACKSIZE, NULL, argc, argp, envp, dir);

    init_sts = 0;
    INIT_ALLOC_HOOK(init_sts);
    if (0 != init_sts) {
        // mey be not enough memory occured.
        ALLOC_OUT("at perfumesh interpriter, memory alloc error occured, force retry interpriter.\n");
        ALLOC_OUT("Next, follow these steps to verify the problem:\n");
        ALLOC_OUT("  > show-stack            --- show stack frame level.\n");
        ALLOC_OUT("  > print [stack-trace]   --- show stack trace, where problem occured.\n");
        ALLOC_OUT("  > w :wide               --- show stack trace and variables.\n");
        ALLOC_OUT("  > dict-local : pairs    --- show local variables\n");
        ALLOC_OUT("\n\n");

        init_sts = 0;
        INIT_ALLOC_HOOK(init_sts);
        if (0 != init_sts) {
            // may be do not reach.
            ALLOC_OUT("at perfumesh interpriter, memory alloc error occured double fault, exit.\n");
            exit(1);
        }
        goto error_retry;
    }

    if ((argc >= 2) && (strcmp(argp[1], "-") != 0)) {
    
	/* batch mode */
        
        b = new_bulk();
	if (strcmp(argp[1], "-c") == 0) {
	    if (argc >= 3) {

		/* batch command execute */
		
                encc = decode_dirent(interp, argp[2], &ence);
                if (0 == encc) {
                    bulk_set_string(b, to_wchar(argp[2]));
                } {
                    bulk_set_string(b, cell_get_addr(encc));
                };
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
	    } else {
		fwprintf(stderr, L"no specified command.\n");
		exit(1);
	    }

	} else {
	    
	    /* batch file load and execute */

	    cmdl = new_list(new_symbol(L"load"));
            encc = decode_dirent(interp, argp[1], &ence);
            if (0 == encc) {
                list_append(cmdl, new_string_str(to_wchar(argp[1])));
            } {
                list_append(cmdl, new_string_cell(encc));
            };
	    any = toy_call(interp, cmdl);
	    if (GET_TAG(any) == EXCEPTION) {
		fwprintf(stdout, L"EXCEPTION: %ls\n", to_string(any));
		exit(1);
	    }
	    /* Do not print result at batch mode */
	}

	exit(0);
    }

error_retry:

    b = new_bulk();
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
