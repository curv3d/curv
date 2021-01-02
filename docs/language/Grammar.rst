Grammar
=======

The Lexical Syntax
------------------
Curv programs are written in ASCII. Unicode and UTF-8 will come later.

Tokens are separated by white-space and/or comments.
White-space characters are space, form-feed, new-line, carriage return,
horizontal tab, and vertical tab.
Comments have two forms:

* A single-line comment begins with ``//`` and extends to the end of the line.
* A multi-line comment is a sequence of characters between ``/*`` and ``*/``.
  The sequence ``*/`` cannot occur within the comment, it will terminate
  the comment.

### Identifiers
::
  identifier ::= plain_id | quoted_id

  plain_id ::= /[a-zA-Z_] [a-zA-Z_0-9]*/, except for reserved words
  reserved_word ::= '_' | 'by' | 'do' | 'else' | 'for' | 'if' | 'in'
        | 'include' | 'let' | 'local' | 'parametric' | 'test'
        | 'until' | 'var' | 'where' | 'while'
  quoted_id ::= /['] id_segment* [']/
      id_segment ::= plain_char | id_escape
      plain_char ::= /[space or printable ASCII character, except ']/
      id_escape ::= ['] '_'

Quoted identifiers exist so that you can include spaces or punctuation
in an identifier, or use a reserved word as an ordinary identifier.
One motivation is that identifiers appear as widget labels in the GUI
when you display a parametric shape.

A quoted identifier may contain any printable ASCII character, including
space, but not including control characters. It is delimited by apostrophe
characters. You write ``'_`` to represent a literal apostrophe (this is the
only escape sequence). See `Char_Escape`_ for the rationale.

### Symbols
::
  symbol ::= /'#' identifier/

### Numerals
::
  numeral ::= mantissa exponent? | hexnum
  
  mantissa ::= digits | '.' digits | digits '.' digits
  exponent ::= [eE] [+-]? digits
  digits ::= [0-9]+
  hexnum ::= '0' [xX] [0-9a-fA-F]+

### Strings
:::
  string ::= '"' segment* '"'

  segment ::= [white space or printable ASCII character, except for " or $]+
    | '"_'
    | '$_'
    | '${' list '}'
    | '$[' list ']'
    | '$(' list ')'
    | '$' identifier
    | '$' not-followed-by [_a-zA-Z{[(]
    | newline (space|tab)* '|'
    | newline (space|tab)* followed-by '"'

Within a quoted string, the special characters ``"`` and ``$`` are escaped
using ``"_`` and ``$_``. See `Char_Escape`_ for the rationale. String syntax
is intertwined with the grammar due to the ``$`` substitution syntax.

.. _`Char_Escape`: rationale/Char_Escape.rst

The Surface Grammar
-------------------
The surface grammar is a simplified grammar that describes the hierarchical
structure of Curv programs, but doesn't ascribe meaning to parse tree nodes.
Not all program texts that have a parse tree are syntactically correct.

There are 11 operator precedence levels, with ``list`` being the lowest
precedence and ``postfix`` being the highest precedence::

  program ::= list

  list ::= empty | item | commas | semicolons
    commas ::= item ',' | item ',' item | item ',' commas
    semicolons ::= optitem | semicolons ';' optitem
    optitem ::= empty | item

  item ::= ritem | ritem 'where' ritem
  
  ritem ::= pipeline
    | '...' ritem
    | 'include' ritem
    | 'local' ritem
    | 'test' ritem
    | pipeline '=' ritem
    | pipeline ':=' ritem
    | pipeline ':' ritem
    | pipeline '->' ritem
    | pipeline '<<' ritem
    | 'if' parens ritem
    | 'if' parens ritem 'else' ritem
    | 'for' '(' ritem 'in' ritem ')' ritem
    | 'for' '(' ritem 'in' ritem 'until' ritem ')' ritem
    | 'while' parens ritem
    | 'do' list 'in' ritem
    | 'let' list 'in' ritem
    | 'parametric' list 'in' ritem

  pipeline ::= disjunction
    | pipeline '>>' disjunction
    | pipeline '`' postfix '`' disjunction
    | pipeline '::' disjunction

  disjunction ::= conjunction | disjunction '||' conjunction

  conjunction ::= relation | conjunction '&&' relation

  relation ::= sum
    | sum '==' sum | sum '!=' sum
    | sum '<' sum  | sum '>' sum
    | sum '<=' sum | sum '>=' sum
    | sum '..' sum | sum '..' sum 'by' sum
    | sum '..<' sum | sum '..<' sum 'by' sum

  sum ::= product | sum '+' product | sum '-' product | sum '++' product

  product ::= power | product '*' power | product '/' power

  power ::= postfix
    | '-' power | '+' power | '!' power | 'var' power
    | postfix '^' power

  postfix ::= primary
    | postfix primary
    | postfix '.' primary

  primary ::= identifier | symbol | numeral | string | parens | brackets | braces

  parens ::= '(' list ')'
  brackets ::= '[' list ']'
  braces ::= '{' list '}'

The Deep Grammar: Phrases
-------------------------
There is a deeper phrase-structure grammar that assigns syntactic meanings
to most parse tree nodes, which are now called phrases.
(Some parse tree nodes do not have an independent meaning, and are not phrases.)
There are 5 primitive phrase types:

definition
  A phrase that binds zero or more names to values, within a scope.

pattern
  A pattern can occur as a function formal parameter,
  or as the left side of a definition, and contains usually one
  (but generally zero or more) parameter names.
  During pattern matching,
  we attempt to match an argument value against a pattern.
  If the match is successful, we bind (each) parameter name
  to (elements of) the argument value.

expression
  A phrase that computes a value.

generator
  A generalized expression that produces a sequence of zero or more values,
  for consumption by a list constructor or record constructor.
  Generators share syntax with the statement language (particularly one-arm
  conditionals and for loops), but are declarative and referentially
  transparent.

statement
  A phrase that is executed to cause an effect.
  The statement language lets you write imperative code using mutable
  local variables, assignment statements, and while loops, but side effects
  do not escape from the statement context. Functions remain pure
  and Curv expressions remain referentially transparent.

.. Comma vs Semicolon
.. ------------------
.. In a definition context, the comma and semicolon operator are
.. interchangeable: they both construct compound definitions.
.. 
.. In a statement context,
.. * A comma phrase is a compound generator. Items in a comma phrase cannot
..   be assignment statements or local definitions, but they can be
..   expressions, debug actions and other generators.
.. * A semicolon phrase is a compound statement, which is strictly more
..   general than a compound generator. A compound statement provides a
..   scope for local definitions. Items in a compound statement can include
..   local definitions and assignment statements.
.. 
.. Semicolon is strictly more general than comma. You can just ignore the
.. comma operator and use semicolon everywhere.
.. 
.. Comma is like a "weak semicolon" that prohibits imperative semantics.
.. If you see 'a,b' then you know that imperative variable mutation cannot
.. occur in the transition from evaluating 'a' to evaluating 'b'.
.. The phrase 'a,b' is referentially transparent, but 'a;b' may not be.
.. A Curv program with no semicolons has no imperative variable-mutation
.. semantics.
.. 
.. The recommended style (for "declarative first" programming) is to use
.. comma everywhere it is permitted, and use semicolon otherwise.

Programs
--------
There are two kinds of programs.
A source file is always interpreted as an expression.
A command line (in the ``curv`` command line interpreter)
can be an expression, a generator, a statement, or a definition.

Phrase Abstraction
------------------
Curv has a set of generic operations for constructing more complex phrases
out of simpler phrases. These operations work on multiple phrase types,
with the same syntax and semantics, and support conditional evaluation,
iteration, and local variables.

Parenthesized phrase: ``(phrase)``
  Any phrase can be wrapped in parentheses without changing its meaning.

Single-arm conditional: ``if (condition) phrase``
  The phrase (a generator or statement)
  is only executed if the condition is true.

Double-arm conditional: ``if (condition) phrase1 else phrase2``
  The phrases may be expressions, generators or statements.

Bounded iteration: ``for (pattern in list_expression) phrase``
  The phrase (a generator or statement) is executed once for each element
  in the list. At each iteration,
  the element is bound to zero or more local variables by the pattern.

Bounded iteration: ``for (pattern in list_expr until cond) phrase``
  The phrase (a generator or statement) is executed once for each element
  in the list, but stopping early if ``cond`` becomes true. At each iteration,
  the element is bound to zero or more local variables by the pattern.

Local variables: ``let definition in phrase``
  Define local variables over the phrase.
  The phrase may be an expression, generator or statement.
