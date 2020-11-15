CC = gcc
CFLAGS = 

OBJS = main.o util.o scan.o
OBJS_FLEX = main.o util.o lex.yy.o

.PHONY: all scanner_cimpl scanner_flex $(OBJS) $(OBJS_FLEX) lex.yy.c

all: scanner_cimpl scanner_flex

scanner_cimpl: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

scanner_flex: $(OBJS_FLEX)
	$(CC) $(CFLAGS) $(OBJS_FLEX) -o $@ -lfl

lex.yy.o: ./lex/cminus.l
	flex $^
	$(CC) $(CFLAGS) -c lex.yy.c

main.o: main.c globals.h util.h scan.h
	$(CC) $(CFLAGS) -c main.c

util.o: util.c util.h globals.h
	$(CC) $(CFLAGS) -c util.c

scan.o: scan.c scan.h util.h globals.h
	$(CC) $(CFLAGS) -c scan.c

clean:
	rm -vf scanner_cimpl scanner_flex *.o lex.yy.c
