#ifndef __UTIL__
#define __UTIL__

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <t_gc.h>
#include "toy.h"
#include "types.h"
#include "encoding.h"

int      read_size(int fd, char* buff, int size);
int      write_size(int fd, char* buff, int size);
wchar_t *to_wchar(const char *src);
char    *to_char(const wchar_t *src);
void     println(Toy_Interp *interp, wchar_t *msg);
char    *encode_dirent(Toy_Interp *interp, wchar_t *name, encoder_error_info **info);
Cell    *decode_dirent(Toy_Interp *interp, char *name, encoder_error_info **info);
wchar_t *decode_error(Toy_Interp *interp, char *str);
int      is_read_ready(int fd, int timeout);
int      is_write_ready(int fd, int timeout);

/* is_read_ready/is_write_ready return status */
#define IRDY_NO     1
#define IRDY_OK     2
#define IRDY_TOUT   3
#define IRDY_ERR    -1

#endif /* __UTIL__ */
