/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */
#include "ast_expr.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h>
#include "errors.h"


IntConstant::IntConstant(yyltype loc, int val) : Expr(loc) {
    value = val;
    type=Type::intType;
}

DoubleConstant::DoubleConstant(yyltype loc, double val) : Expr(loc) {
    value = val;
    type=Type::doubleType;
}

BoolConstant::BoolConstant(yyltype loc, bool val) : Expr(loc) {
    value = val;
    type=Type::boolType;
}

StringConstant::StringConstant(yyltype loc, const char *val) : Expr(loc) {
    Assert(val != NULL);
    value = strdup(val);
    type=Type::stringType;
}

NullConstant::NullConstant(yyltype loc) : Expr(loc) {
    type=Type::nullType;
}


Operator::Operator(yyltype loc, const char *tok) : Node(loc) {
    Assert(tok != NULL);
    strncpy(tokenString, tok, sizeof(tokenString));
}

CompoundExpr::CompoundExpr(Expr *l, Operator *o, Expr *r) 
  : Expr(Join(l->GetLocation(), r->GetLocation())) {
    Assert(l != NULL && o != NULL && r != NULL);
    (op=o)->SetParent(this);
    (left=l)->SetParent(this); 
    (right=r)->SetParent(this);
}

CompoundExpr::CompoundExpr(Operator *o, Expr *r) 
  : Expr(Join(o->GetLocation(), r->GetLocation())) {
    Assert(o != NULL && r != NULL);
    left = NULL; 
    (op=o)->SetParent(this);
    (right=r)->SetParent(this);
}
   
  
ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base=b)->SetParent(this); 
    (subscript=s)->SetParent(this);
}

void ArrayAccess::Check(){
    base->Check();
    subscript->Check();
    // To be implemented!! [] can only be applied to arrays
     // if (base->type!=ArrayType){
     //    printf("base->type!=ArrayType\n");
     // }



    if (subscript->type!=Type::intType){
        ReportError::SubscriptNotInteger(subscript);
    }
}
     
FieldAccess::FieldAccess(Expr *b, Identifier *f) 
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b; 
    if (base) base->SetParent(this); 
    (field=f)->SetParent(this);
}

void FieldAccess::Check(){
    // printf("checking FieldAccess\n");
    if (base){
        // ???
        // base->Check();

    }else{
        //Check variable declaration
        Decl* decl= FindDecl(field);
        if (!decl) {
            ReportError::IdentifierNotDeclared(field, LookingForVariable);
        }else{
            //getting type of the variable
            type=dynamic_cast<VarDecl*>(decl)->GetDeclaredType();
        }
    }
}

Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr*> *a) : Expr(loc)  {
    Assert(f != NULL && a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
    (actuals=a)->SetParentAll(this);
}

void Call::Check(){
    //have to fix field access!!
    //Undeclared function
    FnDecl *decl = dynamic_cast<FnDecl*> (FindDecl(field));
    if (!decl) 
    {
        ReportError::IdentifierNotDeclared(field, LookingForFunction);
    }

    //Number of arguments should match
    int numExpected=decl->formals->NumElements();
    int numGiven=actuals->NumElements();
    if (numExpected!=numGiven){
        ReportError::NumArgsMismatch(field, numExpected, numGiven);
    }
    for (int i = 0; i < numExpected && i<numGiven; ++i)
    {
        if (!actuals->Nth(i) || !decl->formals->Nth(i))
            break;
        Type * given=actuals->Nth(i)->type;
        Type * expected=decl->formals->Nth(i)->GetDeclaredType();
        if (expected!=(given)){
             printf("arguments check\n");
           ReportError::ArgMismatch(actuals->Nth(i), i, given, expected);
        }
    }

} 

NewExpr::NewExpr(yyltype loc, NamedType *c) : Expr(loc) { 
  Assert(c != NULL);
  (cType=c)->SetParent(this);
}


NewArrayExpr::NewArrayExpr(yyltype loc, Expr *sz, Type *et) : Expr(loc) {
    Assert(sz != NULL && et != NULL);
    (size=sz)->SetParent(this); 
    (elemType=et)->SetParent(this);
}

void NewArrayExpr::Check(){
    size->Check();
    elemType->Check();
    if (size->type!=Type::intType){
        ReportError::NewArraySizeNotInteger(size);
    }
}

PostfixExpr::PostfixExpr(LValue *lv, Operator *o) : Expr(Join(lv->GetLocation(), o->GetLocation())) {
    Assert(lv != NULL && o != NULL);
    (lvalue=lv)->SetParent(this);
    (op=o)->SetParent(this);
}
   

void CompoundExpr::Check(){
    //Check for variable declaration
    if (left)
        left->Check();
    right->Check();

    //Check for compatible types
    // printf("checking compatible\n");
    InferType();
}

void This::Check(){
    //traverse up to find class until Program root
    Node* current=this;
    while((current=current->GetParent())){
        if (current->IsClassDecl()){
            decl=dynamic_cast<ClassDecl*> (current);
            InferType();
            return;
        }
    }
    ReportError::ThisOutsideClassScope(this);
}

bool ArithmeticExpr::InferType(){
    if (left && !left->type){
        left->InferType();
    }
    if (right && !right->type){
        right->InferType();
    }
    if (left){
        //two operands
        if (right->type->IsEquivalentTo(left->type)){
            if (right->type==Type::intType || right->type==Type::doubleType){
                type=right->type;
                return true;
            }
        }    
        //else left exists and not equivalent
        type=Type::errorType;
        ReportError::IncompatibleOperands(op, left->type, right->type);

    }else{
        //unary operation
        // printf("unary operation\n");
        if (right->type==Type::intType || right->type==Type::doubleType){
            type=right->type;
            return true;
        }
        type=Type::errorType;
        ReportError::IncompatibleOperand(op, right->type);
    }

    return false;
}

bool RelationalExpr::InferType(){
    if (left && !left->type){
        left->InferType();
    }
    if (right && !right->type){
        right->InferType();
    }
    if (left){
        //two operands
        if (right->type->IsEquivalentTo(left->type)){
            if (right->type==Type::intType || right->type==Type::doubleType){
                type=Type::boolType;
                return true;
            }
        }    
        //else left exists and not equivalent
        type=Type::errorType;
        ReportError::IncompatibleOperands(op, left->type, right->type);

    }else{
        type=Type::errorType;
        ReportError::IncompatibleOperands( op, Type::voidType,right->type);
    }

    return false;
}

bool EqualityExpr::InferType(){
    if (left && !left->type){
        left->InferType();
    }
    if (right && !right->type){
        right->InferType();
    }
    if (left){
        //two operands
        if (right->type->IsEquivalentTo(left->type) || left->type->IsEquivalentTo(right->type)){
                //Objects & null
                type=Type::boolType;
                return true;
        }    
        //else left exists and not equivalent
        type=Type::errorType;
        ReportError::IncompatibleOperands(op, left->type, right->type);

    }else{
        type=Type::errorType;
        ReportError::IncompatibleOperands( op, Type::voidType,right->type);
    }

    return false;
}

bool LogicalExpr::InferType(){
    // printf("InferType()\n");
    if (left && !left->type){
        left->InferType();
    }
    if (right && !right->type){
        right->InferType();
    }

    if (left){
        //two operands
        if (right->type==Type::boolType || right->type==Type::boolType){
            type=Type::boolType;
            return true;
        }
        //else left exists and not equivalent
        type=Type::errorType;
        ReportError::IncompatibleOperands(op, left->type, right->type);

    }else{
        //unary operation
        // printf("unary operation\n");
        if (right->type==Type::boolType){
            type=Type::boolType;
            return true;
        }
        type=Type::errorType;
        ReportError::IncompatibleOperand(op, right->type);
    }

    return false;
}

void AssignExpr::Check(){






}

bool This::InferType(){
    type=decl->GetDeclaredType();
    return true;
}

bool ArrayAccess::InferType(){
    

return false;
}