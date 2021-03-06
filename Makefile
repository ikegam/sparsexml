CFLAGS=-Wall -g -O0

all: test-sparsexml

test: test-sparsexml
	./$<

test-sparsexml: sparsexml.o test.o test-private.o
	$(CC) $(CFLAGS) -o $@ $^ -lcunit

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm test-sparsexml *.o

.PHONY: clean all test
