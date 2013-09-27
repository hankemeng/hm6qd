
all: turtle eins.ps

eins.ps: eins.tlt turtle
	./turtle < eins.tlt > eins.ps

lex.yy.o:  lex.yy.c turtle_hm6qd.tab.h symtab.h
	gcc -c lex.yy.c

lex.yy.c: turtle_hm6qd.l
	flex turtle_hm6qd.l

turtle_hm6qd.tab.h: turtle_hm6qd.y
	bison -d turtle_hm6qd.y

turtle_hm6qd.tab.c: turtle_hm6qd.y
	bison -d turtle_hm6qd.y

turtle_hm6qd.tab.o: turtle_hm6qd.tab.c symtab.h
	gcc -c turtle_hm6qd.tab.c

turtle: turtle_hm6qd.tab.o lex.yy.o symtab.c
	gcc lex.yy.o turtle_hm6qd.tab.o symtab.c -lfl -o turtle

clean:
	rm -f turtle lex.* turtle_hm6qd.tab.* *.o

