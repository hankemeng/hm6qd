
%{
#include "symtab.h"
//#include "token.h"
#include "turtle_hm6qd.tab.h"
%}

ID [a-z]+

%%

\/\*([^\*]|[\r\n]|(\*([^\/]|[\r\n])))*\*\/ ;
\/\/.*          ;
[[:space:]]     ; 

GO              return GO;
TURN            return TURN; 
VAR             return VAR;
var             return VAR;
JUMP            return JUMP;
FOR             return FOR;
STEP            return STEP;
TO              return TO;
DO              return DO;
BEGIN           return COPEN;
END             return CCLOSE;

IF              return IF;
THEN            return THEN;
ELSE            return ELSE;
WHILE           return WHILE;
PROCEDURE       return PROCEDURE;
CALL            return CALL;
PARAM           { //printf("PARAM seen"); 
                  return PARAM; };


sin             return SIN;
cos             return COS;
sqrt            return SQRT;


&&              return AND;
\|\|            return OR;
!               return NOT;
NOT             return NOT;
not             return NOT;
XOR             return XOR;
xor             return XOR;
\+\+            return INCREMENT;
--              return DECREMENT;
\+=             return ADDEQUAL;
-=              return SUBEQUAL;

,               return COMMA;
;               return SEMICOLON;
\+              return PLUS;
-               return MINUS;
\*              return TIMES;
\/              return DIV;
\(              return OPEN;
\)              return CLOSE;

\{              return OPENBRACKET;
\}              return CLOSEBRACKET;
\>=             return GREATEREQUAL;
\>              return GREATER;
\<=             return SMALLEREQUAL;
\<              return SMALLER;
==              return EQUALS;
!=              return UNEQUAL;
=               return ASSIGN;


true            return TRUE;
false           return FALSE;
break           return BREAK;
continue        return CONTINUE;

[a-z][a-z0-9]*          { yylval.n = lookup(yytext); 
                  if (yylval.n==NULL)
		    yylval.n = insert(yytext);
                  return ID;
		}
    
[0-9]+\.[0-9]+  yylval.d = atof(yytext);  return FLOAT;
[0-9]+          yylval.i = atoi(yytext);  return NUMBER;

.               fprintf(stderr,"Unexpected character %c (%2X)\n",yytext[0],yytext[0]);

%%
