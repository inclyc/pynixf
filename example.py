#!/usr/bin/env python3
"""
Example script demonstrating the use of nixf_python bindings.
"""

import json
import sys

import nixf


def print_node_info(node, src, indent=0):
    """Recursively print information about a node and its children."""
    prefix = "  " * indent
    print(f"{prefix}Node: {node.name()} (kind={node.kind()})")
    print(
        f"{prefix}Range: L{node.l_cur().line()}:{node.l_cur().column()} - L{node.r_cur().line()}:{node.r_cur().column()}"
    )
    print(f"{prefix}Source: {repr(node.src(src))}")

    # Print specific information based on node type
    if isinstance(node, nixf.ExprInt):
        print(f"{prefix}Value: {node.value()}")
    elif isinstance(node, nixf.ExprVar):
        print(f"{prefix}Identifier: {node.id()}")
    elif isinstance(node, nixf.ExprString) and node.is_literal():
        print(f"{prefix}String value: {repr(node.literal())}")
    elif isinstance(node, nixf.ExprAttrs):
        print(f"{prefix}Is recursive: {node.is_recursive()}")

    # Recursively print children
    children = node.children()
    if children:
        print(f"{prefix}Children: {len(children)}")
        for child in children:
            if child is not None:
                print_node_info(child, src, indent + 1)


def print_diagnostics(diags):
    """Print diagnostic information."""
    for i, diag in enumerate(diags):
        print(f"Diagnostic {i+1}:")
        print(f"  Kind: {diag.kind()}")
        print(f"  Severity: {diag.severity()}")
        print(f"  Message: {diag.format()}")
        print(
            f"  Range: L{diag.range().l_cur().line()}:{diag.range().l_cur().column()} - "
            + f"L{diag.range().r_cur().line()}:{diag.range().r_cur().column()}"
        )

        # Print notes
        notes = diag.notes()
        if notes:
            print(f"  Notes: {len(notes)}")
            for j, note in enumerate(notes):
                print(f"    Note {j+1}: {note.format()}")

        # Print fixes
        fixes = diag.fixes()
        if fixes:
            print(f"  Fixes: {len(fixes)}")
            for j, fix in enumerate(fixes):
                print(f"    Fix {j+1}: {fix.message()}")
                for edit in fix.edits():
                    if edit.is_insertion():
                        print(
                            f"      Insert '{edit.new_text()}' at L{edit.old_range().l_cur().line()}:{edit.old_range().l_cur().column()}"
                        )
                    elif edit.is_removal():
                        print(
                            f"      Remove text at L{edit.old_range().l_cur().line()}:{edit.old_range().l_cur().column()} - "
                            + f"L{edit.old_range().r_cur().line()}:{edit.old_range().r_cur().column()}"
                        )
                    else:
                        print(
                            f"      Replace text at L{edit.old_range().l_cur().line()}:{edit.old_range().l_cur().column()} - "
                            + f"L{edit.old_range().r_cur().line()}:{edit.old_range().r_cur().column()} with '{edit.new_text()}'"
                        )

        # Print JSON representation
        print(f"  JSON: {json.loads(diag.to_json())}")


def perform_parent_analysis(node, src):
    """Demonstrate the ParentMap analysis."""
    print("\nParent Map Analysis:")
    parent_map = nixf.ParentMapAnalysis()
    parent_map.run_on_ast(node)

    def print_parent_info(node, depth=0):
        prefix = "  " * depth
        print(f"{prefix}Node: {node.name()}")

        # Find parent
        parent = parent_map.query(node)
        if parent is not None and not parent_map.is_root(node):
            print(f"{prefix}Parent: {parent.name()}")

            # Find expression parent
            expr_parent = parent_map.up_expr(node)
            if expr_parent is not None:
                print(f"{prefix}Expression Parent: {expr_parent.name()}")
        else:
            print(f"{prefix}This is the root node")

        # Process a few children for demonstration
        children = node.children()
        if children and depth < 2:  # Limit depth for demonstration
            for i, child in enumerate(children[:2]):  # Limit to first 2 children
                if child is not None:
                    print(f"{prefix}Child {i}:")
                    print_parent_info(child, depth + 1)

    print_parent_info(node)


def main():
    if len(sys.argv) > 1:
        # Parse from file
        with open(sys.argv[1], "r") as f:
            src = f.read()
    else:
        # Use a demo source if no file provided
        src = """
{
  # This is a comment
  x = 123;
  y = "Hello, world!";
  z = { a = 1; b = 2; };
  inherit (z) a;
  broken = a.;
}
"""

    # Parse the Nix code
    print(f"Parsing Nix code:\n{src}\n")
    node, diags = nixf.parse(src)

    # Print information about the AST
    print("AST Information:")
    print_node_info(node, src)

    # Print diagnostics
    print("\nDiagnostics:")
    print_diagnostics(diags)

    # Perform parent map analysis
    perform_parent_analysis(node, src)


if __name__ == "__main__":
    main()
