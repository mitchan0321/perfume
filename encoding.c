#include "encoding.h"

#define ENCODING_NAME_MAX	(1) 	// now ready encodings are RAW and UTF-8.
					// (index range is 0 to ENCODING_NAME_MAX)

static wchar_t *ENCODING_NAME_DEFS[] = {
    L"RAW",		// index: 0 ... RAW encoding (no encoding data stream)
    L"UTF-8",		// index: 1 ... UTF-8 encoding
    L"EUC-JP",		// index: 2 ... EUC-JP encoding (not yet)
    L"SJIS",		// index: 3 ... Shift-JIS encoding (not yet)
    L"ISO-2022-JP",	// index: 4 ... ISO-2022-JP(JIS) encoding (not yet)
};

wchar_t*
get_encoding_name(int enc_idx) {
    /* if return 0, enc_idex range over */
    if ((enc_idx < 0) ||
	(enc_idx > ENCODING_NAME_MAX)) return 0;

    return ENCODING_NAME_DEFS[enc_idx];
}

int
get_encoding_index(wchar_t *enc_name) {
    int i;
    for (i=0; i<= ENCODING_NAME_MAX; i++) {
	if (wcscmp(ENCODING_NAME_DEFS[i], enc_name) == 0) return i;
    }

    /* if return -1, invalid string enc_name */
    return -1;
}
