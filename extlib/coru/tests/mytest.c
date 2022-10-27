#include <stdio.h>
#include <stdlib.h>
#include <coru.h>

void
f1(void *p) {
    fprintf(stdout, "f1::p=%d\n", *(int*)p);
    coru_yield();
    return;
}

void
f2(void *p) {
    fprintf(stdout, "f2::p=%d\n", *(int*)p);
    coru_yield();
    return;
}

void
f3(void *p) {
    fprintf(stdout, "f3::p=%d\n", *(int*)p);
    coru_yield();
    return;
}

int main(int argc, char **argv) {
    coru_t c1, c2, c3;
    int p1=1, p2=2, p3=3;
    void *s1, *s2, *s3;
    
    s1 = malloc(4096);
    s2 = malloc(4096);
    s3 = malloc(4096);
    coru_create(&c1, f1, (void*)&p1, 4096, s1);
    coru_create(&c2, f2, (void*)&p2, 4096, s2);
    coru_create(&c3, f3, (void*)&p3, 4096, s3);
    
    fprintf(stdout, "f1::result=%d\n", coru_resume(&c1));
    fprintf(stdout, "f2::result=%d\n", coru_resume(&c2));
    fprintf(stdout, "f3::result=%d\n", coru_resume(&c3));
    
    fprintf(stdout, "f1::result=%d\n", coru_resume(&c1));
    fprintf(stdout, "f2::result=%d\n", coru_resume(&c2));
    fprintf(stdout, "f3::result=%d\n", coru_resume(&c3));

    fprintf(stdout, "f1::result=%d\n", coru_resume(&c1));
    fprintf(stdout, "f2::result=%d\n", coru_resume(&c2));
    fprintf(stdout, "f3::result=%d\n", coru_resume(&c3));

    coru_destroy(&c1);
    coru_destroy(&c2);
    coru_destroy(&c3);
}

