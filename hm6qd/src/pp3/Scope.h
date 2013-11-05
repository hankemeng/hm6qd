#ifndef SCOPE_H
#define SCOPE_H

#include "hashtable.h"

class Decl;
class Identifier;
class ClassDecl; 

class Scope { 
  public:
    Hashtable<Decl*> *table;

    Scope();

    Decl *Lookup(Identifier *id);
    bool Declare(Decl *dec);
    void CopyFromScope(Scope *other, ClassDecl *cd);
};

#endif
