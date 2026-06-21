/// <reference types="tree-sitter-cli/dsl" />
// tree-sitter grammar for FORM (symbolic manipulation language)
// https://www.nikhef.nl/~form/

module.exports = grammar({
  name: 'form',

  extras: $ => [
    /\s/,
    $.line_comment,
    $.block_comment,
  ],

  word: $ => $.identifier,

  // Tell tree-sitter to allow GLR parsing for these specific ambiguities
  conflicts: $ => [
    [$.procedure_definition],
    [$.call_statement],
  ],

  rules: {
    source_file: $ => repeat($._statement),

    _statement: $ => choice(
      $.declaration_statement,
      $.substitution_statement,
      $.print_statement,
      $.procedure_definition,
      $.call_statement,
      $.if_statement,
      $.repeat_statement,
      $.do_loop,
      $.argument_block,
      $.label_statement,
      $.module_directive,
      $.include_directive,
      $.define_directive,
      $.preproc_directive_with_arg,
      $.preproc_directive_bare,
      $.preproc_line_directive,
      $.preproc_do,
      $.expression_statement,
    ),

    // Declarations: Symbols, Functions, CFunctions, Vectors, Indices, etc.
    declaration_statement: $ => seq(
      field('kind', choice(
        'Symbols', 'Symbol',
        'Functions', 'Function',
        'CFunctions', 'CFunction',
        'Tensors', 'Tensor',
        'Vectors', 'Vector',
        'Indices', 'Index',
        'Sets', 'Set',
        'Tables', 'Table',
        'NTensors', 'NTensor',
        'AutoDeclare',
        'Dimension',
        'FixIndex',
        'Off', 'On',
      )),
      commaSep1($.identifier),
      ';',
    ),

    // Substitution: id pattern = replacement;
    substitution_statement: $ => seq(
      optional(choice('Local', 'Global')),
      'id',
      $._expression,
      '=',
      $._expression,
      ';',
    ),

    // Print/format statements
    print_statement: $ => seq(
      choice('Print', 'NoPrint', 'Format'),
      optional($._expression),
      ';',
    ),

    // Procedure definition: #procedure name(args) ... #endprocedure
    // Parameters use a dedicated list to avoid expression ambiguity.
    procedure_definition: $ => prec(10, seq(
      '#procedure',
      field('name', $.identifier),
      optional($.parameter_list),
      repeat($._statement),
      '#endprocedure',
    )),

    parameter_list: $ => seq(
      token.immediate('('),
      optional(commaSep1(alias($.identifier, $.parameter))),
      ')',
    ),

    // #call procedure(args)
    call_statement: $ => prec(10, seq(
      '#call',
      field('name', $.identifier),
      optional(seq(
        token.immediate('('),
        optional(commaSep1($._expression)),
        ')',
      )),
    )),

    // #include filename
    include_directive: $ => seq(
      '#include',
      choice($.string_literal, $.identifier),
    ),

    // #define name "value"
    define_directive: $ => seq(
      choice('#define', '#redefine'),
      $.identifier,
      $.string_literal,
    ),

    // Preprocessor directives that REQUIRE an argument
    preproc_directive_with_arg: $ => seq(
      choice('#ifdef', '#ifndef', '#if', '#case', '#switch', '#undefine'),
      $._expression,
    ),

    // Preprocessor directives with NO argument
    preproc_directive_bare: _ => choice(
      '#else', '#endif', '#break', '#endswitch',
    ),

    // Preprocessor directives that consume rest of line
    preproc_line_directive: $ => seq(
      choice('#message', '#write', '#close', '#system',
             '#optimize', '#Optimize', '#toexternal', '#fromexternal',
             '#setexternal', '#rmexternal', '#external',
             '#factdollar', '#pipe'),
      /[^\n]*/,
    ),

    // Preprocessor loop: #do var = start, end ... #enddo
    preproc_do: $ => seq(
      '#do',
      $.identifier,
      '=',
      $._expression,
      optional(seq(',', $._expression)),
      repeat($._statement),
      '#enddo',
    ),

    // Module directives: .sort, .end, .clear, .store, .global
    module_directive: _ => seq(
      '.',
      choice('sort', 'end', 'clear', 'store', 'global'),
    ),

    // Label statement
    label_statement: $ => seq(
      choice('label', 'goto'),
      $.identifier,
      ';',
    ),

    // if ... endif;
    if_statement: $ => seq(
      'if',
      '(',
      $._expression,
      ')',
      repeat($._statement),
      optional(seq('else', repeat($._statement))),
      'endif',
      ';',
    ),

    // repeat; ... endrepeat;
    repeat_statement: $ => seq(
      'repeat',
      ';',
      repeat($._statement),
      'endrepeat',
      ';',
    ),

    // argument ... endargument;
    argument_block: $ => seq(
      'argument',
      ';',
      repeat($._statement),
      'endargument',
      ';',
    ),

    // do var = start, end; ... enddo;
    do_loop: $ => seq(
      'do',
      $.identifier,
      '=',
      $._expression,
      ',',
      $._expression,
      ';',
      repeat($._statement),
      'enddo',
      ';',
    ),

    // Generic expression statement (catch-all)
    expression_statement: $ => seq(
      $._expression,
      ';',
    ),

    // Expressions
    _expression: $ => choice(
      $.identifier,
      $.number,
      $.string_literal,
      $.dollar_variable,
      $.wildcard,
      $.function_application,
      $.binary_expression,
      $.unary_expression,
      $.parenthesized_expression,
      $.index_expression,
    ),

    function_application: $ => prec(2, seq(
      field('name', $.identifier),
      '(',
      optional(commaSep1($._expression)),
      ')',
    )),

    binary_expression: $ => choice(
      prec.left(1, seq($._expression, choice('+', '-'), $._expression)),
      prec.left(2, seq($._expression, choice('*', '/'), $._expression)),
      prec.left(0, seq($._expression, choice('==', '!=', '<=', '>=', '<', '>'), $._expression)),
      prec.left(-1, seq($._expression, choice('&&', '||'), $._expression)),
      prec.right(3, seq($._expression, '^', $._expression)),
    ),

    unary_expression: $ => prec(4, seq(choice('-', '+', '!'), $._expression)),

    parenthesized_expression: $ => seq('(', $._expression, ')'),

    index_expression: $ => prec(1, seq(
      $._expression,
      '[',
      commaSep1($._expression),
      ']',
    )),

    // Terminals
    identifier: _ => /[a-zA-Z_][a-zA-Z0-9_]*/,
    dollar_variable: _ => token(seq('$', /[a-zA-Z_][a-zA-Z0-9_]*/)),
    wildcard: _ => token(choice(
      seq(/[a-zA-Z_][a-zA-Z0-9_]*/, '?'),
      seq('?', /[a-zA-Z_][a-zA-Z0-9_]*/),
    )),
    number: _ => /\d+(\.\d+)?/,
    string_literal: _ => seq('"', /[^"]*/, '"'),

    // Comments
    line_comment: _ => token(seq('*', /[^\n]*/)),
    block_comment: _ => token(seq('/*', /[^*]*\*+([^/*][^*]*\*+)*/, '/')),
  },
});

function commaSep1(rule) {
  return seq(rule, repeat(seq(',', rule)));
}
