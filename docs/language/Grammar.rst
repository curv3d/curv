Grammar
=======

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

  item ::= ritem | ritem `where` ritem
  
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
    | 'for' '(' ritem 'in' ritem 'while' ritem ')' ritem
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

  identifier ::= plain_id | quoted_id
    plain_id ::= /[a-zA-Z_] [a-zA-Z_0-9]*/, except for reserved words
      reserved_word ::= '_' | 'by' | 'do' | 'else' | 'for' | 'if' | 'in' | 'include'
        | 'let' | 'local' | 'parametric' | 'test' | 'var' | 'where' | 'while'
    quoted_id ::= /['] id_segment* [']/
      id_segment ::= plain_char | id_escape
      plain_char ::= /[space or printable ASCII character, except ' or $]/
      id_escape ::= /'$.'/ | /'$-'/

  symbol ::= /'#' identifier/

  numeral ::= hexnum | mantissa | /mantissa [eE] [+-]? digits/
    mantissa ::= /digits/ | /'.' digits/ | /digits '.' digits/
    digits ::= /[0-9]+/
    hexnum ::= /'0x' [0-9a-fA-F]+/

  string ::= /'"' segment* '"'/
    segment ::= /[white space or printable ASCII character, except for " or $]+/
      | /'$.'/
      | /'$='/
      | /'${' list '}'/
      | /'$[' list ']'/
      | /'$(' list ')'/
      | /'$' identifier/
      | /newline (space|tab)* '|'/
      | /newline (space|tab)*/ followed by /'"'/

  parens ::= '(' list ')'
  brackets ::= '[' list ']'
  braces ::= '{' list '}'

  C style comments, either '//' to end of line, or '/*'...'*/'

The Deep Grammar: Phrases
-------------------------
There is a deeper phrase-structure grammar that assigns syntactic meanings
to most parse tree nodes, which are now called phrases.
(Some parse tree nodes do not have an independent meaning, and are not phrases.)
There are 7 primitive phrase types:

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

action
  A statement that may cause debug side effects (eg, printing to the debug
  console), or that may cause the program to panic and report an error (eg,
  due to an assertion failure).

generator
  A statement that produces a sequence of zero or more values,
  for consumption by a list constructor or record constructor.
  An action is a generator that produces zero values.
  An expression is a generator that produces one value.

assignment
  An imperative statement that mutates a local variable.

local definition
  An imperative statement that defines a local variable within the scope of
  a compound statement.

Comma vs Semicolon
------------------
In a definition context, the comma and semicolon operator are interchangeable:
they both construct compound definitions.

In a statement context,
* A comma phrase is a compound generator. Items in a comma phrase cannot
  be assignment statements or local definitions, but they can be
  expressions, debug actions and other generators.
* A semicolon phrase is a compound statement, which is strictly more
  general than a compound generator. A compound statement provides a
  scope for local definitions. Items in a compound statement can include
  local definitions and assignment statements.

Semicolon is strictly more general than comma. You can just ignore the
comma operator and use semicolon everywhere.

Comma is like a "weak semicolon" that prohibits imperative semantics.
If you see 'a,b' then you know that imperative variable mutation cannot
occur in the transition from evaluating 'a' to evaluating 'b'.
The phrase 'a,b' is referentially transparent, but 'a;b' may not be.
A Curv program with no semicolons has no imperative variable-mutation
semantics.

The recommended style (for "declarative first" programming) is to use comma
everywhere it is permitted, and use semicolon otherwise.

Programs
--------
There are two kinds of programs.
A source file is always interpreted as an expression.
A command line (in the ``curv`` command line interpreter)
can be an expression, a statement, or a definition.

Phrase Abstraction
------------------
Curv has a set of generic operations for constructing more complex phrases
out of simpler phrases. These operations work on multiple phrase types,
and support sequencing, conditional evaluation, iteration, and local variables.

Parenthesized phrase: ``(phrase)``
  Any phrase can be wrapped in parentheses without changing its meaning.

Compound phrase: ``phrase1; phrase2;...``
  This is an n-ary operator.
  * If all phrases are definitions, then this is a compound definition.
    The order doesn't matter, and the definitions may be mutually recursive.
  * If all phrases are statements,
    then the statements are executed in sequence.
  * This defines a scope for local definitions, which may appear as
    phrase arguments.

Declarative compound phrase: ``phrase1, phrase2, ...``
  This is an n-ary operator.
  Just like ``;``, except that the resulting compound phrase is
  referentially transparent. The argument phrases may not be imperative
  statements (local definitions or assignments).

Single-arm conditional: ``if (condition) statement``
  The statement is only executed if the condition is true.

Double-arm conditional: ``if (condition) phrase1 else phrase2``
  The phrases may be expressions or statements.

Bounded iteration: ``for (pattern in list_expression) statement``
  The statement is executed once for each element in the list.
  At each iteration,
  the element is bound to zero or more local variables by the pattern.

Unbounded iteration: ``while (condition) statement``
  The statement is executed zero or more times, until condition becomes false.

Local variables: ``let definition in phrase``
  Define local variables over the phrase.
  The phrase can be an expression or statement.
