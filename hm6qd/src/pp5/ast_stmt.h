/* File: ast_stmt.h
 * ----------------
 * The Stmt class and its subclasses are used to represent
 * statements in the parse tree.  For each statment in the
 * language (for, if, return, etc.) there is a corresponding
 * node class for that construct. 
 *
 * pp5: You will need to extend the Stmt classes to implement
 * code generation for statements.
 */


#ifndef _H_ast_stmt
#define _H_ast_stmt

#include "list.h"
#include "ast.h"
// #include "scope.h"

class Decl;
class VarDecl;
class Expr;
  
class Program : public Node
{
  protected:
     List<Decl*> *decls;
     
  public:
     Program(List<Decl*> *declList);
     void Check();
     Scope* PrepareScope();

     void Emit(); //need virtual?
     CodeGenerator * cgen;
};

class Stmt : public Node
{
  public:
    Stmt() : Node() {}
    Stmt(yyltype loc) : Node(loc) {}
    virtual Scope* PrepareScope(){
      // printf("/****** PrepareScope to be implemented! ****/\n");
      return NULL;
    };

};

class StmtBlock : public Stmt 
{
  protected:
    List<VarDecl*> *decls;
    List<Stmt*> *stmts;
    
  public:
    StmtBlock(List<VarDecl*> *variableDeclarations, List<Stmt*> *statements);
    void Emit(CodeGenerator * cgen);
    Scope* PrepareScope();
};

  
class ConditionalStmt : public Stmt
{
  protected:
    Expr *test;
    Stmt *body;
  
  public:
    ConditionalStmt(Expr *testExpr, Stmt *body);
    void Emit(CodeGenerator * cgen);
};

class LoopStmt : public ConditionalStmt 
{
  public:
    char* loopLabel;
    char* endLabel;
    LoopStmt(Expr *testExpr, Stmt *body)
            : ConditionalStmt(testExpr, body) {}
    bool IsLoopStmt() { return true; }
};

class ForStmt : public LoopStmt 
{
  protected:
    Expr *init, *step;
  
  public:
    ForStmt(Expr *init, Expr *test, Expr *step, Stmt *body);
    void Emit(CodeGenerator * cgen);
};

class WhileStmt : public LoopStmt 
{
  public:
    WhileStmt(Expr *test, Stmt *body) : LoopStmt(test, body) {}
    void Emit(CodeGenerator * cgen);
};

class IfStmt : public ConditionalStmt 
{
  protected:
    Stmt *elseBody;
  
  public:
    IfStmt(Expr *test, Stmt *thenBody, Stmt *elseBody);
    void Emit(CodeGenerator * cgen);
};

class BreakStmt : public Stmt 
{
  public:
    BreakStmt(yyltype loc) : Stmt(loc) {}
    void Emit(CodeGenerator * cgen);
};

class ReturnStmt : public Stmt  
{
  protected:
    Expr *expr;
  
  public:
    ReturnStmt(yyltype loc, Expr *expr);
    void Emit(CodeGenerator * cgen);
};

class PrintStmt : public Stmt
{
  protected:
    List<Expr*> *args;
    
  public:
    PrintStmt(List<Expr*> *arguments);
    void Emit(CodeGenerator * cgen);
};


class IntConstant;

class Case : public Node
{
protected:
    IntConstant *value;
    List<Stmt*> *stmts;
    
public:
    Case(IntConstant *v, List<Stmt*> *stmts);
    //    const char *GetPrintNameForNode() { return value ? "Case" :"Default"; }
    //    void PrintChildren(int indentLevel);
};

class SwitchStmt : public Stmt
{
protected:
    Expr *expr;
    List<Case*> *cases;
    
public:
    SwitchStmt(Expr *e, List<Case*> *cases);
    //    const char *GetPrintNameForNode() { return "SwitchStmt"; }
    //    void PrintChildren(int indentLevel);
};


#endif
