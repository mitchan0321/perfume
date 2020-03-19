#ifndef __PARSER__
#define __PARSER__

#include <wchar.h>
#include <t_gc.h>
#include "types.h"
#include "cell.h"
#include "bulk.h"

#define ENDCHAR	(L" \t\r\n(){}[]#;\"\',")

Toy_Type* toy_parse_start(Bulk *src);
Toy_Type* toy_parse_script(Bulk *src, wchar_t endc);
Toy_Type* toy_parse_statement(Bulk *src, wchar_t endc);
Toy_Type* toy_parse_symbol(Bulk *src, wchar_t *endc);
Toy_Type* toy_parse_string(Bulk *src, wchar_t endc);
Toy_Type* toy_parse_rquote(Bulk *src, wchar_t endc);
Toy_Type* toy_parse_list(Bulk *src, wchar_t endc);
Toy_Type* toy_parse_block(Bulk *src, wchar_t endc);
Toy_Type* toy_parse_eval(Bulk *src, wchar_t endc);
void parser_debug_dump(Toy_Type *script);

#endif /* __PARSER__ */
