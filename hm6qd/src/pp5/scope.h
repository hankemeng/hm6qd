/* File: scope.h
 * -------------
 * The Scope class will be used to manage scopes, sort of
 * table used to map identifier names to Declaration objects.
 */

#ifndef _H_scope
#define _H_scope

#include "hashtable.h"
// #include "ast.h"

class Decl;
class Identifier;
class ClassDecl; 
class Node;

class Scope { 
  protected:
    Hashtable<Decl*> *table;
    Node* node;

  public:
    Scope();
	Scope(Node* n);
    Decl *Lookup(Identifier *id);
	Decl *Lookup(char* name);     
    bool Declare(Decl *dec);
    void CopyFromScope(Scope *other, ClassDecl *cd);
};


#endif
