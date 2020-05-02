/******************************************************************************
 *  CVS version:
 *     $Id: symbol.h,v 1.1 2003/05/13 22:21:01 nickie Exp $
 ******************************************************************************
 *
 *  C header file : symbol.h
 *  Project       : PCL Compiler
 *  Version       : 1.0 alpha
 *  Written by    : Nikolaos S. Papaspyrou (nickie@softlab.ntua.gr)
 *  Date          : May 14, 2003
 *  Description   : Generic symbol table in C
 *
 *  Comments: (in Greek iso-8859-7)
 *  ---------
 *  ������ �������� �����������.
 *  ����� ������������ ��������� ��� ��������� �����������.
 *  ������ ����������� ������������ ��� �����������.
 *  ���������� ����������� ����������
 */



#ifndef __SYMBOL_H__
#define __SYMBOL_H__


/* ---------------------------------------------------------------------
   -------------------------- ����� bool -------------------------------
   --------------------------------------------------------------------- */
// #include "common.h"
#include <stdbool.h>
// #include "ast.h"
/*
 *  �� �� �������� include ��� ������������� ��� ��� ���������
 *  ��� C ��� ��������������, �������������� �� �� �� ��������:
 */

#if 0
typedef enum { false=0, true=1 } bool;
#endif


/* ---------------------------------------------------------------------
   ------------ ������� �������� ��� ������ �������� -------------------
   --------------------------------------------------------------------- */

#define START_POSITIVE_OFFSET 8     /* ������ ������ offset ��� �.�.   */
#define START_NEGATIVE_OFFSET 0     /* ������ �������� offset ��� �.�. */


/* ---------------------------------------------------------------------
   --------------- ������� ����� ��� ������ �������� -------------------
   --------------------------------------------------------------------- */

/* ����� ��������� ��� ��� ��������� ��� �������� */

typedef int           RepInteger;         /* ��������                  */
typedef unsigned char RepBoolean;         /* ������� �����             */
typedef char          RepChar;            /* ����������                */
typedef long double   RepReal;            /* �����������               */
typedef const char *  RepString;          /* �������������             */


/* ����� ��������� ��� ������������� ����������� */

typedef struct Type_tag * MyType;

typedef enum {                               /***** �� ����� ��� ����� ****/
       TYPE_VOID,                        /* ����� ����� ������������� */
       TYPE_INTEGER,                     /* ��������                  */
       TYPE_BOOLEAN,                     /* ������� �����             */
       TYPE_BYTE,           /*��� byte*/
       TYPE_STRING,
       TYPE_ARRAY_INTEGER,
       TYPE_ARRAY_BYTE,
       TYPE_CHAR,                        /* ����������                */
       TYPE_REAL,                        /* �����������               */
       TYPE_ARRAY,                       /* ������� ������� ��������  */
       TYPE_IARRAY,                      /* ������� �������� �������� */
       TYPE_POINTER                      /* �������                   */
    } Kind;

typedef enum {                               /* ��������� ����������  */
             PARDEF_COMPLETE,                    /* ������ �������     */
             PARDEF_DEFINE,                      /* �� ���� �������    */
             PARDEF_CHECK                        /* �� ���� �������    */
         } Pardef;

struct Type_tag {
    Kind kind;
    MyType           refType;              /* ����� ��������            */
    RepInteger     size;                 /* �������, �� ����� ������� */
    unsigned int   refCount;             /* �������� ��������         */
};


/*-----------Anti gia to ast.h------*/
typedef enum {
  DATATYPE, R_TYPE, TYPE, VARDEF, ASSIGN, FPARDEF, FUNCDEF, WHILE, IF, SEQ, IFTHENELSE,
  STRING, INTEGER, CHAR, FUNCALL, RETURN, ID, CONST, PLUS, MINUS, TIMES, DIV, MOD,
  PROGRAM, EQUAL, NOTEQUAL, LESS, GREAT, LESSEQUAL, GREATEQUAL, AND, OR, NOT,
  WRITEBYTE, WRITEINTEGER, WRITECHAR, WRITESTRING
  
} kind;

typedef struct node {
  kind k;
  char* id;
  char* constchar;
  int num;
  struct node *left, *middle, *right;
  MyType type;
  int nesting_diff;  // ID and LET nodes
  int offset;        // ID and LET nodes
  int num_vars;      // BLOCK node
} *ast;
/*-----------Anti gia to ast.h------*/







/* ����� �������� ��� ������ �������� */

typedef enum {
   ENTRY_VARIABLE,                       /* ����������                 */
   ENTRY_CONSTANT,                       /* ��������                   */
   ENTRY_FUNCTION,                       /* �����������                */
   ENTRY_PARAMETER,                      /* ���������� �����������     */
   ENTRY_TEMPORARY                       /* ���������� ����������      */
} EntryType;


/* ����� ���������� ���������� */

typedef enum {
   PASS_BY_VALUE,                        /* ���' ����                  */
   PASS_BY_REFERENCE                     /* ���' �������               */
} PassMode;


/* ����� �������� ���� ������ �������� */

typedef struct SymbolEntry_tag SymbolEntry;

struct SymbolEntry_tag {
   const char   * id;                 /* ����� ��������������          */
   EntryType      entryType;          /* ����� ��� ��������            */
   unsigned int   nestingLevel;       /* ����� �����������             */
   unsigned int   hashValue;          /* ���� ���������������          */
   SymbolEntry  * nextHash;           /* ������� ������� ���� �.�.     */
   SymbolEntry  * nextInScope;        /* ������� ������� ���� �������� */

   union {                            /* ������� �� ��� ���� ��������: */

      struct {                                /******* ��������� *******/
         MyType          type;                  /* �����                 */
         int           offset;                /* Offset ��� �.�.       */
      } eVariable;

      struct {                                /******** ������� ********/
         MyType          type;                  /* �����                 */
         union {                              /* ����                  */
            RepInteger vInteger;              /*    �������            */
            RepBoolean vBoolean;              /*    ������             */
            RepChar    vChar;                 /*    ����������         */
            RepReal    vReal;                 /*    ����������         */
            RepString  vString;               /*    ������������       */
         } value;
      } eConstant;

      

      struct {                                /******* ��������� *******/
         //bool          isForward;             /* ������ forward        */
         SymbolEntry * firstArgument;         /* ����� ����������      */
         SymbolEntry * lastArgument;          /* ��������� ����������  */
         MyType          resultType;            /* ����� �������������   */
         // enum {                               /* ��������� ����������  */
         //     PARDEF_COMPLETE,                    /* ������ �������     */
         //     PARDEF_DEFINE,                      /* �� ���� �������    */
         //     PARDEF_CHECK                        /* �� ���� �������    */
         // } pardef;
         Pardef pardef;
         ast  funcNode;
         int           firstQuad;             /* ������ �������        */
      } eFunction;

      struct {                                /****** ���������� *******/
         MyType          type;                  /* �����                 */
         int           offset;                /* Offset ��� �.�.       */
         PassMode      mode;                  /* ������ ����������     */
         SymbolEntry * next;                  /* ������� ����������    */
      } eParameter;

      struct {                                /** ��������� ��������� **/
         MyType          type;                  /* �����                 */
         int           offset;                /* Offset ��� �.�.       */
         int           number;
      } eTemporary;

   } u;                               /* ����� ��� union               */
};


/* ����� ������� �������� ��� ���������� ���� ���� �������� */

typedef struct Scope_tag Scope;

struct Scope_tag {
    unsigned int   nestingLevel;             /* ����� �����������      */
    unsigned int   negOffset;                /* ������ �������� offset */
    Scope        * parent;                   /* ������������ ��������  */
    SymbolEntry  * entries;                  /* ������� ��� ���������  */
};


/* ����� ���������� ���� ������ �������� */

typedef enum {
    LOOKUP_CURRENT_SCOPE,
    LOOKUP_ALL_SCOPES
} LookupType;


/* ---------------------------------------------------------------------
   ------------- ��������� ���������� ��� ������ �������� --------------
   --------------------------------------------------------------------- */

extern Scope        * currentScope;       /* �������� ��������         */
extern unsigned int   quadNext;           /* ������� �������� �������� */
extern unsigned int   tempNumber;         /* �������� ��� temporaries  */

extern const MyType typeVoid;
extern const MyType typeInteger;
extern const MyType typeBoolean;
extern const MyType typeString;
extern const MyType typeChar;
extern const MyType typeReal;
extern const MyType typeByte;
extern const MyType typeArrayInteger;
extern const MyType typeArrayByte;


/* ---------------------------------------------------------------------
   ------ ��������� ��� ����������� ��������� ��� ������ �������� ------
   --------------------------------------------------------------------- */

void          initSymbolTable    (unsigned int size);
void          destroySymbolTable (void);

void          openScope          (void);
void          closeScope         (void);

SymbolEntry * newVariable        (const char * name, MyType type);
SymbolEntry * newConstant        (const char * name, MyType type, ...);
SymbolEntry * newFunction        (const char * name, MyType type, ast node);
SymbolEntry * newParameter       (const char * name, MyType type,
                                  PassMode mode, SymbolEntry * f);
SymbolEntry * newTemporary       (MyType type);

// void          forwardFunction    (SymbolEntry * f);
void          endFunctionHeader  (SymbolEntry * f, MyType type);
void          destroyEntry       (SymbolEntry * e);
SymbolEntry * lookupEntry        (const char * name, LookupType type,
                                  bool err);

MyType          typeArray          (RepInteger size, MyType refType);
MyType          typeIArray         (MyType refType);
MyType          typePointer        (MyType refType);
void          destroyType        (MyType type);
unsigned int  sizeOfType         (MyType type);
bool          equalType          (MyType type1, MyType type2);
void          printType          (MyType type);
void          printMode          (PassMode mode);


#endif
