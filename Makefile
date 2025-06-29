CFLAGS=-Wall -g -O0 -I.

all: test-sparsexml examples/simple examples/bench

test: test-sparsexml
	./$<

test-sparsexml: sparsexml.o test.o test-private.o test-oss-xml.o test-entities.o
	$(CC) $(CFLAGS) -o $@ $^ -lcunit

examples/simple: sparsexml.o examples/simple.o
	$(CC) $(CFLAGS) -o $@ $^

examples/bench: sparsexml.o examples/bench.o
	$(CC) $(CFLAGS) -o $@ $^ -lexpat

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f test-sparsexml sample/simple *.o sample/*.o

.PHONY: clean all test
