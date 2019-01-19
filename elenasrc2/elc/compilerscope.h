//---------------------------------------------------------------------------
//		E L E N A   P r o j e c t:  ELENA Compiler
//
//		This file contains ELENA compiler scope class.
//
//                                              (C)2005-2019, by Alexei Rakov
//---------------------------------------------------------------------------

#ifndef compilerscopeH
#define compilerscopeH

#include "syntaxtree.h"
#include "compilercommon.h"
#include "derivation.h"

namespace _ELENA_
{

inline void freeident(ident_t s)
{
   if (s != NULL) {
      free((void*)s.c_str());
   }
}

struct ModuleScope : _ModuleScope
{
   _Compiler* compiler;

   virtual ref_t mapAnonymous(ident_t prefix);

   // warning mapping
//      bool warnOnWeakUnresolved;
   virtual _Module* loadReferenceModule(ident_t referenceName, ref_t& reference);
   virtual _Module* loadReferenceModule(ref_t reference, ref_t& moduleReference)
   {
      return loadReferenceModule(module->resolveReference(reference), moduleReference);
   }

   virtual void importClassInfo(ClassInfo& copy, ClassInfo& target, _Module* exporter, bool headerOnly, bool inheritMode);
   
   virtual ref_t loadClassInfo(ClassInfo& info, ident_t vmtName, bool headerOnly = false);
   virtual ref_t loadClassInfo(ClassInfo& info, ref_t reference, bool headerOnly = false)
   {
      return loadClassInfo(info, module->resolveReference(reference), headerOnly);
   }
   virtual ref_t loadSymbolExpressionInfo(SymbolExpressionInfo& info, ident_t symbolName);

//   //   ref_t mapIdentifier(ident_t referenceName, bool existing = false);
   virtual ref_t mapFullReference(ident_t referenceName, bool existing = false);
//   ///*virtual */ref_t mapNewTerminal(SNode terminal, bool privateOne);
   virtual ref_t mapTemplateClass(ident_t ns, ident_t templateName, bool& alreadyDeclared);
   virtual ref_t mapNewIdentifier(ident_t ns, ident_t identifier, bool privateOne);

   virtual _Memory* mapSection(ref_t reference, bool existing)
   {
      ref_t mask = reference & mskAnyRef;

      ident_t referenceName = module->resolveReference(reference & ~mskAnyRef);
      if (isTemplateWeakReference(referenceName)) {
         ident_t moduleName = module->Name();
         ident_t fullName = resolveWeakTemplateReference(referenceName + TEMPLATE_PREFIX_NS_LEN);
         if (NamespaceName::compare(fullName, moduleName)) {
            return module->mapSection(module->mapReference(fullName + getlength(moduleName)) | mask, existing);
         }
         else return module->mapSection(module->mapReference(fullName) | mask, existing);
      }
      else return module->mapSection(reference, existing);
   }

   ident_t resolveWeakTemplateReference(ident_t referenceName);

   void saveIncludedModule(_Module* extModule);
   void saveListMember(ident_t sectionName, ident_t memberName);

   virtual ref_t resolveImplicitIdentifier(ident_t ns, ident_t identifier, bool referenceOne, IdentifierList* importedNs);

   virtual ident_t resolveFullName(ref_t reference)
   {
      ident_t referenceName = module->resolveReference(reference & ~mskAnyRef);
      if (isTemplateWeakReference(referenceName)) {
         return project->resolveForward(referenceName + TEMPLATE_PREFIX_NS_LEN);
      }
      else return referenceName;
   }   
   virtual ident_t resolveFullName(ident_t referenceName)
   {
      if (isTemplateWeakReference(referenceName)) {
         return project->resolveForward(referenceName + TEMPLATE_PREFIX_NS_LEN);
      }
      else return referenceName;
   }

   virtual ref_t resolveClosure(ref_t closureMessage, ref_t outputRef, ident_t ns);

   virtual ref_t generateTemplate(ref_t reference, List<SNode>& parameters, ident_t ns, bool declarationMode);
   virtual void generateTemplateCode(SyntaxWriter& writer, ref_t reference, List<SNode>& parameters);
   virtual void generateTemplateProperty(SyntaxWriter& writer, ref_t reference, List<SNode>& parameters);
   virtual void generateExtensionTemplate(SyntaxTree& tree, ident_t ns, ref_t extensionRef);
   virtual void importClassTemplate(SyntaxWriter& writer, ref_t reference, List<SNode>& parameters);

   virtual void saveAttribute(ident_t typeName, ref_t classReference);

   virtual void declareNamespace(ident_t name);
   virtual bool includeNamespace(IdentifierList& importedNs, ident_t name, bool& duplicateInclusion);

   void compile(SyntaxTree& derivationTree, ident_t greeting);

   void beginModule(ident_t ns, ident_t filePath, DerivationWriter& writer);
   void endModule(DerivationWriter& writer);

   ModuleScope(_ProjectManager* project, _Compiler* compiler)
   {
      this->project = project;
      this->compiler = compiler;

   //   warnOnWeakUnresolved = project->WarnOnWeakUnresolved();
   }
};

} // _ELENA_

#endif // compilerscopeH
