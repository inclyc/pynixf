from enum import Enum
from typing import List, Optional, Tuple

class AttrNameKind(Enum):
    ID = 0
    String = 1
    Interpolation = 2

class NodeKind(Enum):
    NK_Interpolation = 0
    NK_InterpolableParts = 1
    NK_Misc = 2
    NK_Dot = 3
    NK_Identifier = 4
    NK_AttrName = 5
    NK_AttrPath = 6
    NK_Binding = 7
    NK_Inherit = 8
    NK_Binds = 9
    NK_LambdaArg = 10
    NK_Formals = 11
    NK_Formal = 12
    NK_Op = 13
    NK_ExprInt = 14
    NK_ExprFloat = 15
    NK_ExprVar = 16
    NK_ExprString = 17
    NK_ExprPath = 18
    NK_ExprSPath = 19
    NK_ExprParen = 20
    NK_ExprAttrs = 21
    NK_ExprSelect = 22
    NK_ExprCall = 23
    NK_ExprList = 24
    NK_ExprLambda = 25
    NK_ExprBinOp = 26
    NK_ExprUnaryOp = 27
    NK_ExprOpHasAttr = 28
    NK_ExprIf = 29
    NK_ExprAssert = 30
    NK_ExprLet = 31
    NK_ExprWith = 32

class DiagnosticSeverity(Enum):
    Fatal = 0
    Error = 1
    Warning = 2
    Info = 3
    Hint = 4

class DiagnosticKind(Enum):
    UnterminatedBComment = 0
    FloatNoExp = 1
    FloatLeadingZero = 2
    Expected = 3
    IntTooBig = 4
    RedundantParen = 5
    AttrPathExtraDot = 6
    SelectExtraDot = 7
    UnexpectedBetween = 8
    UnexpectedText = 9
    MissingSepFormals = 10
    LambdaArgExtraAt = 11
    OperatorNotAssociative = 12
    LetDynamic = 13
    EmptyInherit = 14
    OrIdentifier = 15
    DeprecatedURL = 16
    DeprecatedLet = 17
    PathTrailingSlash = 18
    MergeDiffRec = 19
    DuplicatedAttrName = 20
    DynamicInherit = 21
    EmptyFormal = 22
    FormalMissingComma = 23
    FormalExtraEllipsis = 24
    FormalMisplacedEllipsis = 25
    DuplicatedFormal = 26
    DuplicatedFormalToArg = 27
    UndefinedVariable = 28
    UnusedDefLet = 29
    UnusedDefLambdaNoArg_Formal = 30
    UnusedDefLambdaWithArg_Formal = 31
    UnusedDefLambdaWithArg_Arg = 32
    ExtraRecursive = 33
    ExtraWith = 34

class DefinitionSource(Enum):
    With = 0
    Let = 1
    LambdaArg = 2
    LambdaNoArg_Formal = 3
    LambdaWithArg_Arg = 4
    LambdaWithArg_Formal = 5
    Rec = 6
    Builtin = 7

class LookupResultKind(Enum):
    Undefined = 0
    FromWith = 1
    Defined = 2
    NoSuchVar = 3

class LexerCursor:
    def line(self) -> int: ...
    def column(self) -> int: ...
    def offset(self) -> int: ...

class LexerCursorRange:
    def l_cur(self) -> LexerCursor: ...
    def r_cur(self) -> LexerCursor: ...

class TextEdit:
    def old_range(self) -> LexerCursorRange: ...
    def new_text(self) -> str: ...
    def is_removal(self) -> bool: ...
    def is_insertion(self) -> bool: ...
    def is_replace(self) -> bool: ...

class PartialDiagnostic:
    def format(self) -> str: ...
    def args(self) -> List[str]: ...
    def range(self) -> LexerCursorRange: ...

class Note(PartialDiagnostic):
    def kind(self) -> int: ...
    def sname(self) -> str: ...
    def message(self) -> str: ...

class Fix:
    def edits(self) -> List[TextEdit]: ...
    def message(self) -> str: ...

class Diagnostic(PartialDiagnostic):
    def kind(self) -> DiagnosticKind: ...
    def severity(self) -> DiagnosticSeverity: ...
    def message(self) -> str: ...
    def sname(self) -> str: ...
    def notes(self) -> List[Note]: ...
    def fixes(self) -> List[Fix]: ...
    def to_json(self) -> str: ...

class PositionRange: ...

class Node:
    def kind(self) -> NodeKind: ...
    def name(self) -> str: ...
    def range(self) -> LexerCursorRange: ...
    def l_cur(self) -> LexerCursor: ...
    def r_cur(self) -> LexerCursor: ...
    def src(self, src: str) -> str: ...
    def descend(self, range: PositionRange) -> List[Node]: ...
    def children(self) -> List[Node]: ...

class Expr(Node):
    def maybe_lambda(self) -> bool: ...

class ExprInt(Expr):
    def value(self) -> int: ...

class ExprFloat(Expr):
    def value(self) -> float: ...

class Identifier(Node):
    def name(self) -> str: ...

class ExprVar(Expr):
    def id(self) -> Identifier: ...

class ExprString(Expr):
    def parts(self) -> List[Node]: ...
    def is_literal(self) -> bool: ...
    def literal(self) -> str: ...

class AttrName(Node):
    def kind(self) -> AttrNameKind: ...
    def is_static(self) -> bool: ...
    def static_name(self) -> str: ...

class AttrPath(Node):
    def names(self) -> List[AttrName]: ...

class Binding(Node):
    def path(self) -> AttrPath: ...
    def value(self) -> Optional[Expr]: ...

class Binds(Node):
    def bindings(self) -> List[Node]: ...

class ExprAttrs(Expr):
    def binds(self) -> Optional[Binds]: ...
    def is_recursive(self) -> bool: ...

class Definition:
    def syntax(self) -> Node: ...
    def uses(self) -> List[ExprVar]: ...
    def source(self) -> DefinitionSource: ...
    def is_builtin(self) -> bool: ...

class EnvNode:
    def parent(self) -> Optional["EnvNode"]: ...
    def syntax(self) -> Optional[Node]: ...
    def is_with(self) -> bool: ...
    def is_live(self) -> bool: ...

class LookupResult:
    kind: LookupResultKind
    def def_(self) -> Optional[Definition]: ...

class VariableLookupAnalysis:
    def __init__(self): ...
    def run_on_ast(self, node: Node) -> None: ...
    def query(self, var: ExprVar) -> LookupResult: ...
    def to_def(self, node: Node) -> Optional[Definition]: ...
    def env(self, node: Node) -> Optional[EnvNode]: ...
    def diagnostics(self) -> List[Diagnostic]: ...

class ParentMapAnalysis:
    def __init__(self): ...
    def run_on_ast(self, node: Node) -> None: ...
    def query(self, node: Node) -> Optional[Node]: ...
    def up_expr(self, node: Node) -> Optional[Expr]: ...
    def up_to(self, node: Node, target_kind: NodeKind) -> Optional[Node]: ...
    def is_root(self, node: Node) -> bool: ...

def parse(src: str) -> Tuple[Optional[Node], List[Diagnostic]]: ...
