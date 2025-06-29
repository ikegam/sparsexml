#include <stdio.h>
#include <string.h>

int bench_basic_main(int argc, char **argv);
int bench_large_main(int argc, char **argv);
int bench_deep_main(int argc, char **argv);
int bench_attrs_main(int argc, char **argv);
int bench_comments_main(int argc, char **argv);
int bench_entities_main(int argc, char **argv);

int main(int argc, char **argv){
    (void)argc; (void)argv;
    bench_basic_main(argc, argv);
    bench_large_main(argc, argv);
    bench_deep_main(argc, argv);
    bench_attrs_main(argc, argv);
    bench_comments_main(argc, argv);
    bench_entities_main(argc, argv);
    return 0;
}
