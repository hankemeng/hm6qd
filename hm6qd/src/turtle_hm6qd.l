
%{
#include "symtab.h"
#include "token.h"
%}

ID [a-z]+

%%

\/\/.*          ;
[[:space:]]     ; 

GO              return GO;
TURN            return TURN; 
VAR             return VAR;
JUMP            return JUMP;
FOR             return FOR;
STEP            return STEP;
TO              return TO;
DO              return DO;
BEGIN           return CBEGIN;
END             return CEND;

IF              return IF;
THEN            return THEN;
ELSE            return ELSE;
WHILE           return WHILE;
PROCEDURE       return PROCEDURE;



sin             return SIN;
cos             return COS;
sqrt            return SQRT;

;               return SEMICOLON;
\+              return PLUS;
-               return MINUS;
\*              return TIMES;
\/              return DIV;
\(              return OPEN;
\)              return CLOSE;

\{              return OPENBRACKET;
\}              return CLOSEBRACKET;
,               return COMMA;
\>=             return GREATEREQUAL;
\>              return GREATER;
\<=             return SMALLEREQUAL;
\<              return SMALLER;
==              return EQUALS;
=               return ASSIGN;


[a-z]+          { yylval.n = lookup(yytext); 
                  if (yylval.n==NULL)
		    yylval.n = insert(yytext);
                  return ID;
		}
    
[0-9]+\.[0-9]+  yylval.d = atof(yytext);  return FLOAT;
[0-9]+          yylval.i = atoi(yytext);  return NUMBER;

.               fprintf(stderr,"Unexpected character %c (%2X)\n",yytext[0],yytext[0]);

%%
