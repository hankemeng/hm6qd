#ifndef SCOPE_H
#define SCOPE_H

#include "hashtable.h"


class Decl;
class Identifier;
class ClassDecl;
class Node;

class Scope { 
  public:
    Hashtable<Decl*> *table;
    Node * node;
    
    Scope();
    Scope(Node* n);

    Decl *Lookup(Identifier *id);
    bool Declare(Decl *dec);
    void CopyFromScope(Scope *other, ClassDecl *cd);
    Node* GetNode(){return node;}
};

#endif
