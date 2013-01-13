CC	= gcc

compress.out: main.o huffman.o
	$(CC) -s main.o huffman.o -o compress.out

main.o: main.c huffman.o huffman.h
	$(CC) -c main.c

huffman.o: huffman.c huffman.h
	$(CC) -c huffman.c

clean:
	-rm *.o *~ *.out