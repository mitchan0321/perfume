#ifndef __FCLIB__
#define __FCLIB__

#define FCL_MAXNUMCHAR      0x30000
#define FCL_SET(a,c,w)      a[c / 16] |= ((w & 0x3) << ((c % 16) * 2))
#define FCL_GET(a,c)        ((a[c / 16] >> ((c % 16) * 2)) & 0x3)

int fcl_read_cab_file(char *fname);
int fcl_reset();
int fcl_get_width(wchar_t uc);

#endif /* __FCLIB__ */
