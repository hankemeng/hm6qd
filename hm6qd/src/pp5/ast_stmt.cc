/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"


Program::Program(List<Decl*> *d) {
    Assert(d != NULL);
    (decls=d)->SetParentAll(this);
    cgen=new CodeGenerator();
}

Scope* Program::PrepareScope() {
    nodeScope = new Scope(this);
    decls->DeclareAll(nodeScope);
    // decls->CheckAll();
    decls->PrepareScopeAll();
    return nodeScope;
}

void Program::Emit() {
    /* pp5: here is where the code generation is kicked off.
     *      The general idea is perform a tree traversal of the
     *      entire program, generating instructions as you go.
     *      Each node can have its own way of translating itself,
     *      which makes for a great use of inheritance and
     *      polymorphism in the node classes.
     */

    // decls->EmitAll(cgen);
    for (int i=0; i<decls->NumElements(); i++){
        decls->Nth(i)->Emit(cgen);
    }
    cgen->DoFinalCodeGen();
}

StmtBlock::StmtBlock(List<VarDecl*> *d, List<Stmt*> *s) {
    Assert(d != NULL && s != NULL);
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
}

Scope* StmtBlock::PrepareScope() {
    if (nodeScope) return nodeScope;
    nodeScope = new Scope(this);
    decls->DeclareAll(nodeScope);
    // decls->CheckAll();
    // stmts->CheckAll();
    decls->PrepareScopeAll();
    stmts->PrepareScopeAll();
    return nodeScope;
}

void StmtBlock::Emit(CodeGenerator * cgen){
    for (int i=0; i<decls->NumElements(); i++)
        decls->Nth(i)->Emit(cgen);
    for (int i=0; i<stmts->NumElements(); i++)
        stmts->Nth(i)->Emit(cgen);    
}

ConditionalStmt::ConditionalStmt(Expr *t, Stmt *b) { 
    Assert(t != NULL && b != NULL);
    (test=t)->SetParent(this); 
    (body=b)->SetParent(this);
}

void ConditionalStmt::Emit(CodeGenerator * cgen){
    Location* testloc = test->codegen(cgen);

}

ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b): LoopStmt(t, b) { 
    Assert(i != NULL && t != NULL && s != NULL && b != NULL);
    (init=i)->SetParent(this);
    (step=s)->SetParent(this);
}

void ForStmt::Emit(CodeGenerator * cgen){
    loopLabel = cgen->NewLabel();
    endLabel = cgen->NewLabel(); 

    init->codegen(cgen);
    cgen->GenLabel(loopLabel);
    Location* testloc = test->codegen(cgen);
    cgen->GenIfZ(testloc, endLabel);
    body->Emit(cgen);
    step->codegen(cgen);
    cgen->GenGoto(loopLabel);
    cgen->GenLabel(endLabel);

}


void WhileStmt::Emit(CodeGenerator * cgen){
    loopLabel = cgen->NewLabel();
    endLabel = cgen->NewLabel();

    cgen->GenLabel(loopLabel);
    Location* testloc = test->codegen(cgen);
    cgen->GenIfZ(testloc, endLabel);
    body->Emit(cgen);
    cgen->GenGoto(loopLabel);
    cgen->GenLabel(endLabel);

}

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb): ConditionalStmt(t, tb) { 
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
} 

void IfStmt::Emit(CodeGenerator * cgen){
    Location* testloc = test->codegen(cgen);
    char* elseLabel = cgen->NewLabel();
    char* endLabel = cgen->NewLabel();

    cgen->GenIfZ(testloc, elseLabel);
    body->Emit(cgen);
    cgen->GenGoto(endLabel);
    cgen->GenLabel(elseLabel);
    elseBody->Emit(cgen);
    cgen->GenLabel(endLabel);

}

void BreakStmt::Emit(CodeGenerator * cgen){
    Node* current=this;
    while((current=current->GetParent())){
        if (current->IsLoopStmt()) {
            cgen->GenGoto(dynamic_cast<LoopStmt*> (current)->endLabel);
        }
    }
}


ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) { 
    Assert(e != NULL);
    (expr=e)->SetParent(this);
}

void ReturnStmt::Emit(CodeGenerator * cgen){
    Location * result = expr->codegen(cgen);
    cgen->GenReturn(result);
}
  
PrintStmt::PrintStmt(List<Expr*> *a) {    
    Assert(a != NULL);
    (args=a)->SetParentAll(this);
}

void PrintStmt::Emit(CodeGenerator *cgen){
    for (int i=0; i<args->NumElements(); i++){
        Location* arg = args->Nth(i)->codegen(cgen);

        Type* t=args->Nth(i)->InferType();
        if (t==Type::intType)
            cgen->GenBuiltInCall(PrintInt,arg, NULL);
        if (t==Type::stringType)
            cgen->GenBuiltInCall(PrintString,arg, NULL);
        if (t==Type::boolType)
            cgen->GenBuiltInCall(PrintBool,arg, NULL);    
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

