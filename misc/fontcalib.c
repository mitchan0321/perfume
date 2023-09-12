#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>

int to_utf8(unsigned int c, unsigned char *utf8s);

int main(int argc, char **argv) {
    unsigned int i;
    unsigned char buff[5];
    
    for (i=0x20; i<=0x2ffff; i++) {
        if ((i % 32) == 0) {
            fprintf(stdout, "\n");
            fprintf(stdout, "%05x: ", i);
        }
        if (to_utf8(i, buff)) {
            fprintf(stdout, "%s", buff);
        }
    }
    
    /*
    char buff[100];
    int s, i, val;
    struct termios tm, tm_save;
    
    fprintf(stdout, "\e[6n");
    fflush(stdout);
    
    ioctl(0, TCGETS, &tm);
    tm_save = tm;
    cfmakeraw(&tm);
    ioctl(0, TCSETS, &tm);

    s = read(0, buff, 1);
    while (1) {
        if (s > 0) {
            fprintf(stdout, "%02x", buff[0]);
            if (buff[0] == 'R') break;
        } else if ((s == -1) && (errno != EAGAIN)) {
            break;
        }
        s = read(0, buff, 1);
    }
    ioctl(0, TCSETS, &tm_save);
    fprintf(stdout, "\n");
    */
    
    return 0;
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

