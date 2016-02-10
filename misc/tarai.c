#include <stdlib.h>
#include <stdio.h>

int tarai(int x, int y, int z);

int main(int argc, char **argv) {
    int x, y, z;
    
    if (argc != 4) {
	fprintf(stderr, "syntax: tarai x y z\n");
	exit(1);
    }
    x = atoi(argv[1]);
    y = atoi(argv[2]);
    z = atoi(argv[3]);
    
    fprintf(stdout, "%d\n", tarai(x, y, z));
    exit(0);
}

int tarai(int x, int y, int z) {
    if (x <= y) return y;
    return tarai(tarai(x-1, y, z),
		 tarai(y-1, z, x), 
		 tarai(z-1, x, y));
}
