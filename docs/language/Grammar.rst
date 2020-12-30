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

Local variables: ``let definition in phrase``
  Define local variables over the phrase.
  The phrase may be an expression, generator or statement.
