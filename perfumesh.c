/* $Id: toysh.c,v 1.14 2012/03/01 12:33:31 mit-sato Exp $ */

#include <stdio.h>
#include <string.h>
#include "toy.h"

#define BUFFSIZE	(4096)
#define COMMAND		"perfumesh"

int main(int argc, char **argv, char **envp) {
    char buff[BUFFSIZE];
    Toy_Type *any, *result;
    Toy_Type *script;
    Toy_Type *err;
    Bulk *b;
    Cell *c;
    Toy_Interp *interp;

    interp = new_interp("main", STACKSIZE, NULL, argc, argv, envp);
    b = new_bulk();

    if ((argc >= 2) && (strcmp(argv[1], "-") != 0)) {
	/* batch mode */

	if ((strcmp(argv[1], "-c") == 0) && (argc == 3)) {
	    bulk_set_string(b, argv[2]);
	} else {
	    if (0 == bulk_load_file(b, argv[1])) {
		fprintf(stderr, "file not open: %s\n", argv[1]);
		exit(1);
	    }
	}
	any = toy_parse_start(b);
	if (NULL == any) {
	    fprintf(stderr, "no memory\n");
	    exit(1);
	}
	switch (GET_TAG(any)) {
	case EXCEPTION:
	    err = any;
	    fprintf(stderr, "parse error: %s\n",
		    to_string(list_get_item(err->u.exception.msg_list)));
	    exit(0);

	case SCRIPT:
	    script = any;
	    result = toy_eval_script(interp, script);

	    if (GET_TAG(result) != EXCEPTION) {
		/* Do not print result at batch mode */
/*		
		fprintf(stdout, "result[%s]=> ", toy_get_type_str(result));
		fprintf(stdout, "%s\n", to_print(result));
*/
	    } else {
		fprintf(stdout, "EXCEPTION: %s\n", to_string(result));
	    }
	    exit(0);

	default:
	    fprintf(stderr, "parse error: type=%s\n", toy_get_type_str(any));
	    exit(1);
	}
    }

    /* interpriter mode */
    while (! feof(stdin)) {
    next_loop:
	fputs("> ", stderr);
	if (NULL == fgets(buff, BUFFSIZE, stdin)) break;
	buff[BUFFSIZE-1] = 0;

	if (buff[0] == '!') {
	    buff[strlen(buff)-1] = 0;
	    if (0 == bulk_load_file(b, &buff[1])) {
		fprintf(stderr, "file not open: %s\n", &buff[1]);
		continue;
	    }
	} else {
	    bulk_set_string(b, buff);
	}

	any = toy_parse_start(b);
	if (NULL == any) {
	    fprintf(stderr, "no memory\n");

	} else {
	    switch (GET_TAG(any)) {
	    case EXCEPTION:
		err = any;
		fprintf(stderr, "parse error: %s\n",
			to_string(list_get_item(err->u.exception.msg_list)));
		if (cell_eq_str(err->u.exception.code, TE_PARSEBADCHAR) == 0) break;
		if (buff[0] == '!') break;

		c = new_cell(buff);
		while (1) {
		    fputs("=> ", stderr);
		    if (NULL == fgets(buff, BUFFSIZE, stdin)) goto exit_loop;

		    buff[BUFFSIZE-1] = 0;

		    cell_add_str(c, buff);
		    bulk_set_string(b, cell_get_addr(c));
		    any = toy_parse_start(b);

		    if (GET_TAG(any) == EXCEPTION) {
			err = any;
			if (cell_eq_str(err->u.exception.code, TE_PARSEBADCHAR) == 0) {
			    fprintf(stderr, "parse error: %s\n",
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
		    char *p;
		    fprintf(stdout, "result[%s]=> ", toy_get_type_str(result));
		    p = to_print(result);
		    if (strlen(p) > 512) {
			fprintf(stdout, "%-.512s ...\n", p);
		    } else {
			fprintf(stdout, "%s\n", p);
		    }
		} else {
		    fprintf(stdout, "EXCEPTION: %s\n", to_string(result));
		}
		break;

	    default:
		fprintf(stderr, "parse error: type=%s\n", toy_get_type_str(any));
		
	    }
	}
    }
exit_loop:

    return 0;
}
