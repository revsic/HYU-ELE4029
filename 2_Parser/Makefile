CC = gcc
CFLAGS = 

OBJS = main.o util.o lex.yy.o y.tab.o

all: cminus

cminus: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ -lfl

main.o: main.c globals.h y.tab.h util.h scan.h parse.h
	$(CC) $(CFLAGS) -c main.c

util.o: util.c util.h globals.h y.tab.h
	$(CC) $(CFLAGS) -c util.c

lex.yy.c: cminus.l
	flex cminus.l

lex.yy.o: lex.yy.c globals.h util.h scan.h
	$(CC) $(CFLAGS) -c lex.yy.c

y.tab.c: cminus.y
	yacc -d -v cminus.y

y.tab.h: y.tab.c

y.tab.o: y.tab.c y.tab.h globals.h util.h scan.h parse.h
	$(CC) $(CFLAGS) -c y.tab.c

clean:
	rm -vf cminus *.o lex.yy.c y.tab.c y.tab.h y.output
