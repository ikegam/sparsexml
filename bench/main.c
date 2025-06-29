#include <stdio.h>
#include <string.h>

int bench_basic_main(int argc, char **argv);
int bench_large_main(int argc, char **argv);

int main(int argc, char **argv){
    if(argc < 2){
        fprintf(stderr, "Usage: %s [basic|large] [args...]\n", argv[0]);
        return 1;
    }
    if(strcmp(argv[1], "large") == 0){
        return bench_large_main(argc - 1, argv + 1);
    }else if(strcmp(argv[1], "basic") == 0){
        return bench_basic_main(argc - 1, argv + 1);
    }else{
        fprintf(stderr, "Unknown mode: %s\n", argv[1]);
        return 1;
    }
}
