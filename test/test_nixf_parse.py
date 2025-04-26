
import nixf
import pytest
from nixf import DiagnosticKind, DiagnosticSeverity, NodeKind


def test_parse_valid_expression():
    """Test parsing a valid Nix expression."""
    src = "{ x = 1; }"
    ast, diagnostics = nixf.parse(src)

    # Check we got an AST and no errors
    assert ast is not None
    assert len(diagnostics) == 0
    assert ast.kind() == NodeKind.NK_ExprAttrs

    # Check the structure of the AST
    assert isinstance(ast, nixf.ExprAttrs)
    binds = ast.binds()
    assert binds is not None

    # Check bindings
    bindings = binds.bindings()
    assert len(bindings) == 1

    # Check the binding
    binding = bindings[0]
    assert binding.kind() == NodeKind.NK_Binding

    # Check attribute path
    assert isinstance(binding, nixf.Binding)
    path = binding.path()
    names = path.names()
    assert len(names) == 1
    assert names[0].is_static()
    assert names[0].static_name() == "x"

    # Check value
    value = binding.value()
    assert value is not None
    assert value.kind() == NodeKind.NK_ExprInt
    assert isinstance(value, nixf.ExprInt)
    assert value.value() == 1


def test_parse_syntax_error():
    """Test parsing an expression with syntax errors."""
    src = "{ x = ; }"  # Missing value after =
    ast, diagnostics = nixf.parse(src)

    # We should still get an AST (possibly partial/recovery) but with diagnostics
    assert ast is not None
    assert len(diagnostics) > 0

    # Check that at least one diagnostic is an error
    has_error = any(diag.severity() == DiagnosticSeverity.Error for diag in diagnostics)
    assert has_error


def test_parse_empty_string():
    """Test parsing an empty string."""
    src = ""
    ast, diagnostics = nixf.parse(src)

    # Should have no AST but no errors (empty is valid)
    assert ast is None
    assert len(diagnostics) == 0


def test_parse_complex_expression():
    """Test parsing a more complex Nix expression."""
    src = """
    {
      name = "test";
      value = 42;
      list = [ 1 2 3 ];
      nested = { a = 1; b = 2; };
      func = x: x + 1;
    }
    """
    ast, diagnostics = nixf.parse(src)

    # Check we got an AST and no errors
    assert ast is not None
    assert len(diagnostics) == 0
    assert ast.kind() == NodeKind.NK_ExprAttrs

    # Check the number of bindings
    assert isinstance(ast, nixf.ExprAttrs)
    binds = ast.binds()
    assert binds is not None
    bindings = binds.bindings()
    assert len(bindings) == 5

    # Just a basic check on the source text
    assert "test" in ast.src(src)
    assert "42" in ast.src(src)


def test_parse_with_diagnostics():
    """Test that diagnostics are properly returned."""
    # Missing closing brace
    src = "{ x = 1; "
    ast, diagnostics = nixf.parse(src)

    # There should be diagnostics
    assert len(diagnostics) > 0

    # Check the diagnostic details
    for diag in diagnostics:
        # Verify diagnostic API
        assert isinstance(diag.kind(), DiagnosticKind)
        assert isinstance(diag.severity(), DiagnosticSeverity)
        assert isinstance(diag.message(), str)
        assert isinstance(diag.sname(), str)

        # Check range information
        range_ = diag.range()
        assert range_ is not None

        # Source location validation
        assert range_.l_cur().offset() >= 0
        assert range_.r_cur().offset() >= range_.l_cur().offset()


def test_parse_string_interpolation():
    """Test parsing string interpolation."""
    src = '{ x = "${var}"; }'
    ast, _ = nixf.parse(src)

    # Check we got an AST and no errors
    assert ast is not None
    assert isinstance(ast, nixf.ExprAttrs)

    # Find the string expression
    binds = ast.binds()
    assert binds is not None

    binding = binds.bindings()[0]

    assert binding.kind() == NodeKind.NK_Binding
    assert isinstance(binding, nixf.Binding)

    str_expr = binding.value()

    # Check it's a string
    assert isinstance(str_expr, nixf.ExprString)
    assert str_expr.kind() == NodeKind.NK_ExprString

    # It shouldn't be a literal because it has interpolation
    assert not str_expr.is_literal()


def test_parse_return_type_annotation():
    """Test that the return type annotation is correct."""
    src = "{ }"
    result = nixf.parse(src)

    # Check we get a tuple with two elements
    assert isinstance(result, tuple)
    assert len(result) == 2

    # First element is an optional Node
    assert result[0] is None or hasattr(result[0], 'kind')

    # Second element is a list of diagnostics
    assert isinstance(result[1], list)
    assert all(hasattr(d, 'severity') for d in result[1])


@pytest.mark.parametrize("src,expected_ast_kind,expected_diagnostics", [
    ("{ x = 1; }", NodeKind.NK_ExprAttrs, 0),  # Valid attrs
    ("123", NodeKind.NK_ExprInt, 0),  # Valid int
    ("\"string\"", NodeKind.NK_ExprString, 0),  # Valid string
    ("{ = 1; }", None, 1),  # Invalid syntax
])
def test_parse_parameterized(src, expected_ast_kind, expected_diagnostics):
    """Parameterized tests for different input cases."""
    ast, diagnostics = nixf.parse(src)

    if expected_ast_kind is not None:
        assert ast is not None
        assert ast.kind() == expected_ast_kind

    assert len(diagnostics) >= expected_diagnostics
