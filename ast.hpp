#ifndef __AST_H__
#define __AST_H__

#include "symbol.hpp"
#include "stack.hpp"


ast ast_funcdef(char* id,ast l,MyType type, ast m, ast r);
ast ast_fpar_def(char* id, int n, MyType type);
ast ast_string(kind k,char* s);
//ast ast_local_def(ast l);
ast ast_assign(ast l, ast m);
ast ast_var_def(char*id, MyType type, int n);
//ast ast_compound_stmt(ast l);
// ast ast_call(ast l);
ast ast_write(int n, ast l, char* c);
ast ast_if(ast l,ast m);
ast ast_ifelse(ast l,ast m,ast r);
ast ast_while(ast l ,ast m);
ast ast_return(ast l);
ast ast_program(ast l);
ast ast_func_call(char* id, ast m);
ast ast_integer(int n);
ast ast_char(char c);

ast ast_id (kind k,char* id, ast l);
ast ast_const (int n, MyType t);
ast ast_op (MyType type, ast l, kind op, ast r);
ast ast_seq (ast l, ast m);
// ast ast_read()

int ast_run (ast t);
void ast_sem( ast t);
SymbolEntry * lookup(char* c);
SymbolEntry * insertFunction(ast n);
SymbolEntry * insertParameter(char* c, MyType t,PassMode m);
SymbolEntry * insertVariable(char* c, MyType t);

void llvm_compile_and_dump (ast t);

#endif
