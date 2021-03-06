#include "elena.h"
// --------------------------------------------------------------------------
#include "elenavm.h"
#include "x86elenavmachine.h"

using namespace _ELENA_;

static x86ELENAVMMachine* _Machine = NULL;
//static Path rootPath;
static void* _SystemEnv = NULL;
static void* _Args = NULL;

// --- initmachine ---

void initMachine(path_t rootPath)
{
   _Machine = new x86ELENAVMMachine(rootPath);

 /*  if (::IsDebuggerPresent()) {
      _Machine->getInstance()->setDebugMode();
   }*/
}

//// --- freeMachine ---
//
//void freeMachine()
//{
//   freeobj(_Machine);
//   _Machine = nullptr;
//}

//// --- getAppPath ---
//
//void loadDLLPath(HMODULE hModule)
//{
//   TCHAR path[MAX_PATH + 1];
//
//   ::GetModuleFileName(hModule, path, MAX_PATH);
//
//   rootPath.copySubPath(path);
//   rootPath.lower();
//}

// ==== DLL entries ====

// == Linux specific routines ==

int l_core_getargc()
{
   int* ptr = (int*)_Args;

   return *ptr;
}

int l_core_getarg(int index, char* buffer, int length)
{
   if (index <= 0)
      return 0;

   const char** args = (const char**)_Args;

   for (int i = 0; i < length; i++) {
      char tmp = args[index][i];

      buffer[i] = tmp;

      if (!tmp) {
         return i;
      }
   }

   return length;
}

// == ELENA run-time routines ==

/// Is used to initialize command argument list reference
void PrepareEM(void* args)
{
   _Args = args;
}

void InitializeVMSTA(void* sehTable, void* systemEnv, void* exceptionHandler, void* criticalHandler, void* vmTape,
   ProgramHeader* header)
{
   header->root_exception_struct.core_catch_addr = (pos_t)exceptionHandler;

   // initialize the critical exception handler
   if (criticalHandler != nullptr)
      __routineProvider.InitCriticalStruct(&header->root_critical_struct, (pos_t)criticalHandler);

   // initialize system env variable
   _SystemEnv = systemEnv;

   if (_Machine == nullptr)
      initMachine("/usr/lib/elena");

   // start the system
   _Machine->startSTA(header, (SystemEnv*)systemEnv, sehTable, vmTape);
}

void OpenFrame(void* systemEnv, void* frameHeader)
{
   SystemRoutineProvider::OpenFrame((SystemEnv*)systemEnv, (FrameHeader*)frameHeader);
}

void CloseFrame(void* systemEnv, void* frameHeader)
{
   SystemRoutineProvider::CloseFrame((SystemEnv*)systemEnv, (FrameHeader*)frameHeader);
}

void Exit(int exitCode)
{
   _Machine->Exit(exitCode);
}

// !! temporally

//EXTERN_DLL_EXPORT int ReadCallStack(void* instance, size_t framePosition, size_t currentAddress, size_t startLevel, int* buffer, size_t maxLength)
//{
//   return 0; // !! temporally
//}

int LoadAddressInfo(void* retPoint, char* buffer, size_t maxLength)
{
   Instance* instance = _Machine->getInstance();
   if (instance == NULL)
      return 0;

   try {
      if (instance->loadAddressInfo(retPoint, buffer, maxLength)) {
         return maxLength;
      }
      else return 0;
   }
   catch (JITUnresolvedException& e)
   {
      instance->setStatus("Cannot load ", e.referenceInfo);

      return 0;
   }
   catch (InternalError& e)
   {
      instance->setStatus(e.message);

      return 0;
   }
   catch (EAbortException&)
   {
      return 0;
   }
}

int LoadClassName(void* vmtAddress, char* buffer, int maxLength)
{
   Instance* instance = _Machine->getInstance();
   if (instance == NULL)
      return 0;

   try {
      size_t length = maxLength;

      int packagePtr = *(int*)((int)vmtAddress - 24);
      int namePtr = *(int*)((int)vmtAddress - 20);

      char* name = (char*)namePtr;
      char* ns = ((char**)packagePtr)[0];

      size_t ns_len = length;
      if (!ident_t(ns).copyTo(buffer, ns_len))
         return 0;

      maxLength -= ns_len;
      if (!ident_t(name).copyTo(buffer + ns_len, length))
         return 0;

      return length + ns_len;


      //ident_t className = instance->getClassName(vmtAddress);
      //size_t length = getlength(className);
      //if (length > 0) {
      //   if (maxLength >= (int)length) {
      //      Convertor::copy(buffer, className, length, length);
      //   }
      //   else buffer[0] = 0;
      //}

      //return length;
   }
   catch (JITUnresolvedException& e)
   {
      instance->setStatus("Cannot load ", e.referenceInfo);

      return 0;
   }
   catch (InternalError& e)
   {
      instance->setStatus(e.message);

      return 0;
   }
   catch (EAbortException&)
   {
      return 0;
   }
}

int LoadSubjectName(void* subjectRef, char* buffer, int maxLength)
{
   Instance* instance = _Machine->getInstance();
   if (instance == NULL)
      return 0;

   try {
      size_t subj_id = (size_t)subjectRef;

      ident_t subjectName = instance->getSubject(subj_id);
      size_t length = getlength(subjectName);
      if (length > 0) {
         if (maxLength >= (int)length) {
            Convertor::copy(buffer, subjectName, length, length);
         }
         else buffer[0] = 0;
      }

      return length;
   }
   catch (JITUnresolvedException& e)
   {
      instance->setStatus("Cannot load ", e.referenceInfo);

      return 0;
   }
   catch (InternalError& e)
   {
      instance->setStatus(e.message);

      return 0;
   }
   catch (EAbortException&)
   {
      return 0;
   }
}

void* LoadSubject(void* subjectName)
{
   Instance* instance = _Machine->getInstance();
   if (instance == NULL)
      return 0;

   try {
      ref_t subj_id = instance->getSubjectRef((const char*)subjectName);

      return (void*)subj_id;
   }
   catch (JITUnresolvedException& e)
   {
      instance->setStatus("Cannot load ", e.referenceInfo);

      return 0;
   }
   catch (InternalError& e)
   {
      instance->setStatus(e.message);

      return 0;
   }
   catch (EAbortException&)
   {
      return 0;
   }
}

int LoadMessageName(void* message, char* buffer, int maxLength)
{
   Instance* instance = _Machine->getInstance();
   if (instance == NULL)
      return 0;

   try {
      ref_t action, flags;
      int count;
      decodeMessage((ref_t)message, action, count, flags);

      size_t used = 0;
      //if (test((ref_t)message, encodeAction(SIGNATURE_FLAG))) {
      //   ImageSection messageSection;
      //   messageSection.init(_messageSection, 0x10000); // !! dummy size

      //   ref_t verb = messageSection[action];
      //   used += manager.readSubjectName(reader, verb, buffer + used, length - used);
      //}
      //else {
         ident_t subjectName = instance->getSubject(action);
         size_t length = getlength(subjectName);
         if (length > 0) {
            if (maxLength >= (int)(length + used)) {
               Convertor::copy(buffer + used, subjectName, length, length);

               used += length;
            }
            else buffer[used] = 0;
         }
      //}

      if (count > 0) {
         size_t dummy = 10;
         String<char, 10>temp;
         temp.appendInt(count);

         buffer[used++] = '[';
         Convertor::copy(buffer + used, temp, getlength(temp), dummy);
         used += dummy;
         buffer[used++] = ']';
      }
      buffer[used] = 0;

      return used;
   }
   catch (JITUnresolvedException& e)
   {
      instance->setStatus("Cannot load ", e.referenceInfo);

      return 0;
   }
   catch (InternalError& e)
   {
      instance->setStatus(e.message);

      return 0;
   }
   catch (EAbortException&)
   {
      return 0;
   }
}

void* LoadMessage(void* messageName)
{
   Instance* instance = _Machine->getInstance();
   if (instance == NULL)
      return 0;

   try {
      return (void*)(instance->getMessageRef((const char*)messageName));
   }
   catch (JITUnresolvedException& e)
   {
      instance->setStatus("Cannot load ", e.referenceInfo);

      return 0;
   }
   catch (InternalError& e)
   {
      instance->setStatus(e.message);

      return 0;
   }
   catch (EAbortException&)
   {
      return 0;
   }
}

void* LoadSymbolByString(void* systemEnv, void* referenceName)
{
   Instance* instance = _Machine->getInstance();
   if (instance == NULL)
      return 0;

   try {
      return instance->getSymbolRef((SystemEnv*)systemEnv, (const char*)referenceName, false);
   }
   catch (JITUnresolvedException& e)
   {
      instance->setStatus("Cannot load ", e.referenceInfo);

      return 0;
   }
   catch (InternalError& e)
   {
      instance->setStatus(e.message);

      return 0;
   }
   catch (EAbortException&)
   {
      return 0;
   }
}

void* LoadSymbolByBuffer(void* systemEnv, void* referenceName, size_t index, size_t length)
{
   if (length < 0x100) {
      IdentifierString str((const char*)referenceName, index, length);

      return LoadSymbolByString(systemEnv, (void*)str.c_str());
   }
   else {
      DynamicString<char> str((const char*)referenceName, index, length);

      return LoadSymbolByString(systemEnv, (void*)str.str());
   }
}

void* LoadClassByString(void* systemEnv, void* referenceName)
{
   Instance* instance = _Machine->getInstance();
   if (instance == NULL)
      return 0;

   try {
      return instance->getClassVMTRef((SystemEnv*)systemEnv, (const char*)referenceName, false);
   }
   catch (JITUnresolvedException& e)
   {
      instance->setStatus("Cannot load ", e.referenceInfo);

      return 0;
   }
   catch (InternalError& e)
   {
      instance->setStatus(e.message);

      return 0;
   }
   catch (EAbortException&)
   {
      return 0;
   }
}

void* LoadClassByBuffer(void* systemEnv, void* referenceName, size_t index, size_t length)
{
   if (length < 0x100) {
      IdentifierString str((const char*)referenceName, index, length);

      return LoadClassByString(systemEnv, (void*)str.c_str());
   }
   else {
      DynamicString<char> str((const char*)referenceName, index, length);

      return LoadClassByString(systemEnv, (void*)str.str());
   }
}

int EvaluateTape(void* systemEnv, void* sehTable, void* tape)
{
   Instance* instance = _Machine->getInstance();
   if (instance == NULL)
      return 0;

   try {
      int retVal = instance->interprete((SystemEnv*)systemEnv, sehTable, tape, false);

      return retVal;
   }
   catch (JITUnresolvedException& e)
   {
      instance->setStatus("Cannot load ", e.referenceInfo);

      return 0;
   }
   catch(InternalError& e)
   {
      instance->setStatus(e.message);

      return 0;
   }
   catch (EAbortException&)
   {
      return 0;
   }
}

//EXTERN_DLL_EXPORT size_t SetDebugMode()
//{
//   Instance* instance = getCurrentInstance();
//   if (instance == NULL)
//      return 0;
//
//   instance->setDebugMode();
//
//   return (size_t)instance->loadDebugSection();
//}

const char* GetVMLastError()
{
   Instance* instance = _Machine->getInstance();

   return  instance ? instance->getStatus() : "Not initialized";
}
