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
     // if (base->InferType()!=ArrayType){
     //    printf("base->InferType()!=ArrayType\n");
     // }
    if (!base->isArrayType()){
        ReportError::BracketsOnNonArray(base);
    }


    if (subscript->InferType()!=Type::intType){
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
        // find Decl based on Expr:
        // base->Check();
        // base->FindDecl();

    }else{
        //Check variable declaration
        if (!fieldDecl)
            fieldDecl= FindDecl(field);
        if (!fieldDecl) {
            ReportError::IdentifierNotDeclared(field, LookingForVariable);
        }else{
            //getting type of the variable
            InferType();
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
    // base->InferType();
    // for (int i=0; i<actuals->NumElements(); i++){
    //     actuals->Nth(i)->InferType();
    // }

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

       Type * given=actuals->Nth(i)->InferType();
        Type * expected=decl->formals->Nth(i)->GetDeclaredType();
        if (expected!=given){
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
    if (size->InferType()!=Type::intType){
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

Type* ArithmeticExpr::InferType(){
    if (type) return type;
    if (left && !left->InferType()){
        left->InferType();
    }
    if (right && !right->InferType()){
        right->InferType();
    }
    if (left){
        //two operands
        if (right->InferType()->IsEquivalentTo(left->InferType())){
            if (right->InferType()==Type::intType || right->InferType()==Type::doubleType){
                type=right->InferType();
                return type;
            }
        }    
        //else left exists and not equivalent
        type=Type::errorType;
        ReportError::IncompatibleOperands(op, left->InferType(), right->InferType());

    }else{
        //unary operation
        // printf("unary operation\n");
        if (right->InferType()==Type::intType || right->InferType()==Type::doubleType){
            type=right->InferType();
            return type;
        }
        type=Type::errorType;
        ReportError::IncompatibleOperand(op, right->InferType());
    }

    return type;
}

Type* RelationalExpr::InferType(){
    if (type) return type;
    if (left && !left->InferType()){
        left->InferType();
    }
    if (right && !right->InferType()){
        right->InferType();
    }
    if (left){
        //two operands
        if (right->InferType()->IsEquivalentTo(left->InferType())){
            if (right->InferType()==Type::intType || right->InferType()==Type::doubleType){
                type=Type::boolType;
                return type;
            }
        }    
        //else left exists and not equivalent
        type=Type::errorType;
        ReportError::IncompatibleOperands(op, left->InferType(), right->InferType());

    }else{
        type=Type::errorType;
        ReportError::IncompatibleOperands( op, Type::voidType,right->InferType());
    }

    return type;
}

Type* EqualityExpr::InferType(){
    if (type) return type;
   if (left && !left->InferType()){
        left->InferType();
    }
    if (right && !right->InferType()){
        right->InferType();
    }
    if (left){
        //two operands
        if (right->InferType()->IsEquivalentTo(left->InferType()) || left->InferType()->IsEquivalentTo(right->InferType())){
                //Objects & null
                type=Type::boolType;
                return type;
        }    
        //else left exists and not equivalent
        type=Type::errorType;
        ReportError::IncompatibleOperands(op, left->InferType(), right->InferType());

    }else{
        type=Type::errorType;
        ReportError::IncompatibleOperands( op, Type::voidType,right->InferType());
    }

    return type;
}

Type* LogicalExpr::InferType(){
    if (type) return type;
    // printf("InferType()\n");
    if (left && !left->InferType()){
        left->InferType();
    }
    if (right && !right->InferType()){
        right->InferType();
    }

    if (left){
        //two operands
        if (right->InferType()==Type::boolType || right->InferType()==Type::boolType){
            type=Type::boolType;
            return type;
        }
        //else left exists and not equivalent
        type=Type::errorType;
        ReportError::IncompatibleOperands(op, left->InferType(), right->InferType());

    }else{
        //unary operation
        // printf("unary operation\n");
        if (right->InferType()==Type::boolType){
            type=Type::boolType;
            return type;
        }
        type=Type::errorType;
        ReportError::IncompatibleOperand(op, right->InferType());
    }

    return type;
}

void AssignExpr::Check(){






}

Type* This::InferType(){
    if (type) return type;
    type=decl->GetDeclaredType();
    return type;
}

Type* ArrayAccess::InferType(){
    if (type) return type;
    

    return type;
}

Type* FieldAccess::InferType(){
    if (type) return type;
    if (!fieldDecl)
        fieldDecl= FindDecl(field);
    if (fieldDecl)
        type=dynamic_cast<VarDecl*>(fieldDecl)->GetDeclaredType();

    return type;
}

