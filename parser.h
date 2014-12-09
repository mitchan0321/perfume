/* $Id: parser.h,v 1.3 2009/05/10 10:44:03 mit-sato Exp $ */

#ifndef __PARSER__
#define __PARSER__

#include <t_gc.h>
#include "types.h"
#include "cell.h"
#include "bulk.h"

#define ENDCHAR	(" \t\r\n(){}[]#;\"\',")

Toy_Type* toy_parse_start(Bulk *src);
Toy_Type* toy_parse_script(Bulk *src, char endc);
Toy_Type* toy_parse_statement(Bulk *src, char endc);
Toy_Type* toy_parse_symbol(Bulk *src, char *endc);
Toy_Type* toy_parse_string(Bulk *src, char endc);
Toy_Type* toy_parse_rquote(Bulk *src, char endc);
Toy_Type* toy_parse_list(Bulk *src, char endc);
Toy_Type* toy_parse_block(Bulk *src, char endc);
Toy_Type* toy_parse_eval(Bulk *src, char endc);
void parser_debug_dump(Toy_Type *script);

#endif /* __PARSER__ */
