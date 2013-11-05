/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"
#include "errors.h"
#include "Scope.h"


Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id=n)->SetParent(this);
}


VarDecl::VarDecl(Identifier *n, Type *t) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
}


ClassDecl::ClassDecl(Identifier *n, NamedType *ex, List<NamedType*> *imp, List<Decl*> *m) : Decl(n) {
    // extends can be NULL, impl & mem may be empty lists but cannot be NULL
    Assert(n != NULL && imp != NULL && m != NULL);
    extends = ex;
    if (extends) extends->SetParent(this);
    (implements=imp)->SetParentAll(this);
    (members=m)->SetParentAll(this);
}


InterfaceDecl::InterfaceDecl(Identifier *n, List<Decl*> *m) : Decl(n) {
    Assert(n != NULL && m != NULL);
    (members=m)->SetParentAll(this);
}


FnDecl::FnDecl(Identifier *n, Type *r, List<VarDecl*> *d) : Decl(n) {
    Assert(n != NULL && r!= NULL && d != NULL);
    (returnType=r)->SetParent(this);
    (formals=d)->SetParentAll(this);
    body = NULL;
}

void FnDecl::SetFunctionBody(Stmt *b) {
    (body=b)->SetParent(this);
}


bool Decl::ConflictsWithPrevious(Decl *prev) {
    ReportError::DeclConflict(this, prev);
    return true;
}

bool FnDecl::ConflictsWithPrevious(Decl *prev) {
    if (IsMethodDecl() && prev->IsMethodDecl() ) {
        //both are inside classes
        if (parent != prev->GetParent()) {
            //in different classes, can be interface implementation
            if (!MatchPrototype(dynamic_cast<FnDecl*> (prev))) {
                ReportError::OverrideMismatch(this);
                return true;
            }
            //else matches prototype and in different classes
            return false;
        }
        //else inside the same class ???
    }
    ReportError::DeclConflict(this, prev);
    return true;
}

bool FnDecl::IsMethodDecl(){
    return dynamic_cast<ClassDecl*>(parent) !=NULL || dynamic_cast<InterfaceDecl*>(parent)!=NULL;
}

bool FnDecl::MatchPrototype(FnDecl *prototype){
    if (id->name == prototype->id->name && returnType==prototype->returnType && formals->NumElements() == prototype->formals->NumElements()) {
        //if name, return type, #arguments match
        for (int i=0; i<formals->NumElements(); i++){
            if (formals->Nth(i)->GetType()!=prototype->formals->Nth(i)->GetType()) { //modify to equivalent
                return false;
            }
        }
        //All arguments types match
        return true;
    }
    return false;
}

/************** Check **********************/
void VarDecl::Check(){
    type->Check();
}

void ClassDecl::Check(){
    if (extends) {
        if (!extends->IsClass()){
            ReportError::IdentifierNotDeclared(extends->GetId(), LookingForClass);
            extends=NULL;
        }
    }
    for (int i=0; i<implements->NumElements(); i++) {
        if (!implements->Nth(i)->IsInterface()){
            ReportError::IdentifierNotDeclared(implements->Nth(i)->GetId(), LookingForInterface);
            implements->RemoveAt(i);
            i--;
        }
    }
    PrepareScope();
    members->CheckAll();
    
}

void InterfaceDecl::Check(){
    
}

void FnDecl::Check(){
    
}

Scope *ClassDecl::PrepareScope()
{
    
    return NULL;
}