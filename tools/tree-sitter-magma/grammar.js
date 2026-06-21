/// <reference types="tree-sitter-cli/dsl" />
// tree-sitter grammar for Magma (computational algebra system)
// https://magma.maths.usyd.edu.au/magma/

module.exports = grammar({
  name: 'magma',

  extras: $ => [
    /\s/,
    $.line_comment,
    $.block_comment,
  ],

  word: $ => $.identifier,

  conflicts: $ => [
    [$.function_definition],
    [$.procedure_definition],
    [$.intrinsic_definition],
  ],

  rules: {
    source_file: $ => repeat($._statement),

    _statement: $ => choice(
      $.function_definition,
      $.procedure_definition,
      $.intrinsic_definition,
      $.if_statement,
      $.for_statement,
      $.while_statement,
      $.repeat_statement,
      $.case_statement,
      $.try_statement,
      $.assignment_statement,
      $.return_statement,
      $.load_statement,
      $.print_statement,
      $.require_statement,
      $.error_statement,
      $.delete_statement,
      $.freeze_statement,
      $.expression_statement,
    ),

    // function Name(args) ... end function;
    function_definition: $ => prec(10, seq(
      'function',
      field('name', $.identifier),
      $.parameter_list,
      optional(seq('->', $._type_expr)),
      repeat($._statement),
      'end', 'function', ';',
    )),

    // procedure Name(~args) ... end procedure;
    procedure_definition: $ => prec(10, seq(
      'procedure',
      field('name', $.identifier),
      $.parameter_list,
      repeat($._statement),
      'end', 'procedure', ';',
    )),

    // intrinsic Name(x::Type) -> Type { doc } ... end intrinsic;
    intrinsic_definition: $ => prec(10, seq(
      'intrinsic',
      field('name', $.identifier),
      $.parameter_list,
      optional(seq('->', $._type_expr)),
      optional($.doc_string),
      repeat($._statement),
      'end', 'intrinsic', ';',
    )),

    parameter_list: $ => seq(
      token.immediate('('),
      optional(commaSep1($.parameter)),
      ')',
    ),

    parameter: $ => seq(
      optional('~'),
      $.identifier,
      optional(seq('::', $._type_expr)),
    ),

    doc_string: $ => seq('{', /[^}]*/, '}'),

    // if cond then ... elif ... else ... end if;
    if_statement: $ => seq(
      'if',
      $._expression,
      'then',
      repeat($._statement),
      repeat($.elif_clause),
      optional($.else_clause),
      'end', 'if', ';',
    ),

    elif_clause: $ => seq(
      choice('elif', 'elsif'),
      $._expression,
      'then',
      repeat($._statement),
    ),

    else_clause: $ => seq(
      'else',
      repeat($._statement),
    ),

    // for var in expr do ... end for;
    // for var := start to end do ... end for;
    for_statement: $ => seq(
      'for',
      $.identifier,
      choice(
        seq('in', $._expression),
        seq(':=', $._expression, 'to', $._expression,
            optional(seq('by', $._expression))),
      ),
      'do',
      repeat($._statement),
      'end', 'for', ';',
    ),

    // while cond do ... end while;
    while_statement: $ => seq(
      'while',
      $._expression,
      'do',
      repeat($._statement),
      'end', 'while', ';',
    ),

    // repeat ... until cond;
    repeat_statement: $ => seq(
      'repeat',
      repeat($._statement),
      'until',
      $._expression,
      ';',
    ),

    // case expr: when val: ... end case;
    case_statement: $ => seq(
      'case',
      $._expression,
      ':',
      repeat($.when_clause),
      optional($.else_clause),
      'end', 'case', ';',
    ),

    when_clause: $ => seq(
      'when',
      commaSep1($._expression),
      ':',
      repeat($._statement),
    ),

    // try ... catch e ... end try;
    try_statement: $ => seq(
      'try',
      repeat($._statement),
      'catch',
      $.identifier,
      repeat($._statement),
      'end', 'try', ';',
    ),

    // x := expr;
    assignment_statement: $ => seq(
      $._expression,
      choice(':=', '+:=', '-:=', '*:=', 'cat:='),
      $._expression,
      ';',
    ),

    return_statement: $ => seq(
      'return',
      optional($._expression),
      ';',
    ),

    load_statement: $ => seq(
      'load',
      field('path', $._expression),
      ';',
    ),

    print_statement: $ => seq(
      choice('print', 'printf', 'fprintf'),
      commaSep1($._expression),
      ';',
    ),

    require_statement: $ => seq(
      'require',
      $._expression,
      ':',
      $._expression,
      ';',
    ),

    error_statement: $ => seq(
      'error',
      $._expression,
      ';',
    ),

    delete_statement: $ => seq(
      'delete',
      $._expression,
      ';',
    ),

    freeze_statement: $ => seq(
      'freeze',
      $._expression,
      ';',
    ),

    expression_statement: $ => seq(
      $._expression,
      ';',
    ),

    // Expressions
    _expression: $ => choice(
      $.identifier,
      $.number,
      $.string_literal,
      $.boolean_literal,
      $.call_expression,
      $.binary_expression,
      $.unary_expression,
      $.parenthesized_expression,
      $.index_expression,
      $.member_expression,
      $.sequence_expression,
      $.set_expression,
      $.tuple_expression,
      $.range_expression,
      $.comprehension,
      $.anonymous_function,
      $.format_string,
    ),

    call_expression: $ => prec(8, seq(
      field('name', $._expression),
      '(',
      optional(commaSep1(choice($._expression, $.named_argument))),
      ')',
    )),

    named_argument: $ => seq(
      $.identifier,
      ':=',
      $._expression,
    ),

    binary_expression: $ => choice(
      // Arithmetic
      prec.left(4, seq($._expression, '+', $._expression)),
      prec.left(4, seq($._expression, '-', $._expression)),
      prec.left(5, seq($._expression, '*', $._expression)),
      prec.left(5, seq($._expression, '/', $._expression)),
      prec.right(6, seq($._expression, '^', $._expression)),
      // Word operators
      prec.left(5, seq($._expression, 'div', $._expression)),
      prec.left(5, seq($._expression, 'mod', $._expression)),
      prec.left(4, seq($._expression, 'cat', $._expression)),
      // Comparison
      prec.left(3, seq($._expression, choice('eq', 'ne', 'lt', 'gt', 'le', 'ge'), $._expression)),
      prec.left(3, seq($._expression, choice('cmpeq', 'cmpne'), $._expression)),
      prec.left(3, seq($._expression, 'in', $._expression)),
      prec.left(3, seq($._expression, 'notin', $._expression)),
      prec.left(3, seq($._expression, 'subset', $._expression)),
      // Logical
      prec.left(1, seq($._expression, 'and', $._expression)),
      prec.left(0, seq($._expression, 'or', $._expression)),
      // Magma also has: meet, join, diff, sdiff
      prec.left(4, seq($._expression, 'meet', $._expression)),
      prec.left(4, seq($._expression, 'join', $._expression)),
      prec.left(4, seq($._expression, 'diff', $._expression)),
      prec.left(4, seq($._expression, 'sdiff', $._expression)),
      // String/sequence concatenation
      prec.left(4, seq($._expression, '&', $._expression)),
      // Coerce
      prec.left(7, seq($._expression, '!', $._expression)),
    ),

    unary_expression: $ => prec(9, seq(
      choice('-', '+', 'not', '#', '~'),
      $._expression,
    )),

    parenthesized_expression: $ => seq('(', $._expression, ')'),

    index_expression: $ => prec(8, seq(
      $._expression,
      '[',
      commaSep1($._expression),
      ']',
    )),

    member_expression: $ => prec(8, seq(
      $._expression,
      '`',
      $.identifier,
    )),

    // [a, b, c]
    sequence_expression: $ => seq(
      '[',
      optional(commaSep1($._expression)),
      ']',
    ),

    // {a, b, c}
    set_expression: $ => seq(
      '{',
      optional(commaSep1($._expression)),
      '}',
    ),

    // <a, b, c>
    tuple_expression: $ => seq(
      '<',
      optional(commaSep1($._expression)),
      '>',
    ),

    // [a..b]
    range_expression: $ => prec(7, seq(
      '[',
      $._expression,
      '..',
      $._expression,
      ']',
    )),

    // [expr : var in set | guard]  or  {expr : var in set | guard}
    comprehension: $ => choice(
      seq('[', $._expression, ':', $.comp_clause, ']'),
      seq('{', $._expression, ':', $.comp_clause, '}'),
    ),

    comp_clause: $ => seq(
      $.identifier,
      'in',
      $._expression,
      optional(seq('|', $._expression)),
    ),

    // func<args | body>
    anonymous_function: $ => seq(
      'func',
      '<',
      optional(commaSep1($.identifier)),
      '|',
      $._expression,
      '>',
    ),

    // Sprintf("...", args)  — format strings
    format_string: $ => seq(
      'Sprintf',
      '(',
      $.string_literal,
      repeat(seq(',', $._expression)),
      ')',
    ),

    // Type expressions for :: annotations
    _type_expr: $ => choice(
      $.identifier,
      $.parameterized_type,
      $.tuple_type,
    ),

    parameterized_type: $ => prec(2, seq(
      $.identifier,
      '[',
      commaSep1($._type_expr),
      ']',
    )),

    tuple_type: $ => seq(
      '<',
      commaSep1($._type_expr),
      '>',
    ),

    // Terminals
    identifier: _ => /[a-zA-Z_][a-zA-Z0-9_]*/,
    number: _ => /\d+(\.\d+)?([eE][+-]?\d+)?/,
    string_literal: _ => seq('"', /[^"]*/, '"'),
    boolean_literal: _ => choice('true', 'false'),

    // Comments
    line_comment: _ => token(seq('//', /[^\n]*/)),
    block_comment: _ => token(seq('/*', /[^*]*\*+([^/*][^*]*\*+)*/, '/')),
  },
});

function commaSep1(rule) {
  return seq(rule, repeat(seq(',', rule)));
}
