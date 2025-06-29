CC ?= gcc
CFLAGS ?= -Wall -g -O0 -I.

SRC = sparsexml.c
OBJ = $(SRC:.c=.o)

TEST_SRC = test.c test-private.c test-oss-xml.c test-entities.c
TEST_OBJ = $(TEST_SRC:.c=.o)

EXAMPLES_SRC = examples/simple.c
EXAMPLES_OBJ = $(EXAMPLES_SRC:.c=.o)
BENCH_SRC = bench/bench.c bench/bench_large_mem.c bench/main.c
BENCH_OBJ = $(BENCH_SRC:.c=.o)

all: test-sparsexml examples/simple bench/bench

test: test-sparsexml
	./$<

test-sparsexml: $(OBJ) $(TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lcunit

examples/simple: $(OBJ) examples/simple.o
	$(CC) $(CFLAGS) -o $@ $^

bench/bench: $(OBJ) $(BENCH_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lexpat
bench/bench.o: bench/bench.c
	$(CC) $(CFLAGS) -DBENCH_LIBRARY -c $< -o $@
bench/bench_large_mem.o: bench/bench_large_mem.c
	$(CC) $(CFLAGS) -DBENCH_LIBRARY -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TEST_OBJ) $(EXAMPLES_OBJ) $(BENCH_OBJ)
	rm -f test-sparsexml examples/simple bench/bench

.PHONY: clean all test
