CC ?= gcc
CFLAGS ?= -Wall -g -O0 -I.

SRC = sparsexml.c
OBJ = $(SRC:.c=.o)

TEST_SRC = test.c test-private.c test-oss-xml.c test-entities.c
TEST_OBJ = $(TEST_SRC:.c=.o)

EXAMPLES_SRC = examples/simple.c examples/bench.c
EXAMPLES_OBJ = $(EXAMPLES_SRC:.c=.o)

all: test-sparsexml examples/simple examples/bench

test: test-sparsexml
	./$<

test-sparsexml: $(OBJ) $(TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lcunit

examples/simple: $(OBJ) examples/simple.o
	$(CC) $(CFLAGS) -o $@ $^

examples/bench: $(OBJ) examples/bench.o
	$(CC) $(CFLAGS) -o $@ $^ -lexpat

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TEST_OBJ) $(EXAMPLES_OBJ)
	rm -f test-sparsexml examples/simple examples/bench

.PHONY: clean all test
