/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"
#include "scope.h"
#include "errors.h"


Program::Program(List<Decl*> *d) {
    Assert(d != NULL);
    (decls=d)->SetParentAll(this);
}

void Program::Check() {
    nodeScope = new Scope(this);
    decls->DeclareAll(nodeScope);
    decls->CheckAll();
}

StmtBlock::StmtBlock(List<VarDecl*> *d, List<Stmt*> *s) {
    Assert(d != NULL && s != NULL);
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
}
void StmtBlock::Check() {
    // printf("checking StmtBlock\n");
    nodeScope = new Scope(this);
    decls->DeclareAll(nodeScope);
    decls->CheckAll();
    stmts->CheckAll();
}

ConditionalStmt::ConditionalStmt(Expr *t, Stmt *b) { 
    Assert(t != NULL && b != NULL);
    (test=t)->SetParent(this); 
    (body=b)->SetParent(this);
}

void ConditionalStmt::Check() {
    test->Check();
    if (test->InferType()!=Type::boolType){
        ReportError::TestNotBoolean(test);
    }
    body->Check();
}

ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b): LoopStmt(t, b) { 
    Assert(i != NULL && t != NULL && s != NULL && b != NULL);
    (init=i)->SetParent(this);
    (step=s)->SetParent(this);
}

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb): ConditionalStmt(t, tb) { 
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}
void IfStmt::Check() {
    ConditionalStmt::Check();
    if (elseBody) elseBody->Check();
}

void BreakStmt::Check(){
    Node* current=this;
    while((current=current->GetParent())){
        if (current->IsLoopStmt()) return;
    }
    ReportError::BreakOutsideLoop(this);
}

ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) { 
    Assert(e != NULL);
    (expr=e)->SetParent(this);
}
  
void ReturnStmt::Check(){
    expr->Check();
    Node* current=this;
    while((current=current->GetParent())){
        if (current->IsFnDecl()){
            FnDecl* fdecl= dynamic_cast<FnDecl*>(current);
            Type* given=expr->InferType();
            Type* expected=fdecl->GetReturnType();
            if (!given->IsEquivalentTo(expected)){
                ReportError::ReturnMismatch(this, given, expected);
                return;
            }
        }
    }
}

PrintStmt::PrintStmt(List<Expr*> *a) {    
    Assert(a != NULL);
    (args=a)->SetParentAll(this);
}

void PrintStmt::Check(){
    args->CheckAll();
    for (int i=0; i<args->NumElements(); i++){
        Type* type=args->Nth(i)->InferType();
        if (type!=Type::intType && type!=Type::boolType && type!=Type::stringType){
            ReportError::PrintArgMismatch(args->Nth(i), i, type);
        }

    }
}

Case::Case(IntConstant *v, List<Stmt*> *s) {
    Assert(s != NULL);
    value = v;
    if (value) value->SetParent(this);
    (stmts=s)->SetParentAll(this);
}

SwitchStmt::SwitchStmt(Expr *e, List<Case*> *c) {
    Assert(e != NULL && c != NULL);
    (expr=e)->SetParent(this);
    (cases=c)->SetParentAll(this);
}

