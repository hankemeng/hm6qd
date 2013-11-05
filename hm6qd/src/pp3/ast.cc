/* File: ast.cc
 * ------------
 */

#include "ast.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h> // strdup
#include <stdio.h>  // printf
#include "Scope.h"

Node::Node(yyltype loc) {
    location = new yyltype(loc);
    parent = NULL;
}

Node::Node() {
    location = NULL;
    parent = NULL;
}
	 
Identifier::Identifier(yyltype loc, const char *n) : Node(loc) {
    name = strdup(n);
} 

Decl* Node::FindDecl(Identifier* idToFind){
    if (!nodeScope) PrepareScope();
    Decl* result = nodeScope->Lookup(idToFind);
    if (result) return result;
    //traverse up to parent
    if (parent) {
        return parent->FindDecl(idToFind);
    }
    return NULL;
}
