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
 *  Εθνικό Μετσόβιο Πολυτεχνείο.
 *  Σχολή Ηλεκτρολόγων Μηχανικών και Μηχανικών Υπολογιστών.
 *  Τομέας Τεχνολογίας Πληροφορικής και Υπολογιστών.
 *  Εργαστήριο Τεχνολογίας Λογισμικού
 */



#ifndef __SYMBOL_H__
#define __SYMBOL_H__


/* ---------------------------------------------------------------------
   -------------------------- Τύπος bool -------------------------------
   --------------------------------------------------------------------- */
// #include "common.h"
#include <stdbool.h>
// #include "ast.h"
/*
 *  Αν το παραπάνω include δεν υποστηρίζεται από την υλοποίηση
 *  της C που χρησιμοποιείτε, αντικαταστήστε το με το ακόλουθο:
 */

#if 0
typedef enum { false=0, true=1 } bool;
#endif


/* ---------------------------------------------------------------------
   ------------ Ορισμός σταθερών του πίνακα συμβόλων -------------------
   --------------------------------------------------------------------- */

#define START_POSITIVE_OFFSET 8     /* Αρχικό θετικό offset στο Ε.Δ.   */
#define START_NEGATIVE_OFFSET 0     /* Αρχικό αρνητικό offset στο Ε.Δ. */


/* ---------------------------------------------------------------------
   --------------- Ορισμός τύπων του πίνακα συμβόλων -------------------
   --------------------------------------------------------------------- */

/* Τύποι δεδομένων για την υλοποίηση των σταθερών */

typedef int           RepInteger;         /* Ακέραιες                  */
typedef unsigned char RepBoolean;         /* Λογικές τιμές             */
typedef char          RepChar;            /* Χαρακτήρες                */
typedef long double   RepReal;            /* Πραγματικές               */
typedef const char *  RepString;          /* Συμβολοσειρές             */


/* Τύποι δεδομένων και αποτελέσματος συναρτήσεων */

typedef struct Type_tag * MyType;

typedef enum {                               /***** Το είδος του τύπου ****/
       TYPE_VOID,                        /* Κενός τύπος αποτελέσματος */
       TYPE_INTEGER,                     /* Ακέραιοι                  */
       TYPE_BOOLEAN,                     /* Λογικές τιμές             */
       TYPE_BYTE,           /*Για byte*/
       TYPE_STRING,
       TYPE_ARRAY_INTEGER,
       TYPE_ARRAY_BYTE,
       TYPE_CHAR,                        /* Χαρακτήρες                */
       TYPE_REAL,                        /* Πραγματικοί               */
       TYPE_ARRAY,                       /* Πίνακες γνωστού μεγέθους  */
       TYPE_IARRAY,                      /* Πίνακες άγνωστου μεγέθους */
       TYPE_POINTER                      /* Δείκτες                   */
    } Kind;

typedef enum {                               /* Κατάσταση παραμέτρων  */
             PARDEF_COMPLETE,                    /* Πλήρης ορισμός     */
             PARDEF_DEFINE,                      /* Εν μέσω ορισμού    */
             PARDEF_CHECK                        /* Εν μέσω ελέγχου    */
         } Pardef;

struct Type_tag {
    Kind kind;
    MyType           refType;              /* Τύπος αναφοράς            */
    RepInteger     size;                 /* Μέγεθος, αν είναι πίνακας */
    unsigned int   refCount;             /* Μετρητής αναφορών         */
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







/* Τύποι εγγραφών του πίνακα συμβόλων */

typedef enum {
   ENTRY_VARIABLE,                       /* Μεταβλητές                 */
   ENTRY_CONSTANT,                       /* Σταθερές                   */
   ENTRY_FUNCTION,                       /* Συναρτήσεις                */
   ENTRY_PARAMETER,                      /* Παράμετροι συναρτήσεων     */
   ENTRY_TEMPORARY                       /* Προσωρινές μεταβλητές      */
} EntryType;


/* Τύποι περάσματος παραμετρων */

typedef enum {
   PASS_BY_VALUE,                        /* Κατ' αξία                  */
   PASS_BY_REFERENCE                     /* Κατ' αναφορά               */
} PassMode;


/* Τύπος εγγραφής στον πίνακα συμβόλων */

typedef struct SymbolEntry_tag SymbolEntry;

struct SymbolEntry_tag {
   const char   * id;                 /* Ονομα αναγνωριστικού          */
   EntryType      entryType;          /* Τύπος της εγγραφής            */
   unsigned int   nestingLevel;       /* Βάθος φωλιάσματος             */
   unsigned int   hashValue;          /* Τιμή κατακερματισμού          */
   SymbolEntry  * nextHash;           /* Επόμενη εγγραφή στον Π.Κ.     */
   SymbolEntry  * nextInScope;        /* Επόμενη εγγραφή στην εμβέλεια */

   union {                            /* Ανάλογα με τον τύπο εγγραφής: */

      struct {                                /******* Μεταβλητή *******/
         MyType          type;                  /* Τύπος                 */
         int           offset;                /* Offset στο Ε.Δ.       */
      } eVariable;

      struct {                                /******** Σταθερά ********/
         MyType          type;                  /* Τύπος                 */
         union {                              /* Τιμή                  */
            RepInteger vInteger;              /*    ακέραια            */
            RepBoolean vBoolean;              /*    λογική             */
            RepChar    vChar;                 /*    χαρακτήρας         */
            RepReal    vReal;                 /*    πραγματική         */
            RepString  vString;               /*    συμβολοσειρά       */
         } value;
      } eConstant;

      

      struct {                                /******* Συνάρτηση *******/
         //bool          isForward;             /* Δήλωση forward        */
         SymbolEntry * firstArgument;         /* Λίστα παραμέτρων      */
         SymbolEntry * lastArgument;          /* Τελευταία παράμετρος  */
         MyType          resultType;            /* Τύπος αποτελέσματος   */
         // enum {                               /* Κατάσταση παραμέτρων  */
         //     PARDEF_COMPLETE,                    /* Πλήρης ορισμός     */
         //     PARDEF_DEFINE,                      /* Εν μέσω ορισμού    */
         //     PARDEF_CHECK                        /* Εν μέσω ελέγχου    */
         // } pardef;
         Pardef pardef;
         ast  funcNode;
         int           firstQuad;             /* Αρχική τετράδα        */
      } eFunction;

      struct {                                /****** Παράμετρος *******/
         MyType          type;                  /* Τύπος                 */
         int           offset;                /* Offset στο Ε.Δ.       */
         PassMode      mode;                  /* Τρόπος περάσματος     */
         SymbolEntry * next;                  /* Επόμενη παράμετρος    */
      } eParameter;

      struct {                                /** Προσωρινή μεταβλητή **/
         MyType          type;                  /* Τύπος                 */
         int           offset;                /* Offset στο Ε.Δ.       */
         int           number;
      } eTemporary;

   } u;                               /* Τέλος του union               */
};


/* Τύπος συνόλου εγγραφών που βρίσκονται στην ίδια εμβέλεια */

typedef struct Scope_tag Scope;

struct Scope_tag {
    unsigned int   nestingLevel;             /* Βάθος φωλιάσματος      */
    unsigned int   negOffset;                /* Τρέχον αρνητικό offset */
    Scope        * parent;                   /* Περιβάλλουσα εμβέλεια  */
    SymbolEntry  * entries;                  /* Σύμβολα της εμβέλειας  */
};


/* Τύπος αναζήτησης στον πίνακα συμβόλων */

typedef enum {
    LOOKUP_CURRENT_SCOPE,
    LOOKUP_ALL_SCOPES
} LookupType;


/* ---------------------------------------------------------------------
   ------------- Καθολικές μεταβλητές του πίνακα συμβόλων --------------
   --------------------------------------------------------------------- */

extern Scope        * currentScope;       /* Τρέχουσα εμβέλεια         */
extern unsigned int   quadNext;           /* Αριθμός επόμενης τετράδας */
extern unsigned int   tempNumber;         /* Αρίθμηση των temporaries  */

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
   ------ Πρωτότυπα των συναρτήσεων χειρισμού του πίνακα συμβολών ------
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
