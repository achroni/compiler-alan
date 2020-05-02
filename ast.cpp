#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #include "symbol.h"
#include "ast.hpp"
#include "error.hpp"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/Instructions.h>
#if defined(LLVM_VERSION_MAJOR) && LLVM_VERSION_MAJOR >= 4
#include <llvm/Transforms/Scalar/GVN.h>
#endif

#include <stack> 
#include <tuple>
#include <iostream>
using namespace llvm;




static ast ast_make (kind k, char* c, char* constchar,MyType type, int n, ast l, ast m, ast r) {
  ast p;
  if ((p = (ast) malloc(sizeof(struct node))) == NULL)
    exit(1);
  p->k = k;
  p->id = (char*)malloc(sizeof(char)*(strlen(c)+1));
  strcpy(p->id,c);
  p->constchar = (char*)malloc(sizeof(char)*(strlen(constchar)+1));
  strcpy(p->constchar,constchar);
  p->type = type;
  p->num = n;
  p->left = l;
  p->middle = m;
  p->right = r;
  return p;
}

ast ast_program(ast left){
  // printf("-------- MAKE PROGRAM ---------------\n");
  return ast_make(PROGRAM,"","",NULL,0,left,NULL,NULL);
}

ast ast_funcdef(char* id, ast left, MyType type, ast middle, ast right){
  return ast_make(FUNCDEF,id,"",type,0,left,middle,right);
}

ast ast_fpar_def(char*id, int num, MyType type){
  return ast_make(FPARDEF,id,"",type,num,NULL,NULL,NULL);
}

ast ast_var_def(char*id, MyType type, int n){
  return ast_make(VARDEF,id,"",type,n,NULL,NULL,NULL);
}
  
ast ast_assign(ast lvalue, ast rexpr){
  ast temp = lvalue->left;      //an einai pinakas to lvalue
  return ast_make(ASSIGN,lvalue->id,"",NULL,0,temp,rexpr,NULL);
}


ast ast_if(ast left,ast middle){
  return ast_make(IF,"","",NULL,0,left,middle,NULL);
}

ast ast_ifelse(ast left, ast middle, ast right){
  return ast_make(IFTHENELSE,"","",NULL,0,left,middle,right);
}

ast ast_while(ast left,ast middle){
  return ast_make(WHILE,"","",NULL,0,left,middle,NULL);
}

ast ast_return(ast left){
  return ast_make(RETURN,"","",NULL,0,left,NULL,NULL);
}

ast ast_func_call(char* id, ast left){
  return ast_make(FUNCALL,id,"",NULL,0,left,NULL,NULL);
}

ast ast_integer(int num){
  return ast_make(INTEGER,"","",typeInteger,num,NULL,NULL,NULL);
}

ast ast_char(char c){
  return ast_make(CHAR,"","",typeByte,(int)c,NULL,NULL,NULL);
}


ast ast_id (kind k,char* id, ast l){
  return ast_make(k, id,"", NULL,0, l, NULL, NULL); //k=ID always
}

ast ast_string(kind k,char* s){
  return ast_make(k,"",s,typeString,0,NULL,NULL,NULL);  //k=STRING always
}

ast ast_const (int n, MyType type) {
  return ast_make(CONST, "","", type,n, NULL, NULL, NULL);
}


ast ast_op (MyType type, ast l, kind op, ast m) {
  if (type != NULL)
    return ast_make(op, "", "",type, 0, l, m, NULL);
  else
    return ast_make(op,"","",l->type,0,l,m,NULL);
}

ast ast_seq (ast l, ast m) {
  // if (m == NULL) return l;
  return ast_make(SEQ, "","",NULL, 0, l, m, NULL);
}

// Global LLVM variables related to the LLVM suite.
static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;
static std::unique_ptr<legacy::FunctionPassManager> TheFPM;
static std::map<std::string, AllocaInst *> NamedValues;

// Global LLVM variables related to the generated code.
static GlobalVariable *TheVars;
static GlobalVariable *TheNL;

static Function *TheWriteByte;
static Function *TheWriteInteger;
static Function *TheWriteChar;
static Function *TheWriteString;
static Function *TheReadChar;
static Function *TheReadInteger;
static Function *TheReadByte;
static Function *TheReadString; 
static Function *TheStrlen;
static Function *TheStrcmp;
static Function *TheStrcpy;
static Function *TheStrcat;
static Function *TheShrink;
static Function *TheExtend;



// Useful LLVM types.
static Type * i1 = IntegerType::get(TheContext, 1); //dikia mou
static Type * i8 = IntegerType::get(TheContext, 8);
// static Type * i32 = IntegerType::get(TheContext, 16);
static Type * i16 = IntegerType::get(TheContext, 16);

// Useful LLVM helper functions.
inline ConstantInt* c1(int c) {     //dikia mas!!!
  return ConstantInt::get(TheContext, APInt(1, c, true));
}
inline ConstantInt* c8(char c) {
  return ConstantInt::get(TheContext, APInt(8, c, true));
}
// inline ConstantInt* c32(int n) {
//   return ConstantInt::get(TheContext, APInt(16, n, true));
// }
inline ConstantInt* c16(int n) {
  return ConstantInt::get(TheContext, APInt(16, n, true));
}


std::vector<Type *> EmptyVectorType(){
  std::vector<Type *> tempVect;
  tempVect.clear();
  return tempVect;
}

std::stack<std::vector<std::tuple<std::string,Type*,Value*>>> StackOfVarNames;
std::vector<std::tuple<std::string,Type*,Value*>> VarNames;
std::vector<Type*> GlobalVarTypes;
std::vector<std::string> GlobalVarNames;
std::vector<Type*> MyGlobalVarTypes;
std::vector<std::string> MyGlobalVarNames;
std::stack<std::vector<Type*>> StackOfGlobalVarTypes;
std::stack<std::vector<std::string>> StackOfGlobalVarNames;

static std::map<std::string, std::vector<std::string>> GlobalsOfFunction;


std::vector <Type*> argvType;

char** argvName;
int argvIterator =0;

BasicBlock* bblockGlobal;
bool isReturn;

Type * getFunctionTypeFromMyType(MyType t){
  Type * retType;
  if (equalType(t ,typeInteger))
    retType = i16;
  else if (equalType(t, typeByte))
    retType = i8;
  else if (equalType(t, typeVoid))
    retType = Type::getVoidTy(TheContext);
  else
    printf("Something Wrong FUNCDEF compile!\n");
  return retType;
}

// Convert MyType to Type
Type * getTypeFromMyType(MyType m, int n, int isReference){
  Type * t;
  if (equalType(m ,typeInteger))
  {
  	isReference ? t = PointerType::getUnqual(i16) : t = i16;
  }
  else if (equalType(m, typeByte))
  {
  	isReference ? t = PointerType::getUnqual(i8) : t = i8;
    // t=i8;
  }
  else if (equalType(m,typeArrayInteger))
  {
  	isReference ? t = PointerType::getUnqual(ArrayType::get(i16,n)) : t = ArrayType::get(i16,n);
  	// isReference ? t = PointerType::getUnqual(PointerType()) : t = ArrayType::get(i16,n);
    // t= ArrayType::get(i16,n);
  }
  else if (equalType(m,typeArrayByte))
  {
  	isReference ? t = PointerType::getUnqual(ArrayType::get(i8,n)) : t = ArrayType::get(i8,n);
  	// isReference ? t =  PointerType::getUnqual(PointerType())  : t = ArrayType::get(i8,n);
   // t= ArrayType::get(i8,n);
  }
  else{
    error("Cannot convert MyType to Type");
  }
  return t;
}


// Initial Value of Local Vars
Value * getInitLocalVar(MyType m, int n){
  Value * t;

  if (m == typeInteger)
  {
    t = c16(0);
  }
  else if (m == typeByte)
  {
    t=c8('\0');
  }
  else if (m == typeArrayInteger)
  {
    std::vector<Constant *> my_array(n,c16(0));
    t = ConstantArray::get(ArrayType::get(i16,n), my_array);
  }
  else if (m == typeArrayByte)
  {
    std::vector<Constant *> my_array(n,c8('\0'));
    t = ConstantArray::get(ArrayType::get(i8,n), my_array);
  }
  else{
    error("Cannot convert MyType to Type");
  }
  return t;
}


Function *getFunction(std::string Name) {
  // First, see if the function has already been added to the current module.
  if (auto *F = TheModule->getFunction(Name))
    return F;

  // If no existing prototype exists, return null.
  return nullptr;
}



/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static AllocaInst *CreateEntryBlockAlloca(Function *TheFunction,
                                          const std::string &VarName,
                                          Type *type) {
  IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                   TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(type, nullptr, VarName);
}


Value * ast_compile (ast t) {
  if (t == nullptr) return nullptr;
  switch (t->k) {

    case PROGRAM:{
      Value* l = ast_compile(t->left);
      return l;
    }

    case ASSIGN: {
      Value *Variable = NamedValues[t->id];
      if (!Variable)
        printf("Unknown Variabe Name\n");
      if(t->left == NULL){ //not array
      	Value *Val = ast_compile(t->middle);
      	Builder.CreateStore(Val, Variable);	
      	return Val;
      }
      auto zero = llvm::ConstantInt::get(TheContext, llvm::APInt(16, 0, true));
	  Value * Index = ast_compile(t->left);
	
	  auto ptr = llvm::GetElementPtrInst::CreateInBounds(NamedValues[t->id], { zero, Index }, "", bblockGlobal);
	  Value *Val = ast_compile(t->middle);
	  auto store = new llvm::StoreInst(Val, ptr, false, bblockGlobal);
     
      // Builder.CreateStore(Val,Variable);
      return Val;
    }

    case IF: {
      Value *v = ast_compile(t->left);
       if (!v)
        return nullptr;

      Value *cond = Builder.CreateICmpNE(v, c1(0), "if_cond");

      Function *TheFunction = Builder.GetInsertBlock()->getParent();
      
      BasicBlock *InsideBB =
          BasicBlock::Create(TheContext, "then", TheFunction);
      BasicBlock *AfterBB =
          BasicBlock::Create(TheContext, "endif", TheFunction);
      Builder.CreateCondBr(cond, InsideBB, AfterBB);

      //Emit then value
      Builder.SetInsertPoint(InsideBB);
      bblockGlobal = InsideBB;
      isReturn = false;
      Value* m = ast_compile(t->middle);
      if(isReturn == false)
      	Builder.CreateBr(AfterBB);
       
      //Emit endif value
      Builder.SetInsertPoint(AfterBB);
   	  bblockGlobal = AfterBB;
   	  isReturn = false;
      return m;
    }

    case IFTHENELSE:  
     {
      Value *v = ast_compile(t->left);
      if (!v)
        return nullptr;

      Value *cond = Builder.CreateICmpNE(v, c1(0), "if_cond");
      Function *TheFunction = Builder.GetInsertBlock()->getParent();
      
      // Create blocks for the then and else cases. Insert the 'then' 
      // block at the end of the function
      BasicBlock *ThenBB =
          BasicBlock::Create(TheContext, "then", TheFunction);
      BasicBlock *ElseBB =
          BasicBlock::Create(TheContext, "else");
      BasicBlock *MergeBB =
          BasicBlock::Create(TheContext, "endif");
      Builder.CreateCondBr(cond, ThenBB, ElseBB);

      //Emit then value
      Builder.SetInsertPoint(ThenBB);
      bblockGlobal= ThenBB;
      isReturn = false;
      Value* ThenV = ast_compile(t->middle);
      if(isReturn == false)
      	Builder.CreateBr(MergeBB);
      // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
      ThenBB = Builder.GetInsertBlock();

      // Emit else block.
      TheFunction->getBasicBlockList().push_back(ElseBB);
      Builder.SetInsertPoint(ElseBB);
      bblockGlobal = ElseBB;
      isReturn = false;
      Value * ElseV = ast_compile(t->right);
      if(isReturn == false)
      	Builder.CreateBr(MergeBB);
      // Codegen of 'Else' can change the current block, update ElseBB for the PHI.
      ElseBB = Builder.GetInsertBlock();

      // Emit merge block.
      TheFunction->getBasicBlockList().push_back(MergeBB);
      Builder.SetInsertPoint(MergeBB);
      bblockGlobal = MergeBB;
      
   	  isReturn = false;
      return nullptr;
    }

    case SEQ: {
      Value * m = nullptr;
      isReturn = false;
      Value * l = ast_compile(t->left);
      if(isReturn == false)
      	m = ast_compile(t->middle);
     
      if (m == nullptr)
        return l;
      else
        return m;
    }

    case ID: {
       // Look this variable up in the function.
  
      Value *V = NamedValues[t->id];
      if (!V)
        error("Unknown variable name");
      if(t->left == NULL)
      	return Builder.CreateLoad(V,t->id);

      Value * index = ast_compile(t->left);
      auto zero = llvm::ConstantInt::get(TheContext, llvm::APInt(16, 0, true));
      auto ptr = llvm::GetElementPtrInst::CreateInBounds(NamedValues[t->id], { zero, index }, "", bblockGlobal);
      return Builder.CreateLoad(ptr);
    }

    case CONST: {
      if(equalType(t->type,typeBoolean))
        return c1(t->num);
      else
        return c16(t->num);
    }
    case PLUS: {
      Value *l = ast_compile(t->left);
      Value *m = ast_compile(t->middle);
      return Builder.CreateAdd(l, m, "addtmp");
    }
    case MINUS: {
      Value *l = ast_compile(t->left);
      Value *m = ast_compile(t->middle);
      return Builder.CreateSub(l, m, "subtmp");
    }
    case TIMES: {
      Value *l = ast_compile(t->left);
      Value *m = ast_compile(t->middle);
      return Builder.CreateMul(l, m, "multmp");
    }
    case DIV: {
      Value *l = ast_compile(t->left);
      Value *m = ast_compile(t->middle);
      return Builder.CreateSDiv(l, m, "divtmp");
    }
    case MOD: {
      Value *l = ast_compile(t->left);
      Value *m = ast_compile(t->middle);
      return Builder.CreateSRem(l, m, "modtmp");
    }
  
    case WHILE:{
      Function *TheFunction = Builder.GetInsertBlock()->getParent();
      BasicBlock *LoopBB = BasicBlock::Create(TheContext, "while_loop", TheFunction);
      Builder.CreateBr(LoopBB);

      // Start insertion in the loop.
      Builder.SetInsertPoint(LoopBB);
      bblockGlobal = LoopBB;

      Value *v = ast_compile(t->left);
      if (!v)
       	return nullptr;
      Value *cond = Builder.CreateICmpNE(v, c1(0), "while_cond");

      BasicBlock *InsideBB =
          BasicBlock::Create(TheContext, "while_true", TheFunction);
      BasicBlock *AfterBB =
          BasicBlock::Create(TheContext, "while_false", TheFunction);
      Builder.CreateCondBr(cond, InsideBB, AfterBB);

      //Emit then value
      Builder.SetInsertPoint(InsideBB);
      bblockGlobal = InsideBB;
      isReturn = false;
      Value* m = ast_compile(t->middle);
      if(isReturn == false)
      	Builder.CreateBr(LoopBB);

      //Emit endif value
      Builder.SetInsertPoint(AfterBB);
      bblockGlobal = AfterBB;
      isReturn = false;
      return nullptr;
    }

    case VARDEF:{
      Type * temp_type = getTypeFromMyType(t->type,t->num,0); //0 is for by value
      Value * v = getInitLocalVar(t->type,t->num);
      VarNames.push_back(std::make_tuple(t->id, temp_type,v));
      //ftiaxnw ta global tou paidiou mou
      
      GlobalVarNames.push_back(t->id);
      GlobalVarTypes.push_back(temp_type);
      return nullptr;
    }

    case FPARDEF:{
      argvName[argvIterator] = (char*) malloc(sizeof(char)*(strlen(t->id)+1));
      strcpy(argvName[argvIterator], t->id);
      Type * temp_type = getTypeFromMyType(t->type,0,t->num);  //no dimensions in array parameters
      argvType.push_back(temp_type);
      argvIterator ++;
      return nullptr;
    }
    

case FUNCDEF:{
      std::vector<Type *> OldArgvType;
      Value * RetVal;
      argvName = (char **) malloc(sizeof(char*) * t->num);
      argvIterator =0;
      argvType.resize(t->num + GlobalVarNames.size());
      argvType.clear();

      Type * retType = getFunctionTypeFromMyType(t->type);

      ast_compile(t->left);

     // push ArgvType 
      OldArgvType.resize(t->num);
      OldArgvType.clear();
      for(unsigned i = 0; i < argvType.size(); i++)
        OldArgvType.push_back(argvType[i]);
     
      StackOfGlobalVarNames.push(GlobalVarNames); // push myGlobalVarNames (auta poy moy eftia3e o pateras)
      StackOfGlobalVarTypes.push(GlobalVarTypes);
      MyGlobalVarNames = StackOfGlobalVarNames.top(); // copy myglobalVarnames
      MyGlobalVarTypes = StackOfGlobalVarTypes.top(); 
      GlobalsOfFunction[t->id] = MyGlobalVarNames;
    //ftiaxnw toy paidiou moy ta global arguments
      for(unsigned i = 0; i < argvType.size(); i++){
       		GlobalVarNames.push_back(argvName[i]);
       		GlobalVarTypes.push_back(argvType[i]);
      }
      int l = t->num;
      // 8a valw san arguments ta myglobal
      for(unsigned i=0; i<MyGlobalVarTypes.size(); i++){
      	if((*MyGlobalVarTypes[i]).isPointerTy())
      		argvType.push_back(MyGlobalVarTypes[i]);
      	else
      		argvType.push_back(PointerType::getUnqual(MyGlobalVarTypes[i]));
      }

      FunctionType *FT = FunctionType::get(retType,argvType, false);
      Function *thisFunction = Function::Create(FT, Function::ExternalLinkage, t->id, TheModule.get());   
      
      unsigned Idx = 0; 
      for (auto &Arg : thisFunction->args()){
        if(Idx<l)
        	Arg.setName(argvName[Idx]);
        else
        	Arg.setName((char *)MyGlobalVarNames[Idx-l].c_str());
        Idx++;
      }  

      for(unsigned i = 0; i < t->num; i++)
         free(argvName[i]);
      free(argvName);

      BasicBlock *BB = BasicBlock::Create(TheContext, "entry", thisFunction);
     
      StackOfVarNames.push(VarNames);     //push old (parent) locals
      VarNames.clear();

      ast_compile(t->middle);

      NamedValues.clear();
      Builder.SetInsertPoint(BB);

    // pop ArgvType 
      argvType.resize(t->num);
      argvType.clear();
      for(unsigned i = 0; i < OldArgvType.size(); i++)
        argvType.push_back(OldArgvType[i]);

      int i =0;

      ast tempseq =NULL;
      ast temp = t->left; //arguments
     
      for (auto &Arg : thisFunction->args()) {
      	if(i<t->num){
	      	tempseq = temp->middle;
	        temp = temp->left;

			AllocaInst *Alloca;
	        if(temp->num == 0){	//is by value
		        // Create an alloca for this variable.
		        Alloca = CreateEntryBlockAlloca(thisFunction, Arg.getName(),argvType[i]);
		        // Store the initial value into the alloca.
		        Builder.CreateStore(&Arg, Alloca);
			}
			else{			
				Alloca = (AllocaInst *) &Arg;
			}
	        // Add arguments to variable symbol table.
	        NamedValues[Arg.getName()] = Alloca;
	        temp = tempseq;
	    }
	    else{
	    	AllocaInst *Alloca;
	    	Alloca = (AllocaInst *) &Arg;
	    	NamedValues[Arg.getName()] = Alloca;
	    }
	    i++;
      }
  
      // Register all variables and emit their initializer.
      for (unsigned i = 0, e = VarNames.size(); i != e; ++i) {
        const std::string &VarName = std::get<0>(VarNames[i]);
        Type *VarType = std::get<1>(VarNames[i]);
        // Emit the initializer before adding the variable to scope, this prevents
        // the initializer from referencing the variable itself, and permits stuff
        // Value *InitVal = ConstantFP::get(TheContext, APFloat(0.0));
        Value *InitVal = std::get<2>(VarNames[i]);   
      
        AllocaInst *Alloca = CreateEntryBlockAlloca(thisFunction, VarName, VarType);
   
        Builder.CreateStore(InitVal, Alloca);
        NamedValues[VarName] = Alloca;
      }

      bblockGlobal = BB;
      
      RetVal = ast_compile(t->right);
      
      
      VarNames = StackOfVarNames.top();
      StackOfVarNames.pop();      
     GlobalVarNames = StackOfGlobalVarNames.top();
     GlobalVarTypes = StackOfGlobalVarTypes.top();
     StackOfGlobalVarNames.pop();
     StackOfGlobalVarTypes.pop();
     
      if (equalType(t->type, typeVoid) && isReturn == false)
      	Builder.CreateRetVoid();
      
      if(!equalType(t->type, typeVoid) && isReturn == false){
      	if(equalType(t->type,typeInteger))
      		Builder.CreateRet(c16(0));
      	else
      		Builder.CreateRet(c8(0));
      }
      
      // Validate the generated code, checking for consistency.
      verifyFunction(*thisFunction);
      isReturn = false;
      return thisFunction;
    }

    case FUNCALL:{
		// Look up the name in the global module table.
		Function *CalleeF = getFunction(t->id);
		if (!CalleeF)
			printf("Unknown function referenced\n");

		// // If argument mismatch error.
		// if (CalleeF->arg_size() != Args.size())
		//   return LogErrorV("Incorrect # arguments passed");
		std::vector<Value *> ArgsV;
		ArgsV.clear();
		int count =0;
		Value * P = NULL;
		ast tempseq =NULL;
		ast temp = NULL;   
		ast tempseqDef = NULL;
		ast tempDef = NULL;  
		temp = t->left;	//arguments CALL

		if (strcmp(t->id,"writeInteger")==0 || strcmp(t->id,"writeByte")==0){
			Value *n = ast_compile(temp->left);
			// Value *n16 = Builder.CreateZExt(n, i16, "ext");
			// ArgsV.push_back(n16);
			ArgsV.push_back(n);
			if (!ArgsV.back())
				return nullptr;
			return Builder.CreateCall(CalleeF, ArgsV);
		}

		if (strcmp(t->id,"readString")==0){
			int count =0;
			while (temp !=NULL){
				tempseq = temp->middle;// SEQ or NULL
				temp = temp->left; // argument
				Value * n = ast_compile(temp);
				if(n->getType() == i16){
					Value *n16 = Builder.CreateZExt(n, i16, "ext");
					ArgsV.push_back(n16);
				}
				else{
					P = NamedValues[temp->id];
					P = new BitCastInst( P,PointerType::getUnqual(i8) , "", bblockGlobal);
			    	ArgsV.push_back(P);
				}
				if (!ArgsV.back())
				   return nullptr;
				temp = tempseq;
				count++;
			}
			return Builder.CreateCall(CalleeF, ArgsV);
		}

		if(strcmp(t->id,"writeString")==0){
			temp = temp->left;
			
			Value * argStr = ast_compile(temp);

			if(strcmp(temp->id,"")==0){
				ArgsV.push_back(argStr);
				return Builder.CreateCall(CalleeF, ArgsV);	
			}
			P = NamedValues[temp->id];
			P = new BitCastInst( P,PointerType::getUnqual(i8) , "", bblockGlobal);
			ArgsV.push_back(P);
			return Builder.CreateCall(CalleeF, ArgsV);
		}

		if(strcmp(t->id,"writeChar")==0){
			temp = temp->left;
			Value * argStr = ast_compile(temp);
			ArgsV.push_back(argStr);
			return Builder.CreateCall(CalleeF, ArgsV);	
		}

		if(strcmp(t->id,"extend")==0 || strcmp(t->id,"shrink")==0){
			temp = temp->left;
			Value * argChar = ast_compile(temp);
			ArgsV.push_back(argChar);
			return Builder.CreateCall(CalleeF, ArgsV, "calltmp");	
		}


		if( strcmp(t->id,"strlen")==0){
			temp = temp->left;
			Value * argStr = ast_compile(temp);
			if(strcmp(temp->id,"")==0){
				ArgsV.push_back(argStr);
				return Builder.CreateCall(CalleeF, ArgsV,"calltmp");	
			}
			P = NamedValues[temp->id];
			P = new BitCastInst( P,PointerType::getUnqual(i8) , "", bblockGlobal);
			ArgsV.push_back(P);
			return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
		}

    if(strcmp(t->id,"strcmp")==0 || strcmp(t->id,"strcpy")==0 || strcmp(t->id,"strcat")==0 ){
      while (temp !=NULL){
        tempseq = temp->middle;// SEQ or NULL
        temp = temp->left; // argument
        Value * n = ast_compile(temp);
        if(strcmp(temp->id,"")==0){
          ArgsV.push_back(n);
        }
        else{
          P = NamedValues[temp->id];
          P = new BitCastInst( P,PointerType::getUnqual(i8) , "", bblockGlobal);
          ArgsV.push_back(P);
       }
        if (!ArgsV.back())
           return nullptr;
        temp = tempseq;
      }
      if(strcmp(t->id,"strcmp")==0)
        return Builder.CreateCall(CalleeF, ArgsV,"calltmp");
      else
        return Builder.CreateCall(CalleeF, ArgsV);
    }



		if(strcmp(t->id,"readChar")==0 || strcmp(t->id,"readInteger")==0 || strcmp(t->id,"readByte")==0){
			
			return Builder.CreateCall(CalleeF, ArgsV, "calltmp");	
		}

		tempDef = t->middle->left;	//arguments funcdef

		while (temp !=NULL){  
			tempseq = temp->middle;// SEQ or NULL
			temp = temp->left; // argument
			tempseqDef = tempDef->middle;// SEQ or NULL
			tempDef = tempDef->left; // argument

			if(tempDef->num == 1){ //by reference	
				if(strcmp(temp->id,"")==0){
					Value * str_temp = Builder.CreateGlobalStringPtr(temp->constchar);
					P = str_temp;
					P = new BitCastInst( P,PointerType::getUnqual(ArrayType::get(i8,0)) , "", bblockGlobal);

				}
				else{
					P = NamedValues[temp->id];	//address of arg call
									
					// if is array then i want to bitcast se *i8 h se *i16
					if(equalType(temp->type,typeArrayInteger))
						P = new BitCastInst( P, PointerType::getUnqual(ArrayType::get(i16,0)), "", bblockGlobal);

					if(equalType(temp->type,typeArrayByte))
						P = new BitCastInst( P,PointerType::getUnqual(ArrayType::get(i8,0)) , "", bblockGlobal);

					if(temp->left !=NULL){
						 auto zero = llvm::ConstantInt::get(TheContext, llvm::APInt(16, 0, true));
						 Value * Index = ast_compile(temp->left);
						 P = llvm::GetElementPtrInst::CreateInBounds(P, { zero, Index }, "", bblockGlobal);
							  
					}
				}
				ArgsV.push_back(P);
			}
			else
				ArgsV.push_back(ast_compile(temp));

			if (!ArgsV.back())
				return nullptr;

			temp = tempseq;
			tempDef = tempseqDef;
			count++;
		}
		std::vector<std::string> CalleeGlobalVarNames = GlobalsOfFunction[t->id]; 
		for(unsigned i=0; i<CalleeGlobalVarNames.size(); i++){
			P = NamedValues[CalleeGlobalVarNames[i].c_str()];
			
			ArgsV.push_back(P);

		}
	    if(CalleeF->getReturnType() ==  Type::getVoidTy(TheContext))
	    	return Builder.CreateCall(CalleeF, ArgsV);
	    else
	    	return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
    }

    case RETURN:{
     
      if (t->left == NULL){
        Builder.CreateRetVoid();
        isReturn = true;
        return nullptr;
      }
      Value * RetVal = ast_compile(t->left);
      
      Builder.CreateRet(RetVal);
      isReturn = true;
      return RetVal;
    }

    case INTEGER:
      return c16(t->num);

    case CHAR:
      return c8((char) t->num);

    case STRING:{
      
      Value * str_temp = Builder.CreateGlobalStringPtr(t->constchar);
      return str_temp;
    }
   
    case EQUAL:{
      Value *l = ast_compile(t->left);
      Value *m = ast_compile(t->middle);
      l = Builder.CreateICmpEQ(l, m, "equaltmp");
      return l;
      // Convert bool 0/1 to double 0.0 or 1.0
      // return Builder.CreateUIToFP(l, Type::getDoubleTy(TheContext),
                                // "booltmp");
    }
    case NOTEQUAL:{
      Value *l = ast_compile(t->left);
      Value *m = ast_compile(t->middle);
      
      l = Builder.CreateICmpNE(l, m, "notequaltmp");
      return l;
      // Convert bool 0/1 to double 0.0 or 1.0
      // return Builder.CreateUIToFP(l, Type::getDoubleTy(TheContext),
                                // "booltmp");
    }

    case LESS:{
      Value *l = ast_compile(t->left);
      Value *m = ast_compile(t->middle);
      
      l = Builder.CreateICmpSLT(l, m, "lesstmp");
      return l;
      // return Builder.CreateIntCast(l,i1,true);
      // Convert bool 0/1 to double 0.0 or 1.0
      // return Builder.CreateUIToFP(l, Type::getDoubleTy(TheContext),
                                // "booltmp");
    }

    case LESSEQUAL:{
      Value *l = ast_compile(t->left);
      Value *m = ast_compile(t->middle);
      
      l = Builder.CreateICmpSLE(l, m, "lessequaltmp");
      return l;
      // Convert bool 0/1 to double 0.0 or 1.0
      // return Builder.CreateUIToFP(l, Type::getDoubleTy(TheContext),
                                // "booltmp");
    }

    case GREAT:{
      Value *l = ast_compile(t->left);
      Value *m = ast_compile(t->middle);
      
      l = Builder.CreateICmpSGT(l, m, "greattmp");
      return l;
      // Convert bool 0/1 to double 0.0 or 1.0
      // return Builder.CreateUIToFP(l, Type::getDoubleTy(TheContext),
                                // "booltmp");
    }

    case GREATEQUAL:{
      Value *l = ast_compile(t->left);
      Value *m = ast_compile(t->middle);
      
      l = Builder.CreateICmpSGE(l, m, "greatequaltmp");
      return l;
      // Convert bool 0/1 to double 0.0 or 1.0
      // return Builder.CreateUIToFP(l, Type::getDoubleTy(TheContext),
                                // "booltmp");
    }
    case AND:{
      Value *l = ast_compile(t->left);
      Value *m = ast_compile(t->middle);
      return Builder.CreateAnd(l,m,"andtmp");
    }

    case OR:{
      Value *l = ast_compile(t->left);
      Value *m = ast_compile(t->middle);
      return Builder.CreateOr(l,m,"ortmp");
    }

    case NOT:{
      Value *l = ast_compile(t->left);
      return Builder.CreateNot(l,"nottmp");
    }

    default:{
     error("Compile: Unknown Case");
     return nullptr; 
    }     
    
  }
  return nullptr;
}



#define NOTHING 0

struct activation_record_tag {
  struct activation_record_tag * previous;
  int data[0];
};


typedef struct activation_record_tag * activation_record;

activation_record current_AR = NULL;

//static int var[26];
char* temp ;



SymbolEntry * lookup(char* c) {
  char* name;
  name = (char *)malloc(sizeof(char)*(strlen(c)+1));
  strcpy(name, c);
  SymbolEntry * e = lookupEntry(name, LOOKUP_ALL_SCOPES, true);
  free(name);
  return e;
}

SymbolEntry * insertFunction(ast n, char* name){
  strcpy(name, n->id);
  return newFunction(name,n->type,n);
}


SymbolEntry * insertVariable(char* c, MyType t){
  char* name;
  name = (char *)malloc(sizeof(char)*(strlen(c)+1));
  strcpy(name, c);
  SymbolEntry * e = newVariable(name,t);
  free(name);
  return e;
}


SymbolEntry * insertParameter(char* c, MyType t, PassMode reference){
  char* name;
  name = (char *)malloc(sizeof(char)*(strlen(c)+1));
  strcpy(name, c);
  SymbolEntry * e = lookup(rootFunc->data);
  SymbolEntry * e1 =newParameter(name,t,reference ,e);	
  free(name);
  return e1;
}


int tempArgNum =0;

// rootFunc declared in stack.h

void ast_sem (ast t) {
  if (t == NULL) return;
  switch (t->k) {
    case PROGRAM: 
      openScope();
      ast_sem(t->left);
      t->num_vars = currentScope->negOffset;
      closeScope();
      return;

     case ASSIGN:{
      SymbolEntry * e = lookup(t->id);
      ast_sem(t->middle); //rexpr
      MyType ltype = e->u.eVariable.type;
      if (equalType(ltype, typeInteger) || equalType(ltype, typeByte)){
        if (!equalType(ltype, t->middle->type))
          error("type mismatch in assignment");
      }
      else {
        if(t->left == NULL)
          error("lvalue cannot be Array Type!");
        ast_sem(t->left); //index expr
        if(!equalType(t->left->type, typeInteger))
          error("index of array is not integer!");

        if(equalType(ltype, typeArrayInteger)){
          if(!equalType(t->middle->type, typeInteger))
            error("right expression is not integer!");  
        }
        if(equalType(ltype, typeArrayByte)){
          if(!equalType(t->middle->type, typeByte))
            error("right expression is not byte!");  
        }
      }    
      t->nesting_diff = currentScope->nestingLevel - e->nestingLevel;
      t->offset = e->u.eVariable.offset;
      
      return;
    }
    	
    case IF:
      ast_sem(t->left);
      if (!equalType(t->left->type, typeBoolean))
        error("if expects a boolean condition");
      ast_sem(t->middle);
      return;

    case IFTHENELSE:
      ast_sem(t->left);
      if (!equalType(t->left->type, typeBoolean))
        error("if expects a boolean condition");
      ast_sem(t->middle);
      ast_sem(t->right);
      return;

    case WHILE:
      ast_sem(t->left);
      if (!equalType(t->left->type, typeBoolean))
        error("while expects a boolean condition");
      ast_sem(t->middle);
      return;

    case VARDEF:{
    	insertVariable(t->id, t->type);
      return;
  }

    case FPARDEF:
      if (t->num == 1)
        insertParameter(t->id, t->type, PASS_BY_REFERENCE);
      else
        insertParameter(t->id, t->type, PASS_BY_VALUE);
      tempArgNum++;
      return;

    case FUNCDEF:{
      rootFunc = push(rootFunc,t->id); 
      // h fixoffset afeinei 8B kena....
      char* name;
      name = (char *)malloc(sizeof(char)*(strlen(t->id)+1));
      SymbolEntry * tem =  insertFunction(t,name);
      free(name);
      openScope();
      tempArgNum =0;
      ast_sem(t->left);
      t->num = tempArgNum;
      ast_sem(t-> middle);
     
      t->num_vars = currentScope->negOffset;
      ast_sem(t->right);
      rootFunc = pop(rootFunc);
      closeScope();
      return;
  	}

case FUNCALL:{
   
    if(strcmp(t->id,"readChar")==0 ){
      t->type = typeByte;
      return;
    }
    if(strcmp(t->id,"readString")==0 ){
      t->type = typeVoid;
      ast_sem(t->left);
      return;
    }
    if(strcmp(t->id,"readInteger")==0 ){
      t->type = typeInteger;
      return;
    }
    if(strcmp(t->id,"readByte")==0 ){
      t->type = typeByte;
      return;
    }
    if(strcmp(t->id,"writeString")==0 ){
      t->type = typeVoid;
      return;
    }
    if(strcmp(t->id,"writeInteger")==0 ){
      t->type = typeVoid;
      ast_sem(t->left);
      return;
    }
    if(strcmp(t->id,"writeByte")==0 ){
      t->type = typeVoid;
      ast_sem(t->left);
      return;
    }
    if(strcmp(t->id,"writeChar")==0 ){
      t->type = typeVoid;
      ast_sem(t->left);
      return;
    }

    if(strcmp(t->id,"strlen")==0 || strcmp(t->id,"strcmp")==0){
      t->type = typeInteger;
      ast_sem(t->left);
      return;
    }

     if(strcmp(t->id,"strcat")==0 || strcmp(t->id,"strcpy")==0){
      t->type = typeVoid;
      ast_sem(t->left);
      return;
    }

    if(strcmp(t->id,"shrink")==0 ){
      t->type = typeByte;
      ast_sem(t->left);
      return;
    }

    if(strcmp(t->id,"extend")==0 ){
      t->type = typeInteger;
      ast_sem(t->left);
      return;
    }
    
      SymbolEntry * e = lookup(t->id);
      t->type = e->u.eFunction.resultType;
      t->middle = e->u.eFunction.funcNode;
     

      ast_sem(t->left);
      ast tempseq =NULL;
      ast temp = t->left;
      SymbolEntry * arg = e->u.eFunction.firstArgument;
      int count =0;
      while (temp !=NULL){
        tempseq = temp->middle;
        temp = temp->left;

        if(arg==NULL) error("function %s called with too many arguments",t->id);
     
        if(equalType(temp->type, typeString)){
          if(!equalType(arg->u.eParameter.type, typeArrayByte))
          error("type mismatch in %d argument of function %s",count,t->id);
        }
        else if(!equalType(temp->type,arg->u.eParameter.type)){
          error("type mismatch in %d argument of function %s",count,t->id);
        }

        temp = tempseq;
        arg = arg->u.eParameter.next;
        count++;
      }

      if (arg !=NULL) error("function %s called with less arguments",t->id);
      t->nesting_diff = currentScope->nestingLevel - e->nestingLevel;
     
      return;
    }
    case RETURN:{
      ast_sem(t->left);
      SymbolEntry * e = lookup(rootFunc->data);
  
      if (t->left != NULL){
        if (!equalType(e->u.eFunction.resultType, t->left->type))
          error("Return type mismatch\n");
      }
      else{
        if (!equalType(e->u.eFunction.resultType, typeVoid))
          error("Return type mismatch\n");
      }
   
      return;
    }
    case SEQ:
      ast_sem(t->left);
      ast_sem(t->middle);
      return;

    case ID:{
      SymbolEntry *e = lookup(t->id);
      
      if(t->left == NULL)
        t->type = e->u.eVariable.type;
      else{
        ast_sem(t->left); //index expr
        if(!equalType(t->left->type,typeInteger))
          error("Index of array is not INteger!");
        if(equalType(e->u.eVariable.type,typeArrayInteger))
          t->type = typeInteger;
        else if(equalType(e->u.eVariable.type, typeArrayByte))
          t->type = typeByte;
        else
          error("Wrong Type in SymbolEntry!");
      }

      t->nesting_diff = currentScope->nestingLevel - e->nestingLevel;
      t->offset = e->u.eVariable.offset;

      return;
    }

    case CONST:
    /* type field is filled by calling ast_const */
      return;

    case INTEGER:
      /* type field is filled by runing ast_integer */
      return;
    case CHAR:
      /* type field is filled by runing ast_char */
      return;
    case STRING:
      /* type field is filled by running ast_string */
      return;

    case PLUS:
      ast_sem(t->left);
      ast_sem(t->middle);
      if (!equalType(t->left->type, t->middle->type))
        error("type mismatch in + operator");
      if (!equalType(t->left->type, typeInteger) && 
          !equalType(t->left->type, typeByte))
        error(" + operator expects Byte or Integer)");
      t->type = t->left->type;
      return;

    case MINUS:
      ast_sem(t->left);
      ast_sem(t->middle);
      if (!equalType(t->left->type, t->middle->type))
        error("type mismatch in - operator");
      if (!equalType(t->left->type, typeInteger) && 
          !equalType(t->left->type, typeByte))
        error(" - operator expects Byte or Integer)");
      t->type = t->left->type;
      return;

    case TIMES:
      ast_sem(t->left);
      ast_sem(t->middle);
      if (!equalType(t->left->type, t->middle->type))
        error("type mismatch in * operator");
      if (!equalType(t->left->type, typeInteger) && 
          !equalType(t->left->type, typeByte))
        error(" * operator expects Byte or Integer)");
      t->type = t->left->type;
      return;

    case DIV:
      ast_sem(t->left);
      ast_sem(t->middle);
      if (!equalType(t->left->type, t->middle->type))
        error("type mismatch in / operator");
      if (!equalType(t->left->type, typeInteger) && 
          !equalType(t->left->type, typeByte))
        error(" / operator expects Byte or Integer)");
      t->type = t->left->type;
      return;

    case MOD:
      ast_sem(t->left);
      ast_sem(t->middle);
      if (!equalType(t->left->type, t->middle->type))
        error("type mismatch in % operator");
      if (!equalType(t->left->type, typeInteger) && 
          !equalType(t->left->type, typeByte))
        error(" % operator expects Byte or Integer)");
      t->type = t->left->type;
      return;

    case EQUAL:
      ast_sem(t->left);
      ast_sem(t->middle);
      if (!equalType(t->left->type, t->middle->type))
        error("type mismatch in == operator");
      if (!equalType(t->left->type, typeInteger) &&
          !equalType(t->left->type, typeByte))
        error(" == operator expects Byte or Integer)");
      t->type = typeBoolean;
      return;
    case NOTEQUAL:
      ast_sem(t->left);
      ast_sem(t->middle);
      if (!equalType(t->left->type, t->middle->type))
        error("type mismatch in 1= operator");
      if (!equalType(t->left->type, typeInteger) && 
          !equalType(t->left->type, typeByte))
        error(" != operator expects Byte or Integer)");
      t->type = typeBoolean;
      return;
    case LESS:
      ast_sem(t->left);
      ast_sem(t->middle);
      if (!equalType(t->left->type, t->middle->type))
        error("type mismatch in < operator");
     
      if (!equalType(t->left->type, typeInteger) && 
          !equalType(t->left->type, typeByte))
        error(" < operator expects Byte or Integer");

      t->type = typeBoolean;
      return;
    case LESSEQUAL:
      ast_sem(t->left);
      ast_sem(t->middle);
      if (!equalType(t->left->type, t->middle->type))
        error("type mismatch in <= operator");
      if (!equalType(t->left->type, typeInteger) && 
          !equalType(t->left->type, typeByte))
        error(" <= operator expects Byte or Integer)");
      t->type = typeBoolean;
      return;
    case GREAT:
      ast_sem(t->left);
      ast_sem(t->middle);
      if (!equalType(t->left->type, t->middle->type))
        error("type mismatch in > operator");
      if (!equalType(t->left->type, typeInteger) && 
          !equalType(t->left->type, typeByte))
        error(" > operator expects Byte or Integer)");
      t->type = typeBoolean;
      return;
    case GREATEQUAL:
      ast_sem(t->left);
      ast_sem(t->middle);
      if (!equalType(t->left->type, t->middle->type))
        error("type mismatch in >= operator");
      if (!equalType(t->left->type, typeInteger) && 
          !equalType(t->left->type, typeByte))
        error(" >= operator expects Byte or Integer)");
      t->type = typeBoolean;
      return;
    case AND:
      ast_sem(t->left);
      ast_sem(t->middle);
      if (!equalType(t->left->type, t->middle->type))
        error("type mismatch in & operator");
      if (!equalType(t->left->type, typeBoolean))
        error(" & operator expects Boolean)");
      t->type = typeBoolean;
      return;
    case OR:
      ast_sem(t->left);
      ast_sem(t->middle);
      if (!equalType(t->left->type, t->middle->type))
        error("type mismatch in | operator");
      if (!equalType(t->left->type, typeBoolean))
        error(" | operator expects Boolean)");
      t->type = typeBoolean;
      return;
    case NOT:
      ast_sem(t->left);
      if (!equalType(t->left->type, typeBoolean))
        error("type mismatch in ! operator");
      t->type = typeBoolean;
      return;

    default:
      error("Error NOT RECOGNIZED IN switch ast_sem!!");
    
  }
  return;
}


void freeAstTree (ast t) {
  if (t == NULL) return;
  switch (t->k) {
    case PROGRAM: {
      freeAstTree(t->left);
      free(t->constchar); 
      free(t->id); 
      free(t);
      return;
    }
     case ASSIGN:{
      freeAstTree(t->middle); //rexpr
      freeAstTree(t->left); //index expr
      free(t->id);
      free(t->constchar);
      free(t);
      return;
    }
      
    case IF:
      freeAstTree(t->left);
      freeAstTree(t->middle);
      free(t->id); 
      free(t->constchar);
      free(t);
      return;

    case IFTHENELSE:
      freeAstTree(t->left);
      freeAstTree(t->middle);
      freeAstTree(t->right);
      free(t->id); 
      free(t->constchar);
      free(t);
      return;

    case WHILE:
      freeAstTree(t->left);
      freeAstTree(t->middle);
      free(t->id); 
      free(t->constchar);
      free(t);
      return;

    case VARDEF:{
      free(t->id);
      free(t->constchar);
      free(t);
      return;
  }

    case FPARDEF:
      free(t->id);
      free(t->constchar);
      free(t);
      return;

    case FUNCDEF:{
      freeAstTree(t->left);
      freeAstTree(t-> middle);
      freeAstTree(t->right);
      free(t->id);
      free(t->constchar);
      free(t);
      return;
    }

    case FUNCALL:{
      freeAstTree(t->left);
      free(t->id);
      free(t->constchar);
      free(t);
      return;
    }

    case RETURN:{
      freeAstTree(t->left);
      free(t->id); 
      free(t->constchar);
      free(t);
      return;
    }
    case SEQ:
      freeAstTree(t->left);
      freeAstTree(t->middle);
      free(t->id); 
      free(t->constchar);
      free(t);
      return;

    case ID:{
      if(t->left != NULL)
        freeAstTree(t->left); //index expr
      free(t->id); 
      free(t->constchar);
      free(t);
      return;
    }

    case CONST:
      free(t->id); 
      free(t->constchar);
      free(t);
      return;

    case INTEGER:
      free(t->id); 
      free(t->constchar);
      free(t);
      return;

    case CHAR:
      free(t->id); 
      free(t->constchar);
      free(t);
      return;
      
    case STRING:
      free(t->id); 
      free(t->constchar);
      free(t);
      return;

    case PLUS:
      freeAstTree(t->left);
      freeAstTree(t->middle);
      free(t->id); 
      free(t->constchar);
      free(t);
      return;

    case MINUS:
      freeAstTree(t->left);
      freeAstTree(t->middle);
      free(t->id); 
      free(t->constchar);
      free(t);
      return;

    case TIMES:
      freeAstTree(t->left);
      freeAstTree(t->middle);
      free(t->id); 
      free(t->constchar);
      free(t);
      return;

    case DIV:
      freeAstTree(t->left);
      freeAstTree(t->middle);
      free(t->id); 
      free(t->constchar);
      free(t);
      return;

    case MOD:
      freeAstTree(t->left);
      freeAstTree(t->middle);
      free(t->id); 
      free(t->constchar);
      free(t);
      return;

    case EQUAL:
      freeAstTree(t->left);
      freeAstTree(t->middle);
      free(t->id); 
      free(t->constchar);
      free(t);
      return;

    case NOTEQUAL:
      freeAstTree(t->left);
      freeAstTree(t->middle);
      free(t->id); 
      free(t->constchar);
      free(t);
      return;

    case LESS:
      freeAstTree(t->left);
      freeAstTree(t->middle);
      free(t->id); 
      free(t->constchar);
      // free(t->type);
      free(t);
      return;
    case LESSEQUAL:
      freeAstTree(t->left);
      freeAstTree(t->middle);
      free(t->id); 
      free(t->constchar);
      free(t);
      return;

    case GREAT:
      freeAstTree(t->left);
      freeAstTree(t->middle);
      free(t->id); 
      free(t->constchar);
      free(t);
      return;

    case GREATEQUAL:
      freeAstTree(t->left);
      freeAstTree(t->middle);
      free(t->id); 
      free(t->constchar);
      free(t);
      return;

    case AND:
      freeAstTree(t->left);
      freeAstTree(t->middle);
      free(t->id); 
      free(t->constchar);
      free(t);
      return;

    case OR:
      freeAstTree(t->left);
      freeAstTree(t->middle);
      free(t->id); 
      free(t->constchar);
      free(t);
      return;

    case NOT:
      freeAstTree(t->left);
      free(t->id); 
      free(t->constchar);
      free(t);
      return;

    default:
      error("Error NOT RECOGNIZED IN switch freeAstTree!!");
    
  }
  return;
}

void llvm_compile_and_dump (ast t) {
  // Initialize the module and the optimization passes.
  TheModule = make_unique<Module>("alan program", TheContext);
  TheFPM = make_unique<legacy::FunctionPassManager>(TheModule.get());
  TheFPM->add(createPromoteMemoryToRegisterPass());
  TheFPM->add(createInstructionCombiningPass());
  TheFPM->add(createReassociatePass());
  TheFPM->add(createGVNPass());
  TheFPM->add(createCFGSimplificationPass());
  TheFPM->doInitialization();
  
  ArrayType *nl_type = ArrayType::get(i8, 2);
  TheNL = new GlobalVariable(
      *TheModule, nl_type, true, GlobalValue::PrivateLinkage,
      ConstantArray::get(nl_type,
                         std::vector<Constant *>{ c8('\n'), c8('\0') }),
      "nl");
  TheNL->setAlignment(1);

 //declare i8 @readChar()
 FunctionType *readChar_type =
    FunctionType::get(i8, EmptyVectorType(), false);
  TheReadChar =
    Function::Create(readChar_type, Function::ExternalLinkage,
                     "readChar", TheModule.get());

 //declare i16 @readInteger()
 FunctionType *readInteger_type =
    FunctionType::get(i16, EmptyVectorType(), false);
  TheReadInteger =
    Function::Create(readInteger_type, Function::ExternalLinkage,
                     "readInteger", TheModule.get());

//declare i8 @readByte()
 FunctionType *readByte_type =
    FunctionType::get(i8, EmptyVectorType(), false);
  TheReadByte =
    Function::Create(readByte_type, Function::ExternalLinkage,
                     "readByte", TheModule.get());

    //declare void @readString(i16, i8*)
 FunctionType *readString_type =
    FunctionType::get(Type::getVoidTy(TheContext),
                      std::vector<Type *>{ i16, PointerType::get(i8, 0)},  false);
  TheReadString =
    Function::Create(readString_type, Function::ExternalLinkage,
                     "readString", TheModule.get());


  // declare void @writeInteger(i16)
  FunctionType *writeInteger_type =
    FunctionType::get(Type::getVoidTy(TheContext),
                      std::vector<Type *>{ i16 }, false);
  TheWriteInteger =
    Function::Create(writeInteger_type, Function::ExternalLinkage,
                     "writeInteger", TheModule.get());

   // declare void @writeByte(i8)
  FunctionType *writeByte_type =
    FunctionType::get(Type::getVoidTy(TheContext),
                      std::vector<Type *>{ i8 }, false);
  TheWriteByte =
    Function::Create(writeByte_type, Function::ExternalLinkage,
                     "writeByte", TheModule.get());

    // declare void @writeChar(i8)
  FunctionType *writeChar_type =
    FunctionType::get(Type::getVoidTy(TheContext),
                      std::vector<Type *>{ i8 }, false);
  TheWriteChar =
    Function::Create(writeChar_type, Function::ExternalLinkage,
                     "writeChar", TheModule.get());

  // declare void @writeString(i8*)
  FunctionType *writeString_type =
    FunctionType::get(Type::getVoidTy(TheContext),
                      std::vector<Type *>{ PointerType::get(i8, 0) }, false);// std::vector<Type *>{ i16 }, false); // 
                      //i16, false);
  TheWriteString =
    Function::Create(writeString_type, Function::ExternalLinkage,
                     "writeString", TheModule.get());

  //declare i16 @strlen(i8*)
 FunctionType *strlen_type =
    FunctionType::get(i16,
                      std::vector<Type *>{ PointerType::get(i8, 0)},  false);
  TheStrlen =
    Function::Create(strlen_type, Function::ExternalLinkage,
                     "strlen", TheModule.get());

  //declare i16 @strcmp(i8*,i8*)
 FunctionType *strcmp_type =
    FunctionType::get(i16,
                      std::vector<Type *>{ PointerType::get(i8, 0),PointerType::get(i8, 0)},  false);
  TheStrcmp =
    Function::Create(strcmp_type, Function::ExternalLinkage,
                     "strcmp", TheModule.get());

//declare @strcpy(i8*,i8*)
 FunctionType *strcpy_type =
    FunctionType::get(Type::getVoidTy(TheContext),
                      std::vector<Type *>{ PointerType::get(i8, 0),PointerType::get(i8, 0)},  false);
  TheStrcpy =
    Function::Create(strcpy_type, Function::ExternalLinkage,
                     "strcpy", TheModule.get());

//declare @strcat(i8*,i8*)
 FunctionType *strcat_type =
    FunctionType::get(Type::getVoidTy(TheContext),
                      std::vector<Type *>{ PointerType::get(i8, 0),PointerType::get(i8, 0)},  false);
  TheStrcat =
    Function::Create(strcat_type, Function::ExternalLinkage,
                     "strcat", TheModule.get());


  //declare i8 @shrink(i16)
 FunctionType *shrink_type =
    FunctionType::get(i8,
                      std::vector<Type *>{ i16 },  false);
  TheShrink =
    Function::Create(shrink_type, Function::ExternalLinkage,
                     "shrink", TheModule.get());

 //declare i16 @extend(i8)
 FunctionType *extend_type =
    FunctionType::get(i16,
                      std::vector<Type *>{ i8 },  false);
  TheExtend =
    Function::Create(extend_type, Function::ExternalLinkage,
                     "extend", TheModule.get());
 
// Start functions' of alan program
      ast_compile(t);

// Define and start the main function.
      Value * RetVal;
   
      argvType.resize(0);
      argvType.clear();

      Type * retType = Type::getVoidTy(TheContext);
      
      FunctionType *FT = FunctionType::get(retType,argvType, false);
      Function *main = Function::Create(FT, Function::ExternalLinkage, "main", TheModule.get());

      BasicBlock *BB = BasicBlock::Create(TheContext, "entry", main);
      Builder.SetInsertPoint(BB);

     // Call first function of alan program
     Function *CalleeF = getFunction(t->left->id);
      if (!CalleeF)
        printf("Unknown function referenced\n");

      std::vector<Value *> ArgsV;
      RetVal = Builder.CreateCall(CalleeF, ArgsV);
      
      Builder.CreateRetVoid();
      verifyFunction(*main);

  // Verify and optimize the main function.
  bool bad = verifyModule(*TheModule, &errs());
  if (bad) {
    fprintf(stderr, "The faulty IR is:\n");
    fprintf(stderr, "------------------------------------------------\n\n");
    TheModule->print(outs(), nullptr);
    return;
  }
  
  TheModule->print(outs(), nullptr);

  freeAstTree(t);
}


