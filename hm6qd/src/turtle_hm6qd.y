
%{
#include <stdio.h>
#include "symtab.h"
%}

%union { int i; node *n; double d;}

%token GO TURN VAR JUMP
%token FOR STEP TO DO
%token COPEN CCLOSE
%token SIN COS SQRT
%token <d> FLOAT
%token <n> ID               
%token <i> NUMBER       
%token SEMICOLON PLUS MINUS TIMES DIV OPEN CLOSE ASSIGN
%token TRUE FALSE
%token AND OR NOT XOR
%token INCREMENT DECREMENT ADDEQUAL SUBEQUAL

%type <n> decl
%type <n> decllist

%token IF THEN ELSE WHILE OPENBRACKET CLOSEBRACKET COMMA
%token GREATER GREATEREQUAL SMALLER SMALLEREQUAL EQUALS UNEQUAL
%token PROCEDURE PARAM CALL
%token BREAK CONTINUE
%%
program: head stmtlist tail;


head: { printf("%%!PS Adobe\n"
               "\n"
	       "newpath 0 0 moveto\n"
	       );
      };

tail: { printf("closepath\nstroke\n"); };

decllist: ;
decllist: decllist decl;

decl: VAR idlist SEMICOLON ;
idlist: idlist COMMA idatom;
idlist: idatom;
idatom: ID { printf("/tlt%s 0 def\n",$1->symbol);};
idatom: ID { printf("/tlt%s 0 def\n",$1->symbol);} 
      ASSIGN expr {printf("/tlt%s exch store\n",$1->symbol);};

stmtlist: ;
stmtlist: stmtlist stmt ;
stmtlist: stmtlist procedure;
stmtlist: stmtlist decllist;

stmt: ID ASSIGN expr SEMICOLON {printf("/tlt%s exch store\n",$1->symbol);} ;
stmt: GO expr SEMICOLON {printf("0 rlineto\n");};
stmt: JUMP expr SEMICOLON {printf("0 rmoveto\n");};
stmt: TURN expr SEMICOLON {printf("rotate\n");};

stmt: FOR ID ASSIGN expr 
          STEP expr
	  TO expr
	  DO {printf("{ /tlt%s exch store\n",$2->symbol);} 
	     stmt {printf("} for\n");};

stmt: COPEN stmtlist CCLOSE;	 



expr: expr PLUS term { printf("add ");};
expr: expr MINUS term { printf("sub ");};
expr: term;

term: term TIMES factor { printf("mul ");};
term: term DIV factor { printf("div ");};
term: factor;

factor: MINUS atomic { printf("neg ");};
factor: PLUS atomic;
factor: SIN factor { printf("sin ");};
factor: COS factor { printf("cos ");};
factor: SQRT factor { printf("sqrt ");};
factor: atomic;


atomic: OPEN expr CLOSE;
atomic: NUMBER {printf("%d ",$1);};
atomic: FLOAT {printf("%f ",$1);};
atomic: ID {printf("tlt%s ", $1->symbol);};


stmt: IF OPEN boolean CLOSE then {printf("{ ");}
      stmtbrkt {printf("} ");}
      elif ;
boolean: expr EQUALS expr {printf("eq\n");};
boolean: expr UNEQUAL expr {printf("ne\n");};
boolean: expr GREATEREQUAL expr {printf("ge\n");};
boolean: expr GREATER expr {printf("gt\n");};
boolean: expr SMALLEREQUAL expr {printf("le\n");};
boolean: expr SMALLER expr {printf("lt\n");};

boolean: boolexpr;
boolexpr: boolatomic;
boolexpr: boolexpr XOR boolexpr {printf("xor ");};
boolexpr: boolexpr AND boolexpr {printf("and ");};
boolexpr: boolexpr OR boolexpr {printf("or ");};
boolexpr: NOT boolexpr {printf("not ");};
boolatomic: TRUE {printf("true ");};
boolatomic: FALSE {printf("false ");};
boolatomic: OPEN boolean CLOSE;

stmtbrkt: stmt;
stmtbrkt: OPENBRACKET 
          stmtlist CLOSEBRACKET ;
then: ;
then: THEN;
elif:  {printf("if \n");};
elif: ELSE {printf("{ ");}
      stmtbrkt {printf("} ifelse \n");};

stmt: WHILE {printf("{ ");} 
      OPEN boolean CLOSE {printf("{} {exit} ifelse \n");}
      stmtbrkt {printf("} loop \n");};

procedure: PROCEDURE ID
           {printf("/proc%s ", $2->symbol);} 
           OPENBRACKET {printf("{ ");} 
           stmtlist CLOSEBRACKET {printf("} def\n");};
stmt: CALL ID paramlist SEMICOLON {printf("proc%s\n",$2->symbol);};
atomic: PARAM;
paramlist: ;
paramlist: paramlist expr;

stmt: ID INCREMENT SEMICOLON {printf("tlt%s 1 add /tlt%s exch store\n",$1->symbol,$1->symbol);};
stmt: ID DECREMENT SEMICOLON {printf("tlt%s 1 sub /tlt%s exch store\n",$1->symbol,$1->symbol);};
stmt: ID ASSIGN ID INCREMENT SEMICOLON {printf("tlt%s 1 add /tlt%s exch store\n",$3->symbol,$1->symbol);};
stmt: ID ASSIGN ID DECREMENT SEMICOLON {printf("tlt%s 1 sub /tlt%s exch store\n",$3->symbol,$1->symbol);};
stmt: ID {printf("tlt%s ",$1->symbol);}
      ADDEQUAL expr SEMICOLON {printf("add /tlt%s exch store\n",$1->symbol);};
stmt: ID {printf("tlt%s ",$1->symbol);}
      SUBEQUAL expr SEMICOLON {printf("sub /tlt%s exch store\n",$1->symbol);};

stmt: BREAK SEMICOLON {printf("break\n");};
stmt: CONTINUE SEMICOLON {printf("continue\n");};


%%
int yyerror(char *msg)
{  fprintf(stderr,"Error: %s\n",msg);
   return 0;
}

int main(void)
{   yyparse();
    return 0;
}

