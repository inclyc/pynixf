#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "nixf/Basic/Diagnostic.h"
#include "nixf/Basic/JSONDiagnostic.h"
#include "nixf/Basic/Nodes/Attrs.h"
#include "nixf/Basic/Nodes/Basic.h"
#include "nixf/Basic/Nodes/Simple.h"
#include "nixf/Parse/Parser.h"
#include "nixf/Sema/ParentMap.h"
#include "nixf/Sema/VariableLookup.h"

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace py = pybind11;

// Python-friendly wrapper for VariableLookupAnalysis that owns the diagnostics
// vector
class VariableLookupAnalysis {
private:
  std::vector<nixf::Diagnostic> diagnostics;
  std::unique_ptr<nixf::VariableLookupAnalysis> analysis;

public:
  VariableLookupAnalysis()
      : diagnostics(),
        analysis(std::make_unique<nixf::VariableLookupAnalysis>(diagnostics)) {}

  void runOnAST(const nixf::Node &root) { analysis->runOnAST(root); }

  nixf::VariableLookupAnalysis::LookupResult
  query(const nixf::ExprVar &var) const {
    return analysis->query(var);
  }

  const nixf::Definition *toDef(const nixf::Node &node) const {
    return analysis->toDef(node);
  }

  const nixf::EnvNode *env(const nixf::Node *node) const {
    return analysis->env(node);
  }

  const std::vector<nixf::Diagnostic> &getDiagnostics() const {
    return diagnostics;
  }
};

PYBIND11_MODULE(nixf, m) {
  m.doc() = "Python bindings for nixf library";

  // Bind Diagnostic classes
  py::class_<nixf::PartialDiagnostic>(m, "PartialDiagnostic")
      .def("format", &nixf::PartialDiagnostic::format)
      .def(
          "args", [](const nixf::PartialDiagnostic &d) { return d.args(); },
          py::return_value_policy::reference_internal)
      .def("range", &nixf::PartialDiagnostic::range,
           py::return_value_policy::reference_internal);

  py::class_<nixf::Note, nixf::PartialDiagnostic>(m, "Note")
      .def("kind", static_cast<nixf::Note::NoteKind (nixf::Note::*)() const>(
                       &nixf::Note::kind))
      .def("sname",
           static_cast<const char *(nixf::Note::*)() const>(&nixf::Note::sname),
           py::return_value_policy::reference_internal)
      .def("message",
           static_cast<const char *(nixf::Note::*)() const>(
               &nixf::Note::message),
           py::return_value_policy::reference_internal);

  py::class_<nixf::Diagnostic, nixf::PartialDiagnostic>(m, "Diagnostic")
      .def("kind", [](const nixf::Diagnostic &diag) { return diag.kind(); })
      .def("severity",
           [](const nixf::Diagnostic &diag) {
             return nixf::Diagnostic::severity(diag.kind());
           })
      .def("message",
           static_cast<const char *(nixf::Diagnostic::*)() const>(
               &nixf::Diagnostic::message),
           py::return_value_policy::reference_internal)
      .def("sname",
           static_cast<const char *(nixf::Diagnostic::*)() const>(
               &nixf::Diagnostic::sname),
           py::return_value_policy::reference_internal)
      .def(
          "notes", [](const nixf::Diagnostic &diag) { return diag.notes(); },
          py::return_value_policy::reference_internal)
      .def(
          "fixes", [](const nixf::Diagnostic &diag) { return diag.fixes(); },
          py::return_value_policy::reference_internal)
      .def("to_json", [](const nixf::Diagnostic &diag) {
        nlohmann::json j;
        nixf::to_json(j, diag);
        return j.dump();
      });

  py::class_<nixf::Fix>(m, "Fix")
      .def(
          "edits", [](const nixf::Fix &f) { return f.edits(); },
          py::return_value_policy::reference_internal)
      .def(
          "message", [](const nixf::Fix &f) { return f.message(); },
          py::return_value_policy::reference_internal);

  py::class_<nixf::TextEdit>(m, "TextEdit")
      .def("old_range", &nixf::TextEdit::oldRange)
      .def("new_text", [](const nixf::TextEdit &e) { return e.newText(); })
      .def("is_removal", &nixf::TextEdit::isRemoval)
      .def("is_insertion", &nixf::TextEdit::isInsertion)
      .def("is_replace", &nixf::TextEdit::isReplace);

  py::class_<nixf::LexerCursor>(m, "LexerCursor")
      .def("line", &nixf::LexerCursor::line)
      .def("column", &nixf::LexerCursor::column)
      .def("offset", &nixf::LexerCursor::offset);

  py::class_<nixf::LexerCursorRange>(m, "LexerCursorRange")
      .def("l_cur", &nixf::LexerCursorRange::lCur)
      .def("r_cur", &nixf::LexerCursorRange::rCur);

  // Bind Node hierarchy
  py::class_<nixf::Node, std::shared_ptr<nixf::Node>>(m, "Node")
      .def("kind", &nixf::Node::kind)
      .def("name", [](const nixf::Node &n) { return n.name(); })
      .def("range", &nixf::Node::range)
      .def("l_cur", &nixf::Node::lCur)
      .def("r_cur", &nixf::Node::rCur)
      .def("src",
           [](const nixf::Node &n, const std::string &src) {
             return std::string(n.src(src));
           })
      .def(
          "descend",
          [](const nixf::Node &n, nixf::PositionRange range) {
            return n.descend(range);
          },
          py::return_value_policy::reference_internal)
      .def(
          "children",
          [](const nixf::Node &n) {
            // Convert boost::container::small_vector to std::vector
            auto children = n.children();
            std::vector<const nixf::Node *> result(children.begin(),
                                                   children.end());
            return result;
          },
          py::return_value_policy::reference_internal);

  py::class_<nixf::Expr, nixf::Node, std::shared_ptr<nixf::Expr>>(m, "Expr")
      .def("maybe_lambda", [](const nixf::Expr &e) { return e.maybeLambda(); });

  // Bind common expression types
  py::class_<nixf::ExprInt, nixf::Expr, std::shared_ptr<nixf::ExprInt>>(
      m, "ExprInt")
      .def("value", &nixf::ExprInt::value);

  py::class_<nixf::ExprFloat, nixf::Expr, std::shared_ptr<nixf::ExprFloat>>(
      m, "ExprFloat")
      .def("value", &nixf::ExprFloat::value);

  py::class_<nixf::ExprString, nixf::Expr, std::shared_ptr<nixf::ExprString>>(
      m, "ExprString")
      .def("parts", &nixf::ExprString::parts,
           py::return_value_policy::reference_internal)
      .def("is_literal", &nixf::ExprString::isLiteral)
      .def("literal", [](const nixf::ExprString &e) {
        return e.isLiteral() ? e.literal() : std::string();
      });

  py::class_<nixf::ExprVar, nixf::Expr, std::shared_ptr<nixf::ExprVar>>(
      m, "ExprVar")
      .def("id", &nixf::ExprVar::id,
           py::return_value_policy::reference_internal);

  py::class_<nixf::Identifier, nixf::Node, std::shared_ptr<nixf::Identifier>>(
      m, "Identifier")
      .def("name", &nixf::Identifier::name,
           py::return_value_policy::reference_internal);

  py::class_<nixf::ExprAttrs, nixf::Expr, std::shared_ptr<nixf::ExprAttrs>>(
      m, "ExprAttrs")
      .def("binds", &nixf::ExprAttrs::binds,
           py::return_value_policy::reference_internal)
      .def("is_recursive", &nixf::ExprAttrs::isRecursive);

  py::class_<nixf::Binds, nixf::Node, std::shared_ptr<nixf::Binds>>(m, "Binds")
      .def("bindings", &nixf::Binds::bindings,
           py::return_value_policy::reference_internal);

  py::class_<nixf::Binding, nixf::Node, std::shared_ptr<nixf::Binding>>(
      m, "Binding")
      .def("path", &nixf::Binding::path,
           py::return_value_policy::reference_internal)
      .def(
          "value", [](const nixf::Binding &b) { return b.value(); },
          py::return_value_policy::reference_internal);

  py::class_<nixf::AttrPath, nixf::Node, std::shared_ptr<nixf::AttrPath>>(
      m, "AttrPath")
      .def("names", &nixf::AttrPath::names,
           py::return_value_policy::reference_internal);

  // Expose AttrName::AttrNameKind enum
  py::enum_<nixf::AttrName::AttrNameKind>(m, "AttrNameKind")
      .value("ID", nixf::AttrName::ANK_ID)
      .value("String", nixf::AttrName::ANK_String)
      .value("Interpolation", nixf::AttrName::ANK_Interpolation);

  py::class_<nixf::AttrName, nixf::Node, std::shared_ptr<nixf::AttrName>>(
      m, "AttrName")
      .def("kind", &nixf::AttrName::kind)
      .def("is_static", &nixf::AttrName::isStatic)
      .def("static_name", [](const nixf::AttrName &a) {
        return a.isStatic() ? a.staticName() : std::string();
      });

  // Expose Definition class and DefinitionSource enum
  py::enum_<nixf::Definition::DefinitionSource>(m, "DefinitionSource")
      .value("With", nixf::Definition::DS_With)
      .value("Let", nixf::Definition::DS_Let)
      .value("LambdaArg", nixf::Definition::DS_LambdaArg)
      .value("LambdaNoArg_Formal", nixf::Definition::DS_LambdaNoArg_Formal)
      .value("LambdaWithArg_Arg", nixf::Definition::DS_LambdaWithArg_Arg)
      .value("LambdaWithArg_Formal", nixf::Definition::DS_LambdaWithArg_Formal)
      .value("Rec", nixf::Definition::DS_Rec)
      .value("Builtin", nixf::Definition::DS_Builtin);

  py::class_<nixf::Definition, std::shared_ptr<nixf::Definition>>(m,
                                                                  "Definition")
      .def("syntax", &nixf::Definition::syntax,
           py::return_value_policy::reference_internal)
      .def("uses", &nixf::Definition::uses,
           py::return_value_policy::reference_internal)
      .def("source", &nixf::Definition::source)
      .def("is_builtin", &nixf::Definition::isBuiltin);

  py::class_<nixf::EnvNode>(m, "EnvNode")
      .def("parent", &nixf::EnvNode::parent,
           py::return_value_policy::reference_internal)
      .def("syntax", &nixf::EnvNode::syntax,
           py::return_value_policy::reference_internal)
      .def("is_with", &nixf::EnvNode::isWith)
      .def("is_live", &nixf::EnvNode::isLive);

  // Expose VariableLookupAnalysis::LookupResultKind enum
  py::enum_<nixf::VariableLookupAnalysis::LookupResultKind>(m,
                                                            "LookupResultKind")
      .value("Undefined",
             nixf::VariableLookupAnalysis::LookupResultKind::Undefined)
      .value("FromWith",
             nixf::VariableLookupAnalysis::LookupResultKind::FromWith)
      .value("Defined", nixf::VariableLookupAnalysis::LookupResultKind::Defined)
      .value("NoSuchVar",
             nixf::VariableLookupAnalysis::LookupResultKind::NoSuchVar);

  // Expose VariableLookupAnalysis::LookupResult struct
  py::class_<nixf::VariableLookupAnalysis::LookupResult>(m, "LookupResult")
      .def_readonly("kind", &nixf::VariableLookupAnalysis::LookupResult::Kind)
      .def_readonly("def_", &nixf::VariableLookupAnalysis::LookupResult::Def);

  // Expose VariableLookupAnalysis wrapper class
  py::class_<VariableLookupAnalysis>(m, "VariableLookupAnalysis")
      .def(py::init<>())
      .def("run_on_ast", &VariableLookupAnalysis::runOnAST)
      .def("query", &VariableLookupAnalysis::query,
           py::return_value_policy::copy)
      .def("to_def", &VariableLookupAnalysis::toDef,
           py::return_value_policy::reference_internal)
      .def("env", &VariableLookupAnalysis::env,
           py::return_value_policy::reference_internal)
      .def("diagnostics", &VariableLookupAnalysis::getDiagnostics,
           py::return_value_policy::reference_internal);

  // Expose ParentMap analysis
  py::class_<nixf::ParentMapAnalysis>(m, "ParentMapAnalysis")
      .def(py::init<>())
      .def("run_on_ast", &nixf::ParentMapAnalysis::runOnAST)
      .def("query", &nixf::ParentMapAnalysis::query)
      .def("up_expr", &nixf::ParentMapAnalysis::upExpr)
      .def("up_to", &nixf::ParentMapAnalysis::upTo)
      .def("is_root",
           static_cast<bool (nixf::ParentMapAnalysis::*)(const nixf::Node &)
                           const>(&nixf::ParentMapAnalysis::isRoot));

  // Expose the parse function as a Python function
  m.def(
      "parse",
      [](const std::string &src) {
        std::vector<nixf::Diagnostic> diags;
        auto node = nixf::parse(src, diags);
        return py::make_tuple(node, diags);
      },
      "Parse Nix code and return (node, diagnostics)");

  // Expose NodeKind enum
  py::enum_<nixf::Node::NodeKind>(m, "NodeKind")
      .value("NK_Interpolation", nixf::Node::NK_Interpolation)
      .value("NK_InterpolableParts", nixf::Node::NK_InterpolableParts)
      .value("NK_Misc", nixf::Node::NK_Misc)
      .value("NK_Dot", nixf::Node::NK_Dot)
      .value("NK_Identifier", nixf::Node::NK_Identifier)
      .value("NK_AttrName", nixf::Node::NK_AttrName)
      .value("NK_AttrPath", nixf::Node::NK_AttrPath)
      .value("NK_Binding", nixf::Node::NK_Binding)
      .value("NK_Inherit", nixf::Node::NK_Inherit)
      .value("NK_Binds", nixf::Node::NK_Binds)
      .value("NK_LambdaArg", nixf::Node::NK_LambdaArg)
      .value("NK_Formals", nixf::Node::NK_Formals)
      .value("NK_Formal", nixf::Node::NK_Formal)
      .value("NK_Op", nixf::Node::NK_Op)
      .value("NK_ExprInt", nixf::Node::NK_ExprInt)
      .value("NK_ExprFloat", nixf::Node::NK_ExprFloat)
      .value("NK_ExprVar", nixf::Node::NK_ExprVar)
      .value("NK_ExprString", nixf::Node::NK_ExprString)
      .value("NK_ExprPath", nixf::Node::NK_ExprPath)
      .value("NK_ExprSPath", nixf::Node::NK_ExprSPath)
      .value("NK_ExprParen", nixf::Node::NK_ExprParen)
      .value("NK_ExprAttrs", nixf::Node::NK_ExprAttrs)
      .value("NK_ExprSelect", nixf::Node::NK_ExprSelect)
      .value("NK_ExprCall", nixf::Node::NK_ExprCall)
      .value("NK_ExprList", nixf::Node::NK_ExprList)
      .value("NK_ExprLambda", nixf::Node::NK_ExprLambda)
      .value("NK_ExprBinOp", nixf::Node::NK_ExprBinOp)
      .value("NK_ExprUnaryOp", nixf::Node::NK_ExprUnaryOp)
      .value("NK_ExprOpHasAttr", nixf::Node::NK_ExprOpHasAttr)
      .value("NK_ExprIf", nixf::Node::NK_ExprIf)
      .value("NK_ExprAssert", nixf::Node::NK_ExprAssert)
      .value("NK_ExprLet", nixf::Node::NK_ExprLet)
      .value("NK_ExprWith", nixf::Node::NK_ExprWith);

  // Expose Diagnostic Severity enum
  py::enum_<nixf::Diagnostic::Severity>(m, "DiagnosticSeverity")
      .value("Fatal", nixf::Diagnostic::DS_Fatal)
      .value("Error", nixf::Diagnostic::DS_Error)
      .value("Warning", nixf::Diagnostic::DS_Warning)
      .value("Info", nixf::Diagnostic::DS_Info)
      .value("Hint", nixf::Diagnostic::DS_Hint);

  // Expose Diagnostic Kind enum
  py::enum_<nixf::Diagnostic::DiagnosticKind>(m, "DiagnosticKind")
      .value("UnterminatedBComment", nixf::Diagnostic::DK_UnterminatedBComment)
      .value("FloatNoExp", nixf::Diagnostic::DK_FloatNoExp)
      .value("FloatLeadingZero", nixf::Diagnostic::DK_FloatLeadingZero)
      .value("Expected", nixf::Diagnostic::DK_Expected)
      .value("IntTooBig", nixf::Diagnostic::DK_IntTooBig)
      .value("RedundantParen", nixf::Diagnostic::DK_RedundantParen)
      .value("AttrPathExtraDot", nixf::Diagnostic::DK_AttrPathExtraDot)
      .value("SelectExtraDot", nixf::Diagnostic::DK_SelectExtraDot)
      .value("UnexpectedBetween", nixf::Diagnostic::DK_UnexpectedBetween)
      .value("UnexpectedText", nixf::Diagnostic::DK_UnexpectedText)
      .value("MissingSepFormals", nixf::Diagnostic::DK_MissingSepFormals)
      .value("LambdaArgExtraAt", nixf::Diagnostic::DK_LambdaArgExtraAt)
      .value("OperatorNotAssociative",
             nixf::Diagnostic::DK_OperatorNotAssociative)
      .value("LetDynamic", nixf::Diagnostic::DK_LetDynamic)
      .value("EmptyInherit", nixf::Diagnostic::DK_EmptyInherit)
      .value("OrIdentifier", nixf::Diagnostic::DK_OrIdentifier)
      .value("DeprecatedURL", nixf::Diagnostic::DK_DeprecatedURL)
      .value("DeprecatedLet", nixf::Diagnostic::DK_DeprecatedLet)
      .value("PathTrailingSlash", nixf::Diagnostic::DK_PathTrailingSlash)
      .value("MergeDiffRec", nixf::Diagnostic::DK_MergeDiffRec)
      .value("DuplicatedAttrName", nixf::Diagnostic::DK_DuplicatedAttrName)
      .value("DynamicInherit", nixf::Diagnostic::DK_DynamicInherit)
      .value("EmptyFormal", nixf::Diagnostic::DK_EmptyFormal)
      .value("FormalMissingComma", nixf::Diagnostic::DK_FormalMissingComma)
      .value("FormalExtraEllipsis", nixf::Diagnostic::DK_FormalExtraEllipsis)
      .value("FormalMisplacedEllipsis",
             nixf::Diagnostic::DK_FormalMisplacedEllipsis)
      .value("DuplicatedFormal", nixf::Diagnostic::DK_DuplicatedFormal)
      .value("DuplicatedFormalToArg",
             nixf::Diagnostic::DK_DuplicatedFormalToArg)
      .value("UndefinedVariable", nixf::Diagnostic::DK_UndefinedVariable)
      .value("UnusedDefLet", nixf::Diagnostic::DK_UnusedDefLet)
      .value("UnusedDefLambdaNoArg_Formal",
             nixf::Diagnostic::DK_UnusedDefLambdaNoArg_Formal)
      .value("UnusedDefLambdaWithArg_Formal",
             nixf::Diagnostic::DK_UnusedDefLambdaWithArg_Formal)
      .value("UnusedDefLambdaWithArg_Arg",
             nixf::Diagnostic::DK_UnusedDefLambdaWithArg_Arg)
      .value("ExtraRecursive", nixf::Diagnostic::DK_ExtraRecursive)
      .value("ExtraWith", nixf::Diagnostic::DK_ExtraWith);
}
