Grammar
=======

The Surface Grammar
-------------------
The surface grammar is a simplified grammar that describes the hierarchical
structure of Curv programs, but doesn't ascribe meaning to parse tree nodes.
Not all programs that have a parse tree are syntactically correct.

There are 12 operator precedence levels, with ``list`` being the lowest precedence
and ``postfix`` being the highest precedence::

  program ::= list

  list ::= empty | item | commas | semicolons | item 'where' list
    commas ::= item ',' | item ',' item | item ',' commas
    semicolons ::= optitem | semicolons `;` optitem
    optitem ::= empty | item

  item ::= pipeline
    | '...' item
    | 'include' item
    | pipeline '=' item
    | pipeline ':=' item
    | pipeline ':'
    | pipeline ':' item
    | pipeline '->' item
    | pipeline '<<' item
    | 'if' parens item
    | 'if' parens item 'else' item
    | 'for' '(' item 'in' item ')' item
    | 'while' parens item
    | 'let' list 'in' item
    | 'do' list 'in' item

  pipeline ::= disjunction
    | pipeline '>>' disjunction
    | pipeline '`' postfix '`' disjunction

  disjunction ::= conjunction | disjunction '||' conjunction

  conjunction ::= relation | conjunction && relation

  relation ::= range
    | range '==' range | range '!=' range
    | range '<' range  | range '>' range
    | range '<=' range | range '>=' range

  range ::= sum
    | sum '..' sum
    | sum '..' sum 'by' sum
    | sum '..<' sum
    | sum '..<' sum 'by' sum

  sum ::= product | sum '+' product | sum '-' product

  product ::= unary | product '*' unary | product '/' unary

  unary ::= power | '-' unary | '+' unary | '!' unary | 'var' unary

  power ::= postfix | postfix '^' unary

  postfix ::= primary
    | postfix primary
    | postfix '.' primary

  primary ::= identifier | numeral | string | parens | brackets | braces
    identifier ::= /[a-zA-Z_] [a-zA-Z_0-9]*/, except for reserved words
      reserved_word ::= '_' | 'by' | 'do' | 'else' | 'for' | 'if'
        | 'in' | 'include' | 'let' | 'var' | 'where' | 'while'

    numeral ::= hexnum | mantissa | /mantissa [eE] [+-]? digits/
      mantissa ::= /digits/ | /'.' digits/ | /digits '.'/ | /digits '.' digits/
      digits ::= /[0-9]+/
      hexnum ::= /'0x' [0-9a-fA-F]+/

    string ::= /'"' segment* '"'/
      segment ::= /[white space or printable ASCII character, except for " or $]+/
        | /'""'/
        | /'$$'/
        | /'${' list '}'/
        | /'$[' list ']'/
        | /'$(' list ')'/
        | /'$' identifier/

    parens ::= '(' list ')'
    brackets ::= '[' list ']'
    braces ::= '{' list '}'

  C style comments, either '//' to end of line, or '/*'...'*/'

The Deep Grammar: Phrases
-------------------------
There is a deeper phrase-structure grammar that assigns syntactic meanings
to most parse tree nodes, which are now called phrases.
(Some parse tree nodes do not have an independent meaning, and are not phrases.)
There are 6 phrase types:

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
  A phrase that causes a side effect, and doesn't compute a value.

element generator
  A phrase that computes a sequence of zero or more values.
  ``[``\ *element_generator*\ ``]`` is a list comprehension.

field generator
  A phrase that computes a sequence of zero or more fields,
  which are name/value or string/value pairs.
  ``{``\ *field_generator*\ ``}`` is a record comprehension.

An action can be used in any context requiring a definition,
element generator, or field generator. An expression can be used
in any context requiring a field generator.

Programs
--------
There are two kinds of programs.
A source file is always interpreted as an expression.
A command line (in the ``curv`` command line interpreter)
can be an expression, an action, or a definition.

Phrase Abstraction
------------------
Curv has a set of generic operations for constructing more complex phrases
out of simpler phrases. These operations work on multiple phrase types,
and support sequencing, conditional evaluation, iteration, and local variables.

Parenthesized phrase: ``(phrase)``
  Any phrase can be wrapped in parentheses without changing its meaning.

Compound phrase: ``phrase1; phrase2``
  * If both phrases are definitions, then this is a compound definition.
    The order doesn't matter, and the definitions may be mutually recursive.
  * If both phrases are actions, element generators, or field generators,
    then the phrases are executed in sequence.

Single-arm conditional: ``if (condition) phrase``
  If the phrase is an action, element generator, or field generator,
  then the phrase is only executed if the condition is true.

Double-arm conditional: ``if (condition) phrase1 else phrase2``
  The phrases may be expressions, actions, element generators, or field generators.

Bounded iteration: ``for (pattern in list_expression) phrase``
  The phrase may be an action, element generator, or field generator.
  The phrase is executed once for each element in the list.
  At each iteration,
  the element is bound to zero or more local variables by the pattern.

Local variables: ``let definition in phrase``
  Define local variables over the phrase.
  The phrase can be an expression, action, element generator or field generator.

Local variables: ``phrase where definition``
  An alternate syntax for defining local variables.

Local actions: ``do action in phrase``
  The phrase can be an expression, action, element generator or field generator.
  The action is executed first, then the phrase is evaluated.
