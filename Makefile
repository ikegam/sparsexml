CC ?= gcc
CFLAGS ?= -Wall -Wextra -g -O0
override CPPFLAGS += -I.
LDFLAGS ?= -Wl,-z,noexecstack

SRC = sparsexml.c
OBJ = $(SRC:.c=.o)

TEST_SRC = test.c test-private.c test-oss-xml.c test-entities.c test-exi.c test-chunked.c test-log.c
TEST_OBJ = $(TEST_SRC:.c=.o)

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
            bench/bench_exi.c \
            bench/tinyxml_stub.c \
            bench/main.c
BENCH_OBJ = $(BENCH_SRC:.c=.o)

# Each benchmark also has a main() of its own unless built as a library
BENCH_LIBRARY_OBJ = $(filter-out bench/tinyxml_stub.o bench/main.o,$(BENCH_OBJ))
$(BENCH_LIBRARY_OBJ): override CPPFLAGS += -DBENCH_LIBRARY

all: test-sparsexml examples/simple bench/bench

test: test-sparsexml
	./$<

test-sparsexml: $(OBJ) $(TEST_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ -lcunit

examples/simple: $(OBJ) $(EXAMPLES_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

bench/bench: $(OBJ) $(BENCH_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ -lexpat

%.o: %.c sparsexml.h sparsexml-priv.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TEST_OBJ): test-log.h

# Most callbacks do not need the explorer they receive
$(TEST_OBJ) $(EXAMPLES_OBJ) $(BENCH_OBJ): override CFLAGS += -Wno-unused-parameter

clean:
	rm -f $(OBJ) $(TEST_OBJ) $(EXAMPLES_OBJ) $(BENCH_OBJ)
	rm -f test-sparsexml examples/simple bench/bench

.PHONY: clean all test
