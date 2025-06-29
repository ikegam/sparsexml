CFLAGS=-Wall -g -O0 -I.

all: test-sparsexml sample/simple

test: test-sparsexml
	./$<

test-sparsexml: sparsexml.o test.o test-private.o test-oss-xml.o test-entities.o
	$(CC) $(CFLAGS) -o $@ $^ -lcunit

sample/simple: sparsexml.o sample/simple.o
	$(CC) $(CFLAGS) -o $@ $^

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f test-sparsexml sample/simple *.o sample/*.o

.PHONY: clean all test
