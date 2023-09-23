#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#define __USE_XOPEN
#include <wchar.h>
#include "fclib.h"

/* if true fclib is initialized */
static int _fclib_init = 0;

/* font width bit map table */
static uint32_t _fbmap[FCL_MAXNUMCHAR/FCL_NUMGROUPS];

#define BUFFSIZE        256

int
fcl_read_cab_file(char *fname) {
    FILE *fp;
    char buff[BUFFSIZE];
    int i;
    int uc;
    int w;
    
    if (_fclib_init) return 1;  /* already initialized */
    
    /* clear bitmap */
    for (i=0; i<FCL_MAXNUMCHAR/FCL_NUMGROUPS; i++) {
        _fbmap[i] = 0;
    }

    /* padding 0x00 - 0x1f character width to 1 */
    for (i=0; i<0x20; i++) {
        FCL_SET(_fbmap, i, 1);
    }
    
    fp = fopen(fname, "r");
    if (NULL == fp) return 0;
    
    /* start at character 0x20 */
    uc = 0x20;
    while (NULL != fgets(buff, BUFFSIZE, fp)) {
        switch (buff[0]) {
        case 0: case '\n': case '\r':
            w = 0;
            break;
        case '0':
            w = 0;
            break;
        case '1':
            w = 1;
            break;
        case '2':
            w = 2;
            break;
        case '3':
            w = 3;
            break;
        case '4':
            w = 4;
            break;
        case '5':
            w = 5;
            break;
        case '6':
            w = 6;
            break;
        case '7':
            w = 7;
            break;
        default:
            w = 1;
        }
        FCL_SET(_fbmap, uc, w);
        uc ++;
    }

    fclose(fp);
    _fclib_init = 1;
    return 1;
}

int
fcl_reset(char *fname) {
    _fclib_init = 0;
    return 1;
}

int
fcl_get_width(wchar_t uc) {
    int w, lw;

    w = wcwidth(uc);
    
    if (0 == _fclib_init) return w;
    if (uc >= FCL_MAXNUMCHAR) return w;
    if (w == -1) return w;
    
    lw = FCL_GET(_fbmap, uc);
    if (lw == 0) {
        if (w == 1) {
            // U+1242b hack
            return FCL_GRPBITMASK + 2;
        }
    }
    if (w == 0) return 0;
    return lw;
}
