CFLAGS=-Wall -g -O0

all: test-sparsexml

test: test-sparsexml
	./$<

test-sparsexml: sparsexml.o test.o test-private.o test-oss-xml.o
	$(CC) $(CFLAGS) -o $@ $^ -lcunit

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f test-sparsexml *.o

.PHONY: clean all test
