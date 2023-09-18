#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>

int get_char_width(unsigned int i, unsigned char *buff);
int to_utf8(unsigned int c, unsigned char *utf8s);

int main(int argc, char **argv) {
    unsigned int i;
    unsigned char buff[5];
    int w;
    struct termios tm, tm_save;
    int c;
    FILE *fp;
    
    fp = fopen("rename-to-your-font-name.fcab","w");
    if (fp == NULL) {
        fprintf(stderr, "can't open calibration file.\n");
        return 1;
    }

#ifdef __FreeBSD__
    tcgetattr(0, &tm);
#else
    ioctl(0, TCGETS, &tm);
#endif
    tm_save = tm;
    cfmakeraw(&tm);
#ifdef __FreeBSD__
    tcsetattr(0, TCSANOW, &tm);
#else
    ioctl(0, TCSETS, &tm);
#endif

    fprintf(stdout, "\e[H\e[2J"); /* cursor home and clear */
    fflush(stdout);
    
    for (i=0; i<0x20; i++) {
        fprintf(fp, "%d\n", 1);
    }
    
    for (i=0x20; i<=0x2ffff; i++) {
        w = get_char_width(i, buff);
        if (-1 == w) break;
        fprintf(stdout, "\t%06x\t%s:\t%d\n", i, buff, w);
        fprintf(fp, "%d\n", w);
    }
    
#ifdef __FreeBSD__
    tcsetattr(0, TCSANOW, &tm_save);
#else
    ioctl(0, TCSETS, &tm_save);
#endif
    
    fprintf(stdout, "\n");
    fclose(fp);

    return 0;
}

int
get_char_width(unsigned int c, unsigned char *buff) {
    int s;
    char r, l;
    int acc_y=0, acc_x=0;
    
    s = to_utf8(c, buff);
    if (0 == s) return 0;
    
    fprintf(stdout, "\e[H");    /* cursor home */
    fflush(stdout);
    fprintf(stdout, "%s", buff);
    fflush(stdout);
    fprintf(stdout, "\e[6n");   /* cursor position report request -> return "u+001bu+005by;xR" */
    fflush(stdout);
    l = read(0, &r, 1);
    if (-1 == l) return -1;
    if (r != '\e') return -1;
    l = read(0, &r, 1);
    if (-1 == l) return -1;
    if (r != '[') return -1;
    l = read(0, &r, 1);
    if (-1 == l) return -1;
    while (r != ';') {
        acc_y *= 10;
        acc_y += (r - '0');
        l = read(0, &r, 1);
        if (-1 == l) return -1;
    }
    l = read(0, &r, 1);
    if (-1 == l) return -1;
    while (r != 'R') {
        acc_x *= 10;
        acc_x += (r - '0');
        l = read(0, &r, 1);
        if (-1 == l) return -1;
    }

    return acc_x - 1;
}

int
to_utf8(unsigned int c, unsigned char *utf8s) {
    if (c <= 0x7f) {
        utf8s[0] = c;
        utf8s[1] = 0;
        return 1;
    } else if (c <= 0x7ff) {
        utf8s[0] = 0xc0 | ((c >> 6) & 0x1f);
        utf8s[1] = 0x80 | (c & 0x3f);
        utf8s[2] = 0;
        return 2;
    } else if (c <= 0xffff) {
        utf8s[0] = 0xe0 | ((c >> 12) & 0x0f);
        utf8s[1] = 0x80 | ((c >> 6) & 0x3f);
        utf8s[2] = 0x80 | (c & 0x3f);
        utf8s[3] = 0;
        return 3;
    } else if (c <= 0x1fffff) {
        utf8s[0] = 0xf0 | ((c >> 18) & 0x7);
        utf8s[1] = 0x80 | ((c >> 12) & 0x3f);
        utf8s[2] = 0x80 | ((c >> 6) & 0x3f);
        utf8s[3] = 0x80 | (c & 0x3f);
        utf8s[4] = 0;
        return 4;
    }

    /* bad utf-8 codepoint */
    utf8s[0] = 0;
    return 0;
}

