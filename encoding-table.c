#include <wchar.h>
#include <string.h>
#include <toy.h>
#include "t_gc.h"

wchar_t Unicode_to_JIS0208[65536];
wchar_t JIS0208_to_Unicode[65536];
int jisencoder_setup_done = 0;

void
JisEncoder_Setup() {
    if (jisencoder_setup_done == 0) {
	memset(Unicode_to_JIS0208, 0, sizeof(Unicode_to_JIS0208));
	memset(JIS0208_to_Unicode, 0, sizeof(JIS0208_to_Unicode));
#   	include "encoding-set-utoj.h"
#   	include "encoding-set-jtou.h"
    }
    
    jisencoder_setup_done = 1;
}
