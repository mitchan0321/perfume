#ifndef __FCLIB__
#define __FCLIB__

#define FCL_MAXNUMCHAR      0x30000
#define FCL_NUMGROUPS       10
#define FCL_GRPNUM          3
#define FCL_GRPBITMASK      0x07

#define FCL_SET(a,c,w)      a[c / FCL_NUMGROUPS] |= ((w & FCL_GRPBITMASK) << ((c % FCL_NUMGROUPS) * FCL_GRPNUM))
#define FCL_GET(a,c)        ((a[c / FCL_NUMGROUPS] >> ((c % FCL_NUMGROUPS) * FCL_GRPNUM)) & FCL_GRPBITMASK)

int fcl_read_cab_file(char *fname);
int fcl_reset();
int fcl_get_width(wchar_t uc);

#endif /* __FCLIB__ */
