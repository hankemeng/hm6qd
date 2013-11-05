/* File: ast_decl.h
 * ----------------
 * In our parse tree, Decl nodes are used to represent and
 * manage declarations. There are 4 subclasses of the base class,
 * specialized for declarations of variables, functions, classes,
 * and interfaces.
 *
 * pp3: You will need to extend the Decl classes to implement
 * semantic processing including detection of declaration conflicts
 * and managing scoping issues.
 */

#ifndef _H_ast_decl
#define _H_ast_decl

#include "ast.h"
#include "list.h"

class Type;
class NamedType;
class Identifier;
class Stmt;

class Decl : public Node
{
    //  protected:
    //    Identifier *id;
    
public:
    Identifier *id;
    Decl(Identifier *name);
    friend std::ostream& operator<<(std::ostream& out, Decl *d) { return out << d->id; }
    bool ConflictsWithPrevious(Decl *prev);
    bool IsMethodDecl(){return false;}
    bool IsClassDecl(){return false;}
    bool IsInterfaceDecl(){return false;}
};

class VarDecl : public Decl
{
protected:
    Type *type;
    
public:
    VarDecl(Identifier *name, Type *type);
    Type * GetType(){return type;}
    void Check();
};

class ClassDecl : public Decl
{
protected:
    List<Decl*> *members;
    NamedType *extends;
    List<NamedType*> *implements;
    
public:
    ClassDecl(Identifier *name, NamedType *extends,
              List<NamedType*> *implements, List<Decl*> *members);
    void Check();
    bool IsClassDecl(){return true;}
    Scope * PrepareScope();

};

class InterfaceDecl : public Decl
{
protected:
    List<Decl*> *members;
    
public:
    InterfaceDecl(Identifier *name, List<Decl*> *members);
    void Check();
    bool IsInterfaceDecl(){return true;}
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
    bool ConflictsWithPrevious(Decl *prev);
    bool IsMethodDecl();
    bool MatchPrototype(FnDecl *prototype);
    void Check();
};

#endif
