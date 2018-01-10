//---------------------------------------------------------------------------
//		E L E N A   P r o j e c t:  ELENA Tools
//
//		This is a main file containing ecode viewer code
//
//                                              (C)2012-2018, by Alexei Rakov
//---------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
// --------------------------------------------------------------------------
#include "elena.h"
#include "libman.h"
#include "module.h"
#include "config.h"
#include "bytecode.h"

#ifdef _WIN32

#include "winapi/consolehelper.h"

#endif // _WIN32

#define PROJECT_SECTION "project"
#define ROOTPATH_OPTION "libpath"

#define MAX_LINE           256
#define REVISION_VERSION   8

#define INT_CLASS                "system'IntNumber" 
#define LONG_CLASS               "system'LongNumber" 
#define REAL_CLASS               "system'RealNumber" 
#define STR_CLASS                "system'LiteralValue" 
#define WSTR_CLASS               "system'WideLiteralValue" 
#define CHAR_CLASS               "system'CharValue" 

using namespace _ELENA_;

// === Variables ===
MessageMap         _verbs;
ident_t _integer = INT_CLASS;
ident_t _long = LONG_CLASS;
ident_t _real = REAL_CLASS;
ident_t _literal = STR_CLASS;
ident_t _wide = WSTR_CLASS;
ident_t _char = CHAR_CLASS;

bool    _ignoreBreakpoints = true;

TextFileWriter* _writer;

// === Helper functions ===

// --- trim ---

inline ident_t trim(ident_t s)
{
   while (s[0]==' ')
      s+=1;

   return s;
}

// --- commands ---

void print(ident_t line)
{
   wprintf(WideString(line));
   if (_writer)
      _writer->writeLiteral(line);
}

void printLine(ident_t line1, ident_t line2)
{
   wprintf(WideString(line1));
   wprintf(WideString(line2));
   printf("\n");

   if (_writer) {
      _writer->writeLiteral(line1);
      _writer->writeLiteral(line2);
      _writer->writeNewLine();
   }
}

void printLine(ident_t line1, ident_t line2, ident_t line3, ident_t line4)
{
   wprintf(WideString(line1));
   wprintf(WideString(line2));
   wprintf(WideString(line3));
   wprintf(WideString(line4));
   printf("\n");

   if (_writer) {
      _writer->writeLiteral(line1);
      _writer->writeLiteral(line2);
      _writer->writeLiteral(line3);
      _writer->writeLiteral(line4);
      _writer->writeNewLine();
   }
}

void printLine()
{
   printf("\n");

   if (_writer) {
      _writer->writeNewLine();
   }
}

void printLoadError(LoadResult result)
{
   switch(result)
   {
   case lrNotFound:
      print("Module not found\n");
      break;
   case lrWrongStructure:
      print("Invalid module\n");
      break;
   case lrWrongVersion:
      print("Module out of date\n");
      break;
   }
}

void printHelp()
{
   printf("-b                      - hide / show breakpoints\n");
   printf("-q                      - quit\n");
   printf("-h                      - help\n");
   printf("?<class>.<method name>  - view method byte codes\n");
   printf("?<class>                - list all class methods\n");
   printf("??<symbol>              - view symbol byte codes\n");
   printf("-o<path>                - save the output\n");
   printf("-l                      - list all classes with methods\n");
   printf("?                       - list all classes\n");
}

_Memory* findClassMetaData(_Module* module, ident_t referenceName)
{
   ref_t reference = module->mapReference(referenceName, true);
   if (reference == 0) {
      return NULL;
   }
   return module->mapSection(reference | mskMetaRDataRef, true);
}

_Memory* findClassVMT(_Module* module, ident_t referenceName)
{
   ref_t reference = module->mapReference(referenceName, true);
   if (reference == 0) {
      return NULL;
   }
   return module->mapSection(reference | mskVMTRef, true);
}

_Memory* findClassCode(_Module* module, ident_t referenceName)
{
   ref_t reference = module->mapReference(referenceName, true);
   if (reference == 0) {
      return NULL;
   }
   return module->mapSection(reference | mskClassRef, true);
}

_Memory* findSymbolCode(_Module* module, ident_t referenceName)
{
   ref_t reference = module->mapReference(referenceName, true);
   if (reference == 0) {
      return NULL;
   }
   return module->mapSection(reference | mskSymbolRef, true);
}

ref_t resolveMessage(_Module* module, ident_t method)
{
   int paramCount = 0;
   ref_t actionRef = 0;
   ref_t flags = 0;

   if (method.startsWith("#private&")) {
      flags |= SEALED_MESSAGE;

      method = method.c_str() + getlength("#private&");
   }
   else if (method.startsWith("#conversion&")) {
      flags |= CONVERSION_MESSAGE;

      method = method.c_str() + getlength("#conversion&");
   }

   IdentifierString actionName;
   int paramIndex = method.find('[', -1);
   if (paramIndex != -1) {
      actionName.copy(method, paramIndex);

      IdentifierString countStr(method + paramIndex + 1, getlength(method) - paramIndex - 2);
      paramCount = countStr.ident().toInt();
   }
   else actionName.copy(method);

   if (actionName.compare("dispatch")) {
      actionRef = DISPATCH_MESSAGE_ID;
   }
   else if (actionName.compare("#new")) {
      actionRef = NEWOBJECT_MESSAGE_ID;
   }
   else if (actionName.compare("#init")) {
      actionRef = NEWOBJECT_MESSAGE_ID;
      flags = CONVERSION_MESSAGE;
   }
   else {
      if (method.find("set&") != NOTFOUND_POS) {
         actionName.cut(0, 4);
         flags = PROPSET_MESSAGE;
      }
      else if (actionName.compare("set")) {
         flags = PROPSET_MESSAGE;
      }

      actionRef = module->mapSubject(actionName, true);
      if (actionRef == 0) {
         printLine("Unknown subject ", actionName);

         return 0;
      }
   }

   return encodeMessage(actionRef, paramCount) | flags;
}

inline void appendHex32(IdentifierString& command, unsigned int hex)
{
   unsigned int n = hex / 0x10;
   int len = 7;
   while (n > 0) {
      n = n / 0x10;

      len--;
   }

   while (len > 0) {
      command.append('0');
      len--;
   }

   command.appendHex(hex);
}

int getLabelIndex(int label, List<int>& labels)
{
   int index = 0;
   List<int>::Iterator it = labels.start();
   while (!it.Eof()) {
      if (*it == label)
         return index;

      index++;
      it++;
   }

   return -1;
}

void printLabel(IdentifierString& command, int labelPosition, List<int>& labels)
{
   int index = getLabelIndex(labelPosition, labels);
   if (index == -1) {
      index = labels.Count();

      labels.add(labelPosition);
   }

   command.append("Lab");
   if (index < 10) {
      command.append('0');
   }
   command.appendInt(index);
}

void parseMessageConstant(IdentifierString& message, ident_t reference)
{
   // message constant: nverb&signature

   int verbId = 0;
   int signatureId = 0;

   // read the param counter
   int count = reference[0] - '0';

   // skip the param counter
   reference+=1;

   int index = reference.find('&');
   //HOTFIX: for generic GET message we have to ignore ampresand
   if (reference[index + 1] == 0)
      index = -1;

   if (index != -1) {
      //HOTFIX: for GET message we have &&, so the second ampersand should be used
      if (reference[index + 1] == 0 || reference[index + 1] == '&')
         index++;

      IdentifierString verb(reference, index);
      ident_t signature = reference + index + 1;

      // if it is a predefined verb
      if (verb[0] == '#') {
         verbId = verb[1] - 0x20;
      }

      message.append(retrieveKey(_verbs.start(), verbId, DEFAULT_STR));
      message.append(signature);
   }
   else {
      // if it is a predefined verb
      if (reference[0] == '#') {
         verbId = reference[1] - 0x20;

         message.append(retrieveKey(_verbs.start(), verbId, DEFAULT_STR));
      }
      else message.append(reference);
   }
}

void printReference(IdentifierString& command, _Module* module, size_t reference)
{
   bool literalConstant = false;
   bool charConstant = false;
   ident_t referenceName = NULL;
   int mask = reference & mskAnyRef;
   if (mask == mskInt32Ref) {
      referenceName = _integer;
      literalConstant = true;
   }
   else if (mask == mskInt64Ref) {
      referenceName = _long;
      literalConstant = true;
   }
   else if (mask == mskLiteralRef) {
      referenceName = _literal;
      literalConstant = true;
   }
   else if (mask == mskWideLiteralRef) {
      referenceName = _wide;
      literalConstant = true;
   }
   else if (mask == mskRealRef) {
      referenceName = _real;
      literalConstant = true;
   }
   else if (mask == mskCharRef) {
      referenceName = _char;
      charConstant = true;
   }
   else if (reference == 0) {
      referenceName = "$nil";
   }
   else if (reference == -1) {
      referenceName = "$undefined";
   }
   else referenceName = module->resolveReference(reference & ~mskAnyRef);

   if (emptystr(referenceName)) {
      command.append("unknown");
   }
   else {
      command.append(referenceName);
      if (literalConstant) {
         command.append("(");
         command.append(module->resolveConstant(reference & ~mskAnyRef));
         command.append(")");
      }
      else if (charConstant) {
         const char* ch = module->resolveConstant(reference & ~mskAnyRef);

         IdentifierString num;
         num.appendInt(ch[0]);
         command.append("(");
         command.append(num);
         command.append(")");

      }
   }
}

void printMessage(IdentifierString& command, _Module* module, size_t reference)
{
   ref_t actionRef = 0;
   int paramCount = 0;
   decodeMessage(reference, actionRef, paramCount);

   if (actionRef == DISPATCH_MESSAGE_ID) {
      command.append("dispatch");
   }
   else if (actionRef == NEWOBJECT_MESSAGE_ID) {
      if (test(reference, CONVERSION_MESSAGE)) {
         command.append("#init");
      }
      else command.append("#new");
   }
   else if (actionRef <= PREDEFINED_MESSAGE_ID) {
      if (test(reference, CONVERSION_MESSAGE)) {
         command.append("#conversion&");
      }
      else if (test(reference, SEALED_MESSAGE)) {
         command.append("#private&");
      }

      ident_t verbName = retrieveKey(_verbs.start(), actionRef, DEFAULT_STR);
      command.append(verbName);
   }
   else {
      if (test(reference, CONVERSION_MESSAGE)) {
         command.append("#conversion&");
      }
      else {
         if (test(reference, SEALED_MESSAGE)) {
            command.append("#private&");
         }
         if (test(reference, PROPSET_MESSAGE)) {
            command.append("set&");
         }
      }
      ident_t subjectName = module->resolveSubject(actionRef);
      command.append(subjectName);
   }

   if (paramCount > 0) {
      command.append('[');
      command.appendInt(paramCount);
      command.append(']');
   }
}

bool printCommand(_Module* module, MemoryReader& codeReader, int indent, List<int>& labels)
{
   // read bytecode + arguments
   int position = codeReader.Position();
   unsigned char code = codeReader.getByte();

   // ignore a breakpoint if required
   if (code == bcBreakpoint && _ignoreBreakpoints)
      return false;

   char opcode[0x30];
   ByteCodeCompiler::decode((ByteCode)code, opcode);

   IdentifierString command;
   while (indent > 0) {
      command.append(" ");

      indent--;
   }
   if (code < 0x10)
      command.append('0');

   command.appendHex((int)code);
   command.append(' ');

   int argument = 0;
   int argument2 = 0;
   if (code > MAX_DOUBLE_ECODE) {
      argument = codeReader.getDWord();
      argument2 = codeReader.getDWord();

      appendHex32(command, argument);
      command.append(' ');

      appendHex32(command, argument2);
      command.append(' ');
   }
   else if (code > MAX_SINGLE_ECODE) {
      argument = codeReader.getDWord();

      appendHex32(command, argument);
      command.append(' ');
   }

   size_t tabbing = code == bcNop ? 24 : 31;
   while (getlength(command) < tabbing) {
      command.append(' ');
   }

   switch(code)
   {
      case bcPushF:
      case bcSCopyF:
      case bcACopyF:
      case bcBCopyF:
         command.append(opcode);
         command.append(" fp:");
         command.appendInt(argument);
         break;
      case bcACopyS:
         command.append(opcode);
         command.append(" sp:");
         command.appendInt(argument);
         break;
      case bcJump:
      case bcHook:
      case bcIf:
      case bcIfB:
      case bcElse:
      case bcIfHeap:
      case bcNotLess:
//      case bcAddress:
         command.append(opcode);
         command.append(' ');
         printLabel(command, position + argument + 5, labels);
         break;
      case bcElseM:
      case bcIfM:
         command.append(opcode);
         command.append(' ');
         printMessage(command, module, argument);
         command.append(' ');
         printLabel(command, position + argument2 + 9, labels);
         break;
      case bcElseR:
      case bcIfR:
         command.append(opcode);
         command.append(' ');
         printReference(command, module, argument);
         command.append(' ');
         printLabel(command, position + argument2 + 9, labels);
         break;
      case bcIfN:
      case bcElseN:
      case bcLessN:
      case bcNotLessN:
      case bcGreaterN:
      case bcNotGreaterN:
         command.append(opcode);
         command.append(' ');
         command.appendHex(argument);
         command.append(' ');
         printLabel(command, position + argument2 + 9, labels);
         break;
      case bcNop:
         printLabel(command, position + argument, labels);
         command.append(':');
         command.append(' ');
         command.append(opcode);
         break;
      case bcPushR:
      case bcALoadR:
      case bcCallExtR:
      case bcCallR:
      case bcASaveR:
      case bcACopyR:
      case bcBCopyR:
         command.append(opcode);
         command.append(' ');
         printReference(command, module, argument);
         break;
      case bcReserve:
      case bcRestore:
      case bcPushN:
      case bcPopI:
      case bcOpen:
      case bcQuitN:
      case bcDCopy:
      case bcECopy:
      case bcAndN:
      case bcOrN:
      case bcInit:
      case bcNLoadI:
      case bcNSaveI:
      case bcMulN:
      case bcAddN:
         command.append(opcode);
         command.append(' ');
         command.appendHex(argument);
         break;
      case bcPushSI:
      case bcALoadSI:
      case bcASaveSI:
      case bcBLoadSI:
      case bcBSaveSI:
         command.append(opcode);
         command.append(" sp[");
         command.appendInt(argument);
         command.append(']');
         break;
      case bcBLoadFI:
      case bcPushFI:
      case bcALoadFI:
      case bcASaveFI:
      case bcDLoadFI:
         command.append(opcode);
         command.append(" fp[");
         command.appendInt(argument);
         command.append(']');
         break;
      case bcAJumpVI:
      case bcACallVI:
         command.append(opcode);
         command.append(" acc::vmt[");
         command.appendInt(argument);
         command.append(']');
         break;
      case bcPushAI:
      case bcALoadAI:
         command.append(opcode);
         command.append(" acc[");
         command.appendInt(argument);
         command.append(']');
         break;
      case bcASaveBI:
      case bcAXSaveBI:
      case bcALoadBI:
         command.append(opcode);
         command.append(" base[");
         command.appendInt(argument);
         command.append(']');
         break;
      case bcNew:
         command.append(opcode);
         command.append(' ');
         printReference(command, module, argument);
         command.append(", ");
         command.appendInt(argument2);
         break;
      case bcXCallRM:
      case bcXJumpRM:
      case bcXIndexRM:
         command.append(opcode);
         command.append(' ');
         printReference(command, module, argument);
         command.append(", ");
         printMessage(command, module, argument2);
         break;
      case bcCopyM:
      case bcSetVerb:
         command.append(opcode);
         command.append(' ');
         printMessage(command, module, argument);
         break;
      case bcSelectR:
      case bcXSelectR:
         command.append(opcode);
         command.append(' ');
         printReference(command, module, argument);
         command.append(", ");
         printReference(command, module, argument2);
         break;
      case bcNewN:
         command.append(opcode);
         command.append(' ');
         printReference(command, module, argument);
         command.append(", ");
         command.appendInt(argument2);
         break;
      case bcSaveFI:
      case bcAddFI:
      case bcSubFI:
         command.append(opcode);
         command.append(" fp[");
         command.appendInt(argument);
         command.append("], ");
         command.appendInt(argument2);
         break;
      default:
         command.append(opcode);
         break;
   }

   print(command);
   return true;
}

void printByteCodes(_Module* module, _Memory* code, ref_t address, int indent, int pageSize)
{
   MemoryReader codeReader(code, address);

   size_t codeSize = codeReader.getDWord();
   size_t endPos = codeReader.Position() + codeSize;

   int row = 1;
   List<int> labels;
   while(codeReader.Position() < endPos) {
      if (printCommand(module, codeReader, indent, labels)) {
         print("\n");

         row++;
         if (row == pageSize - 1) {
            printf("Press any key to continue...");
            ConsoleHelper::getChar();
            printf("\n");

            row = 0;
         }
      }      
   }
}

ref_t resolveMessageByIndex(_Module* module, ident_t className, int index)
{
   // find class VMT
   ReferenceNs reference(module->Name(), className);
   _Memory* vmt = findClassVMT(module, reference);
   if (vmt == NULL) {
      return 0;
   }

   // list methods
   MemoryReader vmtReader(vmt);
   // read tape record size
   size_t size = vmtReader.getDWord();

   // read VMT header
   ClassHeader header;
   vmtReader.read((void*)&header, sizeof(ClassHeader));

   VMTEntry        entry;

   size -= sizeof(ClassHeader);
   IdentifierString temp;
   int row = 0;
   while (size > 0) {
      vmtReader.read((void*)&entry, sizeof(VMTEntry));

      index--;
      if (index == 0) {
         IdentifierString temp;
         printMessage(temp, module, entry.message);

         return resolveMessage(module, temp.c_str());
      }

      size -= sizeof(VMTEntry);
   }

   return 0;
}

void printMethod(_Module* module, ident_t methodReference, int pageSize)
{
   methodReference = trim(methodReference);

   int separator = methodReference.find('.');
   if (separator == -1) {
      printf("Invalid command");

      return;
   }

   IdentifierString className(methodReference, separator);

   ident_t methodName = methodReference + separator + 1;
   ref_t message = 0;

   // resolve method
   if (methodName[0] == '$' && methodName[1] >= '0' && methodName[1] <= '9') {
      message = resolveMessageByIndex(module, className.ident(), methodName.toInt(1));
   }
   else message = resolveMessage(module, methodName);
   
   if (message == 0)
      return;

   // find class VMT
   ReferenceNs reference(module->Name(), className);
   _Memory* vmt = findClassVMT(module, reference);
   _Memory* code = findClassCode(module, reference);
   if (vmt == NULL || code == NULL) {
      printLine("Class not found: ", reference);

      return;
   }

   // find method entry
   MemoryReader vmtReader(vmt);
   // read tape record size
   size_t size = vmtReader.getDWord();

   // read VMT header
   ClassHeader header;
   vmtReader.read((void*)&header, sizeof(ClassHeader));

   VMTEntry        entry;

   // read VMT while the entry not found
   size -= sizeof(ClassHeader);
   bool found = false;
   while (size > 0) {
      vmtReader.read((void*)&entry, sizeof(VMTEntry));

      if (entry.message == message) {
         found = true;

         IdentifierString temp;
         temp.copy(className);
         temp.append('.');
         printMessage(temp, module, entry.message);
         printLine("@method ", temp);

         printByteCodes(module, code, entry.address, 4, pageSize);
         print("@end\n");

         break;
      }

      size -= sizeof(VMTEntry);
   }
   if (!found) {
      printLine("Method not found:", methodName);
   }
}

//void printConstructor(_Module* module, const wchar_t* className, int pageSize)
//{
//   className = trim(className);
//
//   // find class VMT
//   ReferenceNs reference(module->Name(), className);
//   _Memory* vmt = findClassVMT(module, reference);
//   _Memory* code = findClassCode(module, reference);
//   if (vmt == NULL || code == NULL) {
//      wprintf(_T("Class %s not found\n"), (const wchar_t*)reference);
//
//      return;
//   }
//
//   ClassInfo info;
//   loadClassInfo(module, className, info);
//
//   if (info.constructor != 0) {
//      print(_T("@constructor\n"));
//      printByteCodes(module, code, 0, 4, pageSize);
//      print(_T("@end\n"));
//   }
//   else {
//      print(_T("Constructor is not available\n"));
//   }
//}

void printSymbol(_Module* module, ident_t symbolReference, int pageSize)
{
   // find class VMT
   ReferenceNs reference(module->Name(), symbolReference);
   _Memory* code = findSymbolCode(module, reference);
   if (code == NULL) {
      printLine("Symbol not found:", reference);

      return;
   }

   printLine("@symbol ", symbolReference);
   printByteCodes(module, code, 0, 4, pageSize);
   print("@end\n");
}

bool loadClassInfo(_Module* module, ident_t reference, ClassInfo& info)
{
   // find class meta data
   _Memory* data = findClassMetaData(module, reference);
   if (data == NULL) {
      printLine("Class not found:", reference);

      return false;
   }

   MemoryReader reader(data);
   info.load(&reader);

   return true;
}

void listFields(_Module* module, ReferenceNs className)
{
   ClassInfo info;
   if (!loadClassInfo(module, className, info)) {
      return;
   }
   
   ClassInfo::FieldMap::Iterator it = info.fields.start();
   while (!it.Eof()) {
      ref_t type = info.fieldTypes.get(*it).value1;
      if (type != 0) {
         ident_t typeName = module->resolveReference(type);

         printLine("Field ", (const char*)it.key(), " of ", typeName);
      }
      else printLine("Field ", (const char*)it.key());
   


      it++;
   }
}

void listFlags(int flags)
{
   if (test(flags, elNestedClass))
      printLine("@flag ", "elNestedClass");

   if (test(flags, elDynamicRole))
      printLine("@flag ", "elDynamicRole");

   if (test(flags, elStructureRole))
      printLine("@flag ", "elStructureRole");

   if (test(flags, elSealed))
      printLine("@flag ", "elSealed");
   else if (test(flags, elClosed))
      printLine("@flag ", "elClosed");

   if (test(flags, elWrapper))
      printLine("@flag ", "elWrapper");

   if (test(flags, elStateless))
      printLine("@flag ", "elStateless");

   if (test(flags, elGroup))
      printLine("@flag ", "elGroup");

   if (test(flags, elWithGenerics))
      printLine("@flag ", "elWithGenerics");

   if (test(flags, elWithArgGenerics))
      printLine("@flag ", "elWithArgGenerics");

   if (test(flags, elReadOnlyRole))
      printLine("@flag ", "elReadOnlyRole");

   if (test(flags, elNonStructureRole))
      printLine("@flag ", "elNonStructureRole");

   if (test(flags, elSignature))
      printLine("@flag ", "elSignature");

   if (test(flags, elRole))
      printLine("@flag ", "elRole");

   if (test(flags, elExtension))
      printLine("@flag ", "elExtension");

   if (test(flags, elMessage))
      printLine("@flag ", "elMessage");

   if (test(flags, elExtMessage))
      printLine("@flag ", "elExtMessage");

   if (test(flags, elSymbol))
      printLine("@flag ", "elSymbol");

   if (test(flags, elWithMuti))
      printLine("@flag ", "elWithMuti");

   if (test(flags, elClassClass))
      printLine("@flag ", "elClassClass");

   switch (flags & elDebugMask) {
      case elDebugDWORD:
         printLine("@flag ", "elDebugDWORD");
         break;
      case elDebugReal64:
         printLine("@flag ", "elDebugReal64");
         break;
      case elDebugLiteral:
         printLine("@flag ", "elDebugLiteral");
         break;
      case elDebugIntegers:
         printLine("@flag ", "elDebugIntegers");
         break;
      case elDebugArray:
         printLine("@flag ", "elDebugArray");
         break;
      case elDebugQWORD:
         printLine("@flag ", "elDebugQWORD");
         break;
      case elDebugBytes:
         printLine("@flag ", "elDebugBytes");
         break;
      case elDebugShorts:
         printLine("@flag ", "elDebugShorts");
         break;
      //case elDebugPTR:
      //   printLine("@flag ", "elDebugPTR");
      //   break;
      case elDebugWideLiteral:
         printLine("@flag ", "elDebugWideLiteral");
         break;
      case elDebugReference:
         printLine("@flag ", "elDebugReference");
         break;
      case elDebugSubject:
         printLine("@flag ", "elDebugSubject");
         break;
      //case elDebugReals:
      //   printLine("@flag ", "elDebugReals");
      //   break;
      case elDebugMessage:
         printLine("@flag ", "elDebugMessage");
         break;
      //case elDebugDPTR:
      //   printLine("@flag ", "elDebugDPTR");
      //   break;
      case elEnumList:
         printLine("@flag ", "elEnumList");
         break;
   }
}

void listConstructorMethods(_Module* module, ident_t className, ref_t reference)
{
   _Memory* vmt = module->mapSection(reference | mskVMTRef, true);

   // list methods
   MemoryReader vmtReader(vmt);
   // read tape record size
   size_t size = vmtReader.getDWord();

   ClassHeader header;
   vmtReader.read((void*)&header, sizeof(ClassHeader));

   VMTEntry        entry;

   size -= sizeof(ClassHeader);
   IdentifierString temp;
   while (size > 0) {
      vmtReader.read((void*)&entry, sizeof(VMTEntry));

      // print the method name
      temp.copy(className);
      temp.append('.');
      printMessage(temp, module, entry.message);
      printLine("@constructor ", temp);

      size -= sizeof(VMTEntry);
   }
}

void listClassMethods(_Module* module, ident_t className, int pageSize, bool fullInfo, bool withConstructors)
{
   className = trim(className);

   // find class VMT
   ReferenceNs reference(module->Name(), className);
   _Memory* vmt = findClassVMT(module, reference);
   if (vmt == NULL) {
      printLine("Class not found:", reference);

      return;
   }

   // list methods
   MemoryReader vmtReader(vmt);
   // read tape record size
   size_t size = vmtReader.getDWord();

   // read VMT info
   ClassHeader header;
   vmtReader.read((void*)&header, sizeof(ClassHeader));

   if (fullInfo) {
      if (header.parentRef)
         printLine("@parent ", module->resolveReference(header.parentRef));

      listFlags(header.flags);
      listFields(module, reference);
   }

   if (header.classRef != 0 && withConstructors) {
      listConstructorMethods(module, className, header.classRef);
   }

   VMTEntry        entry;

   size -= sizeof(ClassHeader);
   IdentifierString temp;
   int row = 0;
   while (size > 0) {
      vmtReader.read((void*)&entry, sizeof(VMTEntry));

      // print the method name
      temp.copy(className);
      temp.append('.');
      printMessage(temp, module, entry.message);
      printLine("@method ", temp);

      row++;
      if (row == pageSize) {
         print("Press any key to continue...");
         _fgetchar();
         printf("\n");

         row = 0;
      }

      size -= sizeof(VMTEntry);
   }
}

void printAPI(_Module* module, int pageSize)
{
   ident_t moduleName = module->Name();

   ReferenceMap::Iterator it = ((Module*)module)->References();
   while (!it.Eof()) {
      ident_t reference = it.key();
      NamespaceName ns(it.key());
      if (moduleName.compare(ns)) {
         ReferenceName name(it.key());
         if (module->mapSection(*it | mskVMTRef, true)) {
            printLine("class ", name);

            listClassMethods(module, name, pageSize, true, true);
            printLine();
         }
         else if (module->mapSection(*it | mskSymbolRef, true)) {
            printLine("symbol ", name);
         }
      }

      it++;
   }
}

void listClasses(_Module* module, int pageSize)
{
   ident_t moduleName = module->Name();

   int row = 0;
   ReferenceMap::Iterator it = ((Module*)module)->References();
   while (!it.Eof()) {
      ident_t reference = it.key();
      NamespaceName ns(it.key());
      if (moduleName.compare(ns)) {
         ReferenceName name(it.key());
         if (module->mapSection(*it | mskVMTRef, true)) {
            printLine("class ", name);
            row++;
         }
         else if (module->mapSection(*it | mskSymbolRef, true)) {
            printLine("symbol ", name);
            row++;
         }
         
         if (row == pageSize - 1) {
            printf("Press any key to continue...");
            _fgetchar();
            printf("\n");

            row = 0;
         }
      }

      it++;
   }
}

void setOutputMode(path_t path)
{
   if (_writer)
      freeobj(_writer);

   _writer = new TextFileWriter(path, 0, false);
}

void runSession(_Module* module, int pageSize)
{
   char              buffer[MAX_LINE];
   IdentifierString  line;
   while (true) {
      printf("\n>");

      // !! fgets is used instead of fgetws, because there is strange bug in fgetws implementation
      fgets(buffer, MAX_LINE, stdin);
      line.copy(buffer, strlen(buffer));

      while (!emptystr(line) && line[getlength(line) - 1]=='\r' || line[getlength(line) - 1]=='\n')
         line[getlength(line) - 1] = 0;

      while (!emptystr(line) && line[getlength(line) - 1]==' ')
         line[getlength(line) - 1] = 0;

      // execute command
      if (line[0]=='?') {
         if (line[1]=='?') {
            printSymbol(module, line + 2, pageSize);
         }
         else if (line.ident().findSubStr(1, '.', getlength(line) - 1, -1) != -1) {
            printMethod(module, line + 1, pageSize);   
         }
         else if (line[1]==0) {
            listClasses(module, pageSize);
         }
         else listClassMethods(module, line + 1, pageSize, true, false);
      }
      else if (line[0]=='-') {
         switch(line[1]) {
            case 'q':
               return;
            case 'h':
               printHelp();
               break;
            case 'l':
               printAPI(module, pageSize);
               break;
            case 'b':
               _ignoreBreakpoints = !_ignoreBreakpoints;
               break;
            //case 'c':
            //   printConstructor(module, line + 2, pageSize);
            //   break;
            case 'o':
            {
               Path path(line + 2);
               setOutputMode(path.c_str());
               break;
            }
            default:
               printHelp();
         }
      }
      else printHelp();
   }
}

const char* manifestParameters[4] = { "namespace","name     ","version  ","author   " };

void printManifest(_Module* module)
{
   ReferenceNs name(module->Name(), PACKAGE_SECTION);

   _Memory* section = module->mapSection(module->mapReference(name, false) | mskRDataRef, true);
   if (section != NULL) {

      _ELENA_::RelocationMap::Iterator it(section->getReferences());
      ref_t currentMask = 0;
      ref_t currentRef = 0;
      while (!it.Eof()) {
         int i = *it >> 2;
         currentMask = it.key() & mskAnyRef;
         currentRef = it.key() & ~mskAnyRef;

         if (currentMask == mskLiteralRef) {
            //printf(manifestParameters[i]);
            ident_t value = module->resolveConstant(currentRef);
            printf("%s : %s\n", manifestParameters[i], value.c_str());
         }
         it++;
      }
   }
}

// === Main Program ===
int main(int argc, char* argv[])
{
   printf("ELENA command line ByteCode Viewer %d.%d.%d (C)2011-2016 by Alexei Rakov\n", ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION, REVISION_VERSION);

   if (argc<2) {
      printf("ecv <module name> | ecv -p<module path>");
      return 0;
   }

   // define the console size for pagination
   int columns = 0, rows = 30;
   ConsoleHelper::getConsoleSize(columns, rows);

   // prepare library manager
   Path configPath("templates\\lib.cfg");
   Path rootPath("..\\lib33");

   // get viewing module name
   IdentifierString buffer(argv[1]);
   ident_t moduleName = buffer;

   //// load config attributes
   //IniConfigFile config;
   //if (config.load(configPath, feUTF8)) {
   //   Path::loadPath(rootPath, config.getSetting(PROJECT_SECTION, ROOTPATH_OPTION, DEFAULT_STR));
   //}

   LibraryManager loader(rootPath.c_str(), NULL);
   LoadResult result = lrNotFound;
   _Module* module = NULL;

   // if direct path is provieded
   if (moduleName[0]=='-' && moduleName[1]=='p' || moduleName.find('.') != NOTFOUND_POS) {
      if (moduleName[0] == '-')
         moduleName += 2;

      Path path;
      path.copySubPath(moduleName);
      FileName fileName(moduleName);

      IdentifierString name(fileName);
      loader.setNamespace(name, path.c_str());
      module = loader.loadModule(name, result, false);
   }
   else module = loader.loadModule(moduleName, result, false);

   if (result != lrSuccessful) {
      printLoadError(result);

      return -1;
   }
   else {
      printLine(moduleName, " module loaded");

      printManifest(module);
   }

   ByteCodeCompiler::loadVerbs(_verbs);

   runSession(module, rows);

   if (_writer)
      freeobj(_writer);

   return 0;
}
