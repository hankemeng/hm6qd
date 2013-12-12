/* File: codegen.cc
 * ----------------
 * Implementation for the CodeGenerator class. The methods don't do anything
 * too fancy, mostly just create objects of the various Tac instruction
 * classes and append them to the list.
 */

#include "codegen.h"
#include <string.h>
#include "tac.h"
#include "mips.h"
#include "errors.h"

Location* CodeGenerator::ThisPtr= new Location(fpRelative, 4, "this");
  
CodeGenerator::CodeGenerator()
{
  gp=0;
}

char *CodeGenerator::NewLabel()
{
  static int nextLabelNum = 0;
  char temp[10];
  sprintf(temp, "_L%d", nextLabelNum++);
  return strdup(temp);
}

Location *CodeGenerator::GenLocalVar(const char* name)
{
  Location *result = new Location(fpRelative, OffsetToFirstLocal - LocalTempNum * VarSize, name);
  // printf("CodeGenerator::GenLocalVar, new Location(fpRelative, %d, %s);\n",OffsetToFirstLocal - LocalTempNum * VarSize, name);
  LocalTempNum++;
  return result;
}

Location *CodeGenerator::GenTempVar()
{
  static int nextTempNum;
  char temp[10];
  sprintf(temp, "_tmp%d", nextTempNum);
  /* pp5: need to create variable in proper location
     in stack frame for use as temporary. Until you
     do that, the assert below will always fail to remind
     you this needs to be implemented  */
  Location *result = new Location(fpRelative, OffsetToFirstLocal - LocalTempNum * VarSize, temp);
  // printf("CodeGenerator::GenTempVar, new Location(fpRelative, %d, _tmp%d);\n",OffsetToFirstLocal - LocalTempNum * VarSize, nextTempNum);
  nextTempNum++;
  LocalTempNum++;
  Assert(result != NULL);
  return result;
}

Location *CodeGenerator::GenGlobalVar(const char *name)
{
    gp+=VarSize;
    return new Location(gpRelative, gp-4, name);
}

Location *CodeGenerator::GenLoadConstant(int value)
{
  Location *result = GenTempVar();
  code.push_back(new LoadConstant(result, value));
  return result;
}

Location *CodeGenerator::GenLoadConstant(const char *s)
{
  Location *result = GenTempVar();
  code.push_back(new LoadStringConstant(result, s));
  return result;
} 

Location *CodeGenerator::GenLoadLabel(const char *label)
{
  Location *result = GenTempVar();
  code.push_back(new LoadLabel(result, label));
  return result;
} 


void CodeGenerator::GenAssign(Location *dst, Location *src)
{
  code.push_back(new Assign(dst, src));
}


Location *CodeGenerator::GenLoad(Location *ref, int offset)
{
  Location *result = GenTempVar();
  code.push_back(new Load(result, ref, offset));
  return result;
}

void CodeGenerator::GenStore(Location *dst,Location *src, int offset)
{
  code.push_back(new Store(dst, src, offset));
}


Location *CodeGenerator::GenBinaryOp(const char *opName, Location *op1,
						     Location *op2)
{
  Location *result = GenTempVar();
  code.push_back(new BinaryOp(BinaryOp::OpCodeForName(opName), result, op1, op2));
  return result;
}


void CodeGenerator::GenLabel(const char *label)
{
  code.push_back(new Label(label));
}

void CodeGenerator::GenIfZ(Location *test, const char *label)
{
  code.push_back(new IfZ(test, label));
}

void CodeGenerator::GenGoto(const char *label)
{
  code.push_back(new Goto(label));
}

void CodeGenerator::GenReturn(Location *val)
{
  code.push_back(new Return(val));
}


BeginFunc *CodeGenerator::GenBeginFunc()
{
  BeginFunc *result = new BeginFunc;
  code.push_back(result);
  return result;
}

void CodeGenerator::GenEndFunc()
{
  code.push_back(new EndFunc());
}

void CodeGenerator::GenPushParam(Location *param)
{
  code.push_back(new PushParam(param));
}

void CodeGenerator::GenPopParams(int numBytesOfParams)
{
  Assert(numBytesOfParams >= 0 && numBytesOfParams % VarSize == 0); // sanity check
  if (numBytesOfParams > 0)
    code.push_back(new PopParams(numBytesOfParams));
}

Location *CodeGenerator::GenLCall(const char *label, bool fnHasReturnValue)
{
  Location *result = fnHasReturnValue ? GenTempVar() : NULL;
  code.push_back(new LCall(label, result));
  return result;
}

Location *CodeGenerator::GenACall(Location *fnAddr, bool fnHasReturnValue)
{
  Location *result = fnHasReturnValue ? GenTempVar() : NULL;
  code.push_back(new ACall(fnAddr, result));
  return result;
}
 
 
static struct _builtin {
  const char *label;
  int numArgs;
  bool hasReturn;
} builtins[] =
 {{"_Alloc", 1, true},
  {"_ReadLine", 0, true},
  {"_ReadInteger", 0, true},
  {"_StringEqual", 2, true},
  {"_PrintInt", 1, false},
  {"_PrintString", 1, false},
  {"_PrintBool", 1, false},
  {"_Halt", 0, false}};

Location *CodeGenerator::GenBuiltInCall(BuiltIn bn,Location *arg1, Location *arg2)
{
  Assert(bn >= 0 && bn < NumBuiltIns);
  struct _builtin *b = &builtins[bn];
  Location *result = NULL;

  if (b->hasReturn) result = GenTempVar();
                // verify appropriate number of non-NULL arguments given
  Assert((b->numArgs == 0 && !arg1 && !arg2)
	|| (b->numArgs == 1 && arg1 && !arg2)
	|| (b->numArgs == 2 && arg1 && arg2));
  if (arg2) code.push_back(new PushParam(arg2));
  if (arg1) code.push_back(new PushParam(arg1));
  code.push_back(new LCall(b->label, result));
  GenPopParams(VarSize*b->numArgs);
  return result;
}


void CodeGenerator::GenVTable(const char *className, List<const char *> *methodLabels)
{
  code.push_back(new VTable(className, methodLabels));
}


void CodeGenerator::DoFinalCodeGen()
{
  if (IsDebugOn("tac")) { // if debug don't translate to mips, just print Tac
    std::list<Instruction*>::iterator p;
    for (p= code.begin(); p != code.end(); ++p) {
      (*p)->Print();
    }
   }  else {
     Mips mips;
     mips.EmitPreamble();

    std::list<Instruction*>::iterator p;
    for (p= code.begin(); p != code.end(); ++p) {
      (*p)->Emit(&mips);
    }
  }
}

Location *CodeGenerator::GenNewArray(Location *numElems)
{
  Location *zero = GenLoadConstant(0);
  Location *isNegative = GenBinaryOp("<", numElems, zero);
  const char *pastError = NewLabel();
  GenIfZ(isNegative, pastError);
  GenMessage(err_arr_bad_size);
  GenBuiltInCall(Halt, NULL);
  GenLabel(pastError);
 
  Location *arraySize = GenLoadConstant(1);
  Location *num = GenBinaryOp("+", arraySize, numElems);
  Location *four = GenLoadConstant(VarSize);
  Location *bytes = GenBinaryOp("*", num, four);
  Location *result = GenBuiltInCall(Alloc, bytes);
  GenStore(result, numElems);
  return GenBinaryOp("+", result, four);
}

Location *CodeGenerator::GenArrayLen(Location *array)
{
  return GenLoad(array, -4);
}

Location* CodeGenerator::GenArrayAccess(Location* base, Location* subscript){
  Location *zero = GenLoadConstant(0);
  Location *isNegative = GenBinaryOp("<", subscript, zero);
  Location *size = GenLoad(base, -4);
  Location *lessthan =GenBinaryOp("<", subscript, size);
  Location *greaterthanzero = GenBinaryOp("==", lessthan, zero);
  Location *badsize = GenBinaryOp("||", isNegative, greaterthanzero);
  const char *pastError = NewLabel();
  GenIfZ(badsize, pastError);
  GenMessage(err_arr_out_of_bounds);
  GenBuiltInCall(Halt, NULL);

  GenLabel(pastError);
  Location *four = GenLoadConstant(VarSize);
  Location *offBytes = GenBinaryOp("*", four, subscript);
  Location *addr = GenBinaryOp("+", base, offBytes);
  Location *result = GenLoad(addr); //?
  return result;
}

void CodeGenerator::GenMessage(const char *message)
{
   Location *msg = GenLoadConstant(message);
   GenBuiltInCall(PrintString, msg);
}
