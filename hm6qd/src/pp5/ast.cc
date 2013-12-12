/* File: ast.cc
 * ------------
 */

#include "ast.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h> // strdup
#include <stdio.h>  //printf

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

Decl *Node::FindDecl(Identifier *idToFind, lookup l) {
    Decl *mine;
    if (!nodeScope) PrepareScope();
    if (nodeScope && (mine = nodeScope->Lookup(idToFind))){
	  //printf("Node::FindDecl(): found %s\n", idToFind->GetName());
        return mine;
    }
  //printf("Node::FindDecl(): Unable to find %s\n", idToFind->GetName());
    if (l == kDeep && parent)
        return parent->FindDecl(idToFind, l);
  //printf("Node::FindDecl(): Unable to find %s, returning NULL\n", idToFind->GetName());
    return NULL;
}

Decl *Identifier::GetDeclForId(Type *baseType){
    if (!cached) {
        if (!baseType)
            cached = FindDecl(this);
        else if (!baseType->IsNamedType())
            return NULL; 
        else {
            Decl *cd = dynamic_cast<NamedType*>(baseType)->GetDeclForType(); 
            cached = (cd ? cd->FindDecl(this, kShallow) : NULL);
        }
    } 
    return cached;
  }