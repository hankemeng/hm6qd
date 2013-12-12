/* File: ast_decl.h
 * ----------------
 * In our parse tree, Decl nodes are used to represent and
 * manage declarations. There are 4 subclasses of the base class,
 * specialized for declarations of variables, functions, classes,
 * and interfaces.
 *
 * pp5: You will need to extend the Decl classes to implement 
 * code generation for declarations.
 */

#ifndef _H_ast_decl
#define _H_ast_decl

#include "ast.h"
#include "list.h"
// #include "scope.h"
// #include "ast_type.h"

class Type;
class NamedType;
class Identifier;
class Stmt;
class InterfaceDecl;
class VarDecl;
class Location;

class Decl : public Node 
{
  protected:
    Identifier *id;
    int offset;
  
  public:
    Location* tacloc;
    Decl(Identifier *name);
    friend std::ostream& operator<<(std::ostream& out, Decl *d) { return out << d->id; }
    Identifier *GetId() { return id; }
    const char *GetName() { return id->GetName(); }
    
    virtual bool ConflictsWithPrevious(Decl *prev){return false;};

    virtual bool IsVarDecl() { return false; } // jdz: could use typeid/dynamic_cast for these
    virtual bool IsFieldDecl() { return false;}
    virtual bool IsClassDecl() { return false; } //moved to Node
    virtual bool IsInterfaceDecl() { return false; }
    virtual bool IsFnDecl() { return false; } 
    virtual bool IsMethodDecl() { return false; }

    virtual Scope* PrepareScope(){return NULL;}
    int GetOffset(){ return offset; }
    void SetOffset(int off){offset=off;}
};

class ClassDecl : public Decl 
{
  protected:
    List<Decl*> *members;
    List<const char*> *vtable;
    NamedType *extends;
    List<NamedType*> *implements;
    Type *cType;
    List<InterfaceDecl*> *convImp;

  public:
    int fieldCount;
    ClassDecl(Identifier *name, NamedType *extends, 
              List<NamedType*> *implements, List<Decl*> *members);
    void Emit(CodeGenerator * cgen);
    Scope* PrepareScope();
    bool IsClassDecl() { return true; }
    // void CheckImplementAll();
    Type* GetDeclaredType(){ return cType; }
    bool IsChildOf(NamedType* other);
    void MakeVTable();
};

class VarDecl : public Decl 
{
  protected:
    Type *type;
    
  public:
    VarDecl(Identifier *name, Type *type);
    Type *GetDeclaredType() { return type; }
    bool IsVarDecl() { return true; }
    bool IsFieldDecl() { return dynamic_cast<ClassDecl*>(parent) != NULL;}
    void Emit(CodeGenerator * cgen);
};

class InterfaceDecl : public Decl 
{
  protected:
    List<Decl*> *members;
    
  public:
    InterfaceDecl(Identifier *name, List<Decl*> *members);
    void Emit(CodeGenerator * cgen);

    Scope* PrepareScope();

};

class FnDecl : public Decl 
{
  protected:
    List<VarDecl*> *formals;
    Type *returnType;
    Stmt *body;
    
  public:
    FnDecl(Identifier *name, Type *returnType, List<VarDecl*> *formals);
    void SetFunctionBody(Stmt *b);
    void Emit(CodeGenerator * cgen);
    Scope* PrepareScope();
    Type* GetReturnType(){ return returnType;}
    // bool hasReturn(){return returnType!=Type::voidType; }
    int NumArgs(){return formals->NumElements();}
    const char* GetFuncLabel();
    bool IsFnDecl(){return true; }
    bool IsMethodDecl();
};

#endif
