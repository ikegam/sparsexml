CC ?= gcc
CFLAGS ?= -Wall -g -O0 -I.
LDFLAGS ?= -Wl,-z,noexecstack

SRC = sparsexml.c
OBJ = $(SRC:.c=.o)

TEST_SRC = test.c test-private.c test-oss-xml.c test-entities.c test-exi.c
TEST_OBJ = $(TEST_SRC:.c=.o)
EXI_TOOL = scripts/xml_to_exi.py

EXAMPLES_SRC = examples/simple.c
EXAMPLES_OBJ = $(EXAMPLES_SRC:.c=.o)
BENCH_SRC = bench/bench.c \
            bench/bench_large_mem.c \
            bench/bench_deep_nesting.c \
            bench/bench_many_attrs.c \
            bench/bench_comments.c \
            bench/bench_entities.c \
            bench/bench_mixed_content.c \
            bench/bench_stress_test.c \
            bench/bench_unicode_content.c \
            bench/tinyxml_stub.c \
            bench/main.c
BENCH_OBJ = $(BENCH_SRC:.c=.o)

all: test-sparsexml examples/simple bench/bench test-oss-1.exi

test: test-sparsexml
	./$<

test-sparsexml: $(OBJ) $(TEST_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ -lcunit

examples/simple: $(OBJ) examples/simple.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

bench/bench: $(OBJ) $(BENCH_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ -lexpat
bench/bench.o: bench/bench.c
	$(CC) $(CFLAGS) -DBENCH_LIBRARY -c $< -o $@
bench/bench_large_mem.o: bench/bench_large_mem.c
	$(CC) $(CFLAGS) -DBENCH_LIBRARY -c $< -o $@
bench/bench_deep_nesting.o: bench/bench_deep_nesting.c
	$(CC) $(CFLAGS) -DBENCH_LIBRARY -c $< -o $@
bench/bench_many_attrs.o: bench/bench_many_attrs.c
	$(CC) $(CFLAGS) -DBENCH_LIBRARY -c $< -o $@
bench/bench_comments.o: bench/bench_comments.c
	$(CC) $(CFLAGS) -DBENCH_LIBRARY -c $< -o $@
bench/bench_entities.o: bench/bench_entities.c
	$(CC) $(CFLAGS) -DBENCH_LIBRARY -c $< -o $@
bench/bench_mixed_content.o: bench/bench_mixed_content.c
	$(CC) $(CFLAGS) -DBENCH_LIBRARY -c $< -o $@
bench/bench_stress_test.o: bench/bench_stress_test.c
	$(CC) $(CFLAGS) -DBENCH_LIBRARY -c $< -o $@
bench/bench_unicode_content.o: bench/bench_unicode_content.c
	$(CC) $(CFLAGS) -DBENCH_LIBRARY -c $< -o $@

test-oss-1.exi: test-oss-1.xml $(EXI_TOOL)
	python3 $(EXI_TOOL) $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TEST_OBJ) $(EXAMPLES_OBJ) $(BENCH_OBJ)
	rm -f test-sparsexml examples/simple bench/bench test-oss-1.exi

.PHONY: clean all test
