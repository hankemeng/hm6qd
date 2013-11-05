#include "Scope.h"
#include "ast_decl.h"
#include "list.h"

Scope::Scope(){
    table = new Hashtable<Decl*>;
    node=NULL;
}

Scope::Scope(Node* n){
    table = new Hashtable<Decl*>;
    node=n;
}

Decl * Scope::Lookup(Identifier *id) {
    return table->Lookup(id->name);
}

bool Scope::Declare(Decl *dec) {
    Decl * prev = table->Lookup(dec->id->name);
    if (prev){
        if (dec->ConflictsWithPrevious(prev))
            return false;
    }
    table->Enter(dec->id->name, dec, false);
    
    return true;
}

void Scope::CopyFromScope(Scope *other, ClassDecl *cd) {
    Iterator<Decl*> iter = other->table->GetIterator();
    Decl *decl;
    while ((decl = iter.GetNextValue()) != NULL) {
        table->Enter(decl->id->name, decl);
    }
}
