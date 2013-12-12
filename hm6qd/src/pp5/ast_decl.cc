/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"
#include <string.h>
        
         
Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id=n)->SetParent(this); 
    tacloc=NULL;
}


VarDecl::VarDecl(Identifier *n, Type *t) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
}

void VarDecl::Emit(CodeGenerator * cgen){
    //printf("VarDecl::Emit(): %s\n", GetName());
    if (dynamic_cast<Program*>(parent)) {
        tacloc = cgen -> GenGlobalVar(GetName());
    } else if (!dynamic_cast<ClassDecl*>(parent)) 
        tacloc = cgen -> GenLocalVar(GetName());

}


ClassDecl::ClassDecl(Identifier *n, NamedType *ex, List<NamedType*> *imp, List<Decl*> *m) : Decl(n) {
    // extends can be NULL, impl & mem may be empty lists but cannot be NULL
    Assert(n != NULL && imp != NULL && m != NULL);     
    extends = ex;
    if (extends) extends->SetParent(this);
    (implements=imp)->SetParentAll(this);
    (members=m)->SetParentAll(this);
    cType = new NamedType(n);
    cType->SetParent(this);
    convImp = NULL;
    vtable=new List<const char*>;
    fieldCount=0;
}

// void ClassDecl::Check() {
    //check if extends an existing class
    // if (extends && !extends->IsClass()) {
    //     ReportError::IdentifierNotDeclared(extends->GetId(), LookingForClass);
    //     extends = NULL;
    // }
    // for (int i = 0; i < implements->NumElements(); i++) {
    //     NamedType *in = implements->Nth(i);
    //     if (!in->IsInterface()) {
    //         ReportError::IdentifierNotDeclared(in->GetId(), LookingForInterface);
    //         implements->RemoveAt(i--);
    //     }
    // }
    // PrepareScope();
    // members->CheckAll();
// }

// This is not done very cleanly. I should sit down and sort this out. Right now
// I was using the copy-in strategy from the old compiler, but I think the link to
// parent may be the better way now.
Scope *ClassDecl::PrepareScope()
{
    if (nodeScope) return nodeScope;
    nodeScope = new Scope(this);  
    if (extends) {
        ClassDecl *ext = dynamic_cast<ClassDecl*>(parent->FindDecl(extends->GetId())); 
        if (ext) nodeScope->CopyFromScope(ext->PrepareScope(), this);
    }
    convImp = new List<InterfaceDecl*>;
    for (int i = 0; i < implements->NumElements(); i++) {
        NamedType *in = implements->Nth(i);
        InterfaceDecl *id = dynamic_cast<InterfaceDecl*>(in->FindDecl(in->GetId()));
        if (id) {
        nodeScope->CopyFromScope(id->PrepareScope(), NULL);
            convImp->Append(id);
      }
    }
    MakeVTable();
    members->DeclareAll(nodeScope);

    // CheckImplementAll();
    members->PrepareScopeAll();
    return nodeScope;
}

bool ClassDecl::IsChildOf(NamedType* other){
    if (extends && extends->IsEquivalentTo(other))
        return true;
    for (int i=0; i<implements->NumElements(); i++){
        if (implements->Nth(i)->IsEquivalentTo(other))
            return true;
    }
    return false;
}

void ClassDecl::MakeVTable(){
    /**************** To be implemented!! offsets! ********************/
    for (int i =0; i<members->NumElements(); i++){
        Decl *member = members->Nth(i);
        Decl *prev = nodeScope->Lookup(member->GetId());
        FnDecl* f;

        //Add Functions
        if ((f=dynamic_cast<FnDecl*>(member))){

            /*************************************************/
            if (prev) { //inherit
                member->SetOffset(prev->GetOffset());
                  if (vtable->NumElements() <= member->GetOffset()) {
                    while(vtable->NumElements() < member->GetOffset())
                        vtable->Append(NULL);
                    vtable->Append(f->GetFuncLabel());            
                  } else {
                    vtable->RemoveAt(member->GetOffset());
                    vtable->InsertAt(f->GetFuncLabel(), member->GetOffset());
                  }
            } else {
                member->SetOffset(vtable->NumElements());
                vtable->Append(f->GetFuncLabel());
            }


        }
        //Add vars
        else if (members->Nth(i)->IsVarDecl()){
            member->SetOffset(fieldCount);
            fieldCount += 4; 
        }
    }
}

void ClassDecl::Emit(CodeGenerator * cgen){
    for (int i=0; i<members->NumElements(); i++){
        members->Nth(i)->Emit(cgen);
    }
    cgen->GenVTable(GetName(), vtable);

}

InterfaceDecl::InterfaceDecl(Identifier *n, List<Decl*> *m) : Decl(n) {
    Assert(n != NULL && m != NULL);
    (members=m)->SetParentAll(this);
}

Scope *InterfaceDecl::PrepareScope() {
    if (nodeScope) return nodeScope;
    nodeScope = new Scope(this);  
    members->DeclareAll(nodeScope);
    return nodeScope;
}

void InterfaceDecl::Emit(CodeGenerator * cgen){
    /**************** To be implemented!! ********************/
    
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

Scope* FnDecl::PrepareScope() {
    // returnType->Check();
    if (nodeScope) return nodeScope;
    if (body) {
        nodeScope = new Scope(this);
        formals->DeclareAll(nodeScope);
        // formals->CheckAll();
       body->PrepareScope();
       body->nodeScope->CopyFromScope(nodeScope, NULL);
    }
    return nodeScope;
}

void FnDecl::Emit(CodeGenerator * cgen){
    /**************** To be implemented!! ********************/
    cgen->GenLabel(GetFuncLabel());

   // new Location for each param
    int start = cgen->OffsetToFirstParam;
    if (IsMethodDecl()) start+=cgen->VarSize;
    for (int i=0; i<formals->NumElements(); i++){
        Location * loc = new Location(fpRelative, start + i*cgen->VarSize, formals->Nth(i)->GetName());
        formals->Nth(i)->tacloc=loc;
    }
    cgen -> LocalTempNum=0;

    BeginFunc * f = cgen->GenBeginFunc();

    body->Emit(cgen);
    f -> SetFrameSize(cgen->LocalTempNum * cgen->VarSize);
    cgen->GenEndFunc();
}

const char* FnDecl::GetFuncLabel(){
    ClassDecl *cd;
    if ((cd = dynamic_cast<ClassDecl*>(parent)) != NULL) { 
        //have to be longer!!!**********************
        char temp[strlen(cd->GetName())+strlen(id->GetName())+4];
        sprintf(temp, "_%s.%s", cd->GetName(), id->GetName());
        return strdup(temp);
    } else if (strcmp(id->GetName(), "main")!=0){
        char temp[strlen(id->GetName())+2];
        sprintf(temp, "_%s", id->GetName());
        return strdup(temp);
    }else{
        char temp[strlen(id->GetName())];
        sprintf(temp, "%s", id->GetName());
        return strdup(temp);
    }
}

bool FnDecl::IsMethodDecl() 
  { return dynamic_cast<ClassDecl*>(parent) != NULL || dynamic_cast<InterfaceDecl*>(parent) != NULL; }

