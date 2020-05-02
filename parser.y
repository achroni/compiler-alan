%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.hpp"
// #include "symbol.hpp"
#include "stack.hpp"

extern int yylex();
void yyerror (const char *msg);

extern int linenumber;
ast t;
%}

%union{
  ast a;
  char c;
  int n;
  MyType t;
  char* str;
}

%token T_byte "byte"
%token T_else "else"
%token T_false "false"
%token T_if "if"
%token T_int "int"
%token T_proc "proc"
%token T_reference "reference"
%token T_return "return"
%token T_while "while"
%token T_true "true"


%token<n> T_integer 
%token<str> T_id
%token<c> T_char
 
%token<str> T_str 

%token T_not_equal "!=" 
%token T_less_eq "<="
%token T_greater_eq ">=" 
%token T_equal "=="

%left '|'
%left '&'
%nonassoc "==" "!=" '<' '>' "<=" ">="  
   
%nonassoc "then"
%nonassoc "else"

%left '+' '-'
%left '*' '/' '%'
%left UMINUS UPLUS UNEG
   
%type<a> program
%type<a> func_def
%type<a> local_def_star
%type<a> fpar_list
%type<a> fpar_def_star
%type<a> fpar_def

%type<t> data_type
%type<t> type
%type<t> r_type

%type<a> local_def
%type<a> var_def
%type<a> stmt
%type<a> compound_stmt
%type<a> stmt_star
%type<a> func_call
%type<a> expr_list
%type<a> expr_star
%type<a> expr
%type<a> l_value
%type<a> cond


%%

program:
  func_def  { t = $$ = ast_program($1);
              /*t = $$ = $1;*/}
;

func_def:
  T_id '(' ')' ':' r_type local_def_star compound_stmt { $$ = ast_funcdef($1,NULL,$5,$6,$7);}
| T_id '(' fpar_list ')' ':' r_type local_def_star compound_stmt { $$ = ast_funcdef($1,$3,$6,$7,$8);} 
;

local_def_star:
  /*nothing*/   {$$ = NULL;}
| local_def local_def_star  { $$ = ast_seq($1,$2);}
;

fpar_list:
  fpar_def fpar_def_star {$$ = ast_seq($1,$2);}
;

fpar_def_star:
  /*nothing*/   {$$ = NULL;}
| ',' fpar_def fpar_def_star  {$$ = ast_seq($2,$3);}

fpar_def: 
  T_id ':' "reference" type {$$ = ast_fpar_def($1,1,$4);}
| T_id ':' data_type {$$ = ast_fpar_def($1,0,$3);}
;



data_type:
  "int"   {$$ = typeInteger;}
| "byte"  {$$ = typeByte;}
;

type:
  "int"   {$$ = typeInteger;}
| "byte"  {$$ = typeByte;}
| "int" '[' ']' {$$ = typeArrayInteger;/*Den einai kai sigouro!*/}
| "byte" '[' ']' {$$ = typeArrayByte;}
;

r_type:
  "int"   {$$ = typeInteger;}
| "byte"  {$$ = typeByte;}
| "proc"  {$$ = typeVoid;}
;

local_def:
  func_def  {$$ = $1; /*$$ = ast_local_def($1);*/}
| var_def   {$$ = $1; /*$$ = ast_local_def($1);*/}
;

var_def:
  T_id ':' data_type ';'  {$$ = ast_var_def($1,$3,0);}
| T_id ':' "int" '[' T_integer ']' ';'  {$$ = ast_var_def($1,typeArrayInteger,$5);}
| T_id ':' "byte" '[' T_integer ']' ';'  {$$ = ast_var_def($1,typeArrayByte,$5);}
;



stmt:
  ';'   {$$ = NULL;}
| l_value '=' expr ';'            {$$ = ast_assign($1,$3);}
| compound_stmt                   {$$ = $1;   /*$$ = ast_compound_stmt($1); */}
| func_call ';'                   {$$ = $1; /*$$ = ast_call($1);*/}
| "if" '(' cond ')' stmt  %prec "then" {$$ = ast_if($3,$5);} 
| "if" '(' cond ')' stmt "else" stmt  {$$ = ast_ifelse($3,$5,$7);}
| "while" '(' cond ')' stmt       {$$ = ast_while($3,$5);}
| "return" expr ';'               {$$ = ast_return($2);}
| "return" ';'                     {$$ = ast_return(NULL);}
;

compound_stmt:
  '{' stmt_star '}'   {$$ = $2;/*nooo BLOCK*/}
;

stmt_star:
  /*nothing*/       {$$ = NULL;}
| stmt stmt_star    {$$ = ast_seq($1,$2);}
;

func_call:
  T_id '(' ')'            {$$ = ast_func_call($1,NULL);}
| T_id '(' expr_list ')'  {$$ = ast_func_call($1,$3);}
;

expr_list:
  expr expr_star   {$$ = ast_seq($1,$2);}
;

expr_star:
  /*nothing*/           {$$ = NULL;}
| ',' expr expr_star   {$$ = ast_seq($2,$3);}
;

expr:
  T_integer             {$$ = ast_integer($1);}
| T_char                {$$ = ast_char($1);}
| l_value               {$$ = $1;}
| '(' expr ')'          {$$ = $2;}
| func_call             {$$ = $1;}
| '+' expr        {$$ = $2; /*MPOREI NA EINAI LA8OS*/} %prec UPLUS 
| '-' expr        {$$ = ast_op(NULL,ast_const(0,typeInteger), MINUS, $2);} %prec UMINUS
| expr '+' expr     {$$ = ast_op(NULL,$1, PLUS, $3);}
| expr '-' expr     {$$ = ast_op(NULL ,$1, MINUS, $3);}
| expr '*' expr     {$$ = ast_op(NULL , $1, TIMES, $3);}
| expr '/' expr     {$$ = ast_op(NULL , $1, DIV, $3);}
| expr '%' expr     {$$ = ast_op(NULL , $1, MOD, $3);}
;

l_value:
  T_id              {$$ = ast_id(ID,$1,NULL);}
| T_id '[' expr ']' {$$ = ast_id(ID,$1,$3);}
| T_str              {$$ = ast_string(STRING,$1);}
;

cond:
  "true"          {$$ = ast_const(1,typeBoolean);}          
| "false"         {$$ = ast_const(0,typeBoolean);} 
| '(' cond ')'    {$$ = $2;}
| '!' cond        {$$ = ast_op(typeBoolean, $2, NOT, NULL);} %prec UNEG 
| expr "==" expr  {$$ = ast_op(typeBoolean, $1,EQUAL,$3);}
| expr "!=" expr  {$$ = ast_op(typeBoolean, $1,NOTEQUAL,$3);}
| expr '>' expr   {$$ = ast_op(typeBoolean, $1,GREAT,$3);}
| expr '<' expr   {$$ = ast_op(typeBoolean, $1,LESS,$3);}
| expr "<=" expr  {$$ = ast_op(typeBoolean, $1,LESSEQUAL,$3);} 
| expr ">=" expr  {$$ = ast_op(typeBoolean, $1,GREATEQUAL,$3);}
| cond '&' cond   {$$ = ast_op(typeBoolean, $1,AND,$3);}
| cond '|' cond   {$$ = ast_op(typeBoolean, $1,OR,$3);}
;

%%

void yyerror (const char *msg) {
  fprintf(stderr, "Alan error: %s\n", msg);
  fprintf(stderr, "Aborting, I've had enough with line %d...\n",
          linenumber);
  exit(1);
}

Stack rootFunc;

int main() {
  if (yyparse()) return 1;
 // printf("Compilation was successful.\n");
  initSymbolTable(997);
   // rootFunc declared in stack.h
  stack_init(rootFunc);
  ast_sem(t);
  destroySymbolTable();
  //printf("------------- Sem DONE ------------\n");
  //ast_run(t);
  //printf("------------- Run DONE ------------\n");
  llvm_compile_and_dump(t);
  
  return 0;
}
   
