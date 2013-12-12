/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */
#include "ast_expr.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h>
#include "codegen.h"


IntConstant::IntConstant(yyltype loc, int val) : Expr(loc) {
    value = val;
    type=Type::intType;
}

Location* IntConstant::codegen(CodeGenerator * cgen){
    return cgen->GenLoadConstant(value);
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

Location* StringConstant::codegen(CodeGenerator * cgen){
    return cgen->GenLoadConstant(value);
}

NullConstant::NullConstant(yyltype loc) : Expr(loc) {
    type=Type::nullType;
}

Operator::Operator(yyltype loc, const char *tok) : Node(loc) {
    Assert(tok != NULL);
    strncpy(tokenString, tok, sizeof(tokenString));
}

bool Operator::Equals(const char* tok){
    return strncmp(tokenString, tok, 2)==0;
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
   
Location* CompoundExpr::codegen(CodeGenerator* cgen){
    //Assume binary here
    Location * l = left->codegen(cgen);
    Location * r = right->codegen(cgen);
    return cgen->GenBinaryOp(op->GetName(), l, r);   

}

Location* ArithmeticExpr::codegen(CodeGenerator* cgen){
    if (left)
        return CompoundExpr::codegen(cgen);
    Location * l = cgen->GenLoadConstant(0);
    Location * r = right->codegen(cgen);
    return cgen->GenBinaryOp(op->GetName(), l, r);   
}

Location* RelationalExpr::codegen(CodeGenerator* cgen){
    if (op->Equals(">") || op->Equals("<"))
        return CompoundExpr::codegen(cgen);
    if (op->Equals(">=") || op->Equals("<=")){
        char tempop[4];
        tempop[0]=(op->GetName())[0];
        tempop[1]='\0';
        Location * l = left->codegen(cgen);
        Location * r = right->codegen(cgen);
        Location * step1 = cgen->GenBinaryOp(tempop, l, r);   
        Location * step2 = cgen->GenBinaryOp("==", l, r);   
        Location * result = cgen->GenBinaryOp("||", step1, step2);   
        return result;

    }
    return NULL;
}

Location* EqualityExpr::codegen(CodeGenerator* cgen){
    if (op->Equals("=="))
        return CompoundExpr::codegen(cgen);
    if (op->Equals("!=")){
        Location * l = left->codegen(cgen);
        Location * r = right->codegen(cgen);
        Location * step1 = cgen->GenBinaryOp("==", l, r);   
        Location * step2 = cgen->GenLoadConstant(0);   
        Location * result = cgen->GenBinaryOp("==", step1, step2);   
        return result;

    }
    return NULL;
}

Location* LogicalExpr::codegen(CodeGenerator* cgen){
    if (left)
        return CompoundExpr::codegen(cgen);
    Assert(strncmp(op->GetName(), "!", 4)==0);
    Location * l = cgen->GenLoadConstant(0);
    Location * r = right->codegen(cgen);
    return cgen->GenBinaryOp("==", r, l);   
}

Location* AssignExpr::codegen(CodeGenerator * cgen){
    Location * dst = left->codegen(cgen);
    Location * scr = right->codegen(cgen);
    // if (!dst)
    //     printf("AssignExpr::codegen(): dst==NULL\n");
    // if (!scr)
    //     printf("AssignExpr::codegen(): scr==NULL\n");


    if (left->IsArrayAccess()){
        cgen->GenStore(dst, scr);
    }else if (right->IsArrayAccess()){
        dst=cgen->GenLoad(scr);
    }else
        cgen->GenAssign(dst, scr);

    return dst;
}
void AssignExpr::Emit(CodeGenerator * cgen){
    codegen(cgen);
}

ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base=b)->SetParent(this); 
    (subscript=s)->SetParent(this);
}

Location* ArrayAccess::codegen(CodeGenerator* cgen){
    Location* baseLoc = base->codegen(cgen);
    Location* subLoc = subscript->codegen(cgen);
    Location * result = cgen->GenArrayAccess(baseLoc, subLoc);
    
    if (parent->IsAssignExpr())
        return result;
    return cgen->GenLoad(result);
}

     
FieldAccess::FieldAccess(Expr *b, Identifier *f) 
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b; 
    if (base) base->SetParent(this); 
    (field=f)->SetParent(this);
    baseDecl=NULL;
    classDecl=NULL;
    fieldDecl=NULL;
}

/*
1. Object.field
2. (this.)field
3. 

*/
Type* FieldAccess::InferType(){
    if (type) return type;
    Type* baseType = base? base->InferType() : NULL;
    fieldDecl = field->GetDeclForId(baseType);
    if (fieldDecl)
        type=dynamic_cast<VarDecl*>(fieldDecl)->GetDeclaredType();

    if (!base){
        // if no base and is field, this.field
        if (fieldDecl && fieldDecl->IsFieldDecl()) { 
            base = new This(*field->GetLocation()); //??
            base->SetParent(this);
            base->InferType();

        }else if (fieldDecl && !fieldDecl->IsFieldDecl()){
            //not inside class, global var
        }
    }

    if (base && !base->InferType()){

       //printf("FieldAccess::InferType(): base && !base->InferType()\n");
    }

    if (base && base->InferType()->IsNamedType()){
        FieldAccess* _base = dynamic_cast<FieldAccess*>(base);
        baseDecl = _base? _base->fieldDecl : NULL; //what about classdecl??
        classDecl= dynamic_cast<NamedType*> (base->InferType()) -> GetDeclForType();
    }

/*
    if (base){
        l=kShallow;
        
        FieldAccess* _base= dynamic_cast<FieldAccess*>(base);
        if (!_base) {
            //printf("FieldAccess::InferType(): Cannot convert Expr to FieldAccess\n");
        }

        baseDecl= _base->fieldDecl;

        if (baseDecl->IsVarDecl()){ 
            //get the classDecl for the var
            NamedType* t =dynamic_cast<NamedType*> (dynamic_cast<VarDecl*>(baseDecl)->GetDeclaredType());
            classDecl =t->GetDeclForType();

            fieldDecl=classDecl-> FindDecl(field, kShallow);

            // Infer Type
            if (!fieldDecl) {
                // ReportError::FieldNotFoundInBase(field, base->InferType());
                //printf("FieldAccess::InferType(): FieldNotFoundInBase(%s)\n",field->GetName());
                type=Type::errorType;
                // return;
            }
            type=dynamic_cast<VarDecl*> (fieldDecl)->GetDeclaredType();
        }

    } else {

        if (!fieldDecl)
            fieldDecl= FindDecl(field, kDeep);
        if (fieldDecl->IsVarDecl()){

            type=dynamic_cast<VarDecl*>(fieldDecl)->GetDeclaredType();
            NamedType* t =dynamic_cast<NamedType*> (type);
            if (t){
                classDecl = t->GetDeclForType();
            }
        }
    }
*/
    return type;

}

Location* FieldAccess::codegen(CodeGenerator* cgen){
    InferType();
    // fieldDecl= FindDecl(field, kDeep);
    if (base) base->codegen(cgen);
    // field->Emit(cgen);
    return fieldDecl->tacloc;
}

Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr*> *a) : Expr(loc)  {
    Assert(f != NULL && a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
    (actuals=a)->SetParentAll(this);
}
 
Type* Call::InferType(){
    if (type) return type;

    //Check Array.length()
    if (base && base->InferType() && base->InferType()->IsArrayType() && strcmp(field->GetName(), "length") == 0) {
        // if (actuals->NumElements() != 0) 
            // ReportError::NumArgsMismatch(field, 0, actuals->NumElements());
        type= Type::intType;
    }

    Type* baseType = base? base->InferType() : NULL;
    funcDecl = field->GetDeclForId(baseType);

    // if funcDecl found and can be converted to FnDecl
    if (funcDecl && funcDecl->IsFnDecl())
        type=dynamic_cast<FnDecl*>(funcDecl)->GetReturnType();

    if (!base){
        // if no base and is method, this.field
        if (funcDecl && funcDecl->IsMethodDecl()) { 
            base = new This(*field->GetLocation()); //??
            base->SetParent(this);
            base->InferType();

        }else if (funcDecl && !funcDecl->IsMethodDecl()){
            //not inside class, global func
        }
    }

    if (base && !base->InferType()){
       //printf("FieldAccess::InferType(): base && !base->InferType()\n");
    }

    if (base && base->InferType()->IsNamedType()){
        FieldAccess* _base = dynamic_cast<FieldAccess*>(base);
        baseDecl = _base? _base->fieldDecl : NULL; //what about classdecl??
        classDecl= dynamic_cast<NamedType*> (base->InferType()) -> GetDeclForType();
    }


/*    if (base){
        base->InferType();
        FieldAccess* _base= dynamic_cast<FieldAccess*>(base);
        if (!_base) {
            //printf("Cannot convert Expr to FieldAccess\n");
        }

        //Check Array.length()
        if (base->InferType() && base->InferType()->IsArrayType() && strcmp(field->GetName(), "length") == 0) {
            // if (actuals->NumElements() != 0) 
                // ReportError::NumArgsMismatch(field, 0, actuals->NumElements());
            return Type::intType;
        }

        if (_base->classDecl){
            funcDecl=(_base->classDecl-> FindDecl(field, kShallow));
            FnDecl* _funcDecl = dynamic_cast<FnDecl*> (funcDecl);
            if (!_funcDecl) {
                //printf("Call::InferType(): cannot find funcDecl\n");
                type=Type::errorType;
            }
            type=_funcDecl->GetReturnType();
        }else{
            //printf("Call::InferType(): no classDecl in base!\n");
        }

    }else{
        funcDecl = (FindDecl(field));
        FnDecl* _funcDecl = dynamic_cast<FnDecl*> (funcDecl);
        type=_funcDecl? _funcDecl->GetReturnType() : NULL;
    }
*/

    return type;
} 

Location* Call::codegen(CodeGenerator* cgen){

    InferType();
    Location * result = NULL;

    //Array.length()
    if (base && base->InferType() && base->InferType()->IsArrayType() && strcmp(field->GetName(), "length") == 0) { 
        Location* baseLoc = base->codegen(cgen);
        result = cgen->GenArrayLen(baseLoc);
        return result;
    }

    FnDecl* _funcDecl = dynamic_cast<FnDecl*>(funcDecl);
    Assert(_funcDecl->NumArgs()==actuals->NumElements());

    List<Location*> *params = new List<Location*>;
    for (int i=0; i<actuals->NumElements(); i++){
        params->Append(actuals->Nth(i)->codegen(cgen));
    }

    if (base){
        result = cgen->GenDynamicDispatch(base->codegen(cgen), _funcDecl->GetOffset(), params, _funcDecl->GetReturnType()!=Type::voidType);
    }else{
        for (int i=actuals->NumElements()-1; i>=0; i--){
            cgen->GenPushParam(params->Nth(i));
        }
        result=cgen->GenLCall(_funcDecl->GetFuncLabel(), _funcDecl->GetReturnType()!=Type::voidType);
        cgen->GenPopParams(cgen->VarSize * actuals->NumElements());
    }
    return result;

}

NewExpr::NewExpr(yyltype loc, NamedType *c) : Expr(loc) { 
  Assert(c != NULL);
  (cType=c)->SetParent(this);
  type=cType;
}

Location* NewExpr::codegen(CodeGenerator *cgen) { 
    Location *result;
    ClassDecl *cd = dynamic_cast<ClassDecl*>(cType->GetDeclForType());
    result = cgen->GenNew(cd->GetName(), cd->fieldCount); 
    return result;
}


NewArrayExpr::NewArrayExpr(yyltype loc, Expr *sz, Type *et) : Expr(loc) {
    Assert(sz != NULL && et != NULL);
    (size=sz)->SetParent(this); 
    (elemType=et)->SetParent(this);
}

Type * NewArrayExpr::InferType() {
    if (type) return type;
    size->InferType();
    // if (!sizet->IsCompatibleWith(Type::intType))
    //     ReportError::NewArraySizeNotInteger(size);
    // elemType->Check();
    type = new ArrayType(*GetLocation(), elemType);
    return type;
}

Location* NewArrayExpr::codegen(CodeGenerator* cgen){
    Location *result = cgen->GenNewArray(size->codegen(cgen));
    return result;
}

PostfixExpr::PostfixExpr(LValue *lv, Operator *o) : Expr(Join(lv->GetLocation(), o->GetLocation())) {
    Assert(lv != NULL && o != NULL);
    (lvalue=lv)->SetParent(this);
    (op=o)->SetParent(this);
}
  
Type* ArithmeticExpr::InferType(){
    if (type) return type;

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
        // ReportError::IncompatibleOperands(op, left->InferType(), right->InferType());

    }else{
        //unary operation
        if (right->InferType()==Type::intType || right->InferType()==Type::doubleType){
            type=right->InferType();
            return type;
        }
        type=Type::errorType;
        // ReportError::IncompatibleOperand(op, right->InferType());
    }

    return type;
}

Type* RelationalExpr::InferType(){
    if (type) return type;

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
        // ReportError::IncompatibleOperands(op, left->InferType(), right->InferType());

    }else{
        type=Type::errorType;
        // ReportError::IncompatibleOperands( op, Type::voidType,right->InferType());
    }

    return type;
}

Type* EqualityExpr::InferType(){
    if (type) return type;

    if (left){
        //two operands
        if (right->InferType()->IsEquivalentTo(left->InferType()) || left->InferType()->IsEquivalentTo(right->InferType())){
                //Objects & null
                type=Type::boolType;
                return type;
        }    
        //else left exists and not equivalent
        type=Type::errorType;
        // ReportError::IncompatibleOperands(op, left->InferType(), right->InferType());

    }else{
        type=Type::errorType;
        // ReportError::IncompatibleOperands( op, Type::voidType,right->InferType());
    }

    return type;
}

Type* LogicalExpr::InferType(){
    if (type) return type;

    if (left){
        //two operands
        if (right->InferType()==Type::boolType || right->InferType()==Type::boolType){
            type=Type::boolType;
            return type;
        }
        //else left exists and not equivalent
        type=Type::errorType;
        // ReportError::IncompatibleOperands(op, left->InferType(), right->InferType());

    }else{
        //unary operation
        if (right->InferType()==Type::boolType){
            type=Type::boolType;
            return type;
        }
        type=Type::errorType;
        // ReportError::IncompatibleOperand(op, right->InferType());
    }

    return type;
}

Type* AssignExpr::InferType(){
    if (type) return type;
    if (left->InferType()->IsEquivalentTo(right->InferType())){
        // ReportError::IncompatibleOperands(op, left->InferType(), right->InferType());
        type = left->InferType();
    }

    return type;
}

Type* This::InferType(){
    if (type) return type;

    Node* current=this;
    while((current=current->GetParent())){
        if (current->IsClassDecl()){
            decl=dynamic_cast<ClassDecl*> (current);
            break;
        }
    }
 
    type=decl->GetDeclaredType();
    return type;
}

Type* ArrayAccess::InferType(){
    if (type) return type;
    subscript->InferType();

    return base->InferType();
}

       
