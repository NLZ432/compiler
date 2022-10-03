/*
 * Grammar file for WPI Programming Language (WPL).
 */
grammar WPL;

// Parser rules
compilationUnit   : (components+=cuComponent)+ EOF;

cuComponent       : varDeclaration | procedure | function | externDeclaration ;
varDeclaration    : scalarDeclaration | arrayDeclaration ;
scalarDeclaration : (t=type| VAR) scalars+=scalar (',' scalars+=scalar)* ';' ;
scalar            : id=ID vi=varInitializer? ;
arrayDeclaration  : typename=type '[' INTEGER ']' ID ';' ;       // No dynamic arrays, type not inferred
type              : BOOL | INT | STR ;  
varInitializer    : '<-' c=constant ;
externDeclaration : 'extern' (externProcHeader | externFuncHeader) ';';

procedure         : procHeader block ;
procHeader        : 'proc' id=ID '(' p=params? ')' ;
externProcHeader  : 'proc' id=ID '(' ((params ',' ELLIPSIS) | params? | ELLIPSIS?) ')' ;
function          : fh=funcHeader b=block  ;
funcHeader        : t=type 'func' id=ID '(' p=params? ')' ;
externFuncHeader  : t=type 'func' id=ID '(' ((params ',' ELLIPSIS) | params? | ELLIPSIS?) ')' ;

params            : (type ID (',' type ID)*) ;
block            : '{' (statement | varDeclaration | block)+ '}' ;   // Change to expr ???

statement         : assignment
                  | loop
                  | select
                  | conditional
                  | call
                  | block
                  | return
                  ;

loop              : 'while' e=expr 'do' block ;
conditional       : 'if' e=expr 'then'? block ('else' block)? ;
select            : 'select' '{' selectAlt+ '}' ;
selectAlt         : e=expr ':' s=statement ;  
call              : id=ID '(' arguments? ')' ;
arguments         : (arg (',' arg)*) ;  
arg               : (id=ID | c=constant) ; 
return            : 'return' expr? ';' ;

constant          : INTEGER | STRING | BOOLEAN ;
assignment        : targets+=ID (',' targets+=ID)* '<-' exprs+=expr (',' exprs+=expr)* ';' 
                  | arrayIndex '<-' e+=expr ';' ;
arrayIndex        : id=ID '[' expr ']' ;

expr              : 
                  fpname=ID '(' (args+=expr (',' args+=expr)*)? ')'       # FuncProcCallExpr
                  | arrayIndex                                            # SubscriptExpr
                  | <assoc=right> '-' expr                                # UMinusExpr
                  | <assoc=right> '~' expr                                # NotExpr
                  | left=expr (MUL | DIV) right=expr                      # MultExpr
                  | left=expr (PLUS | MINUS) right=expr                   # AddExpr
                  | left=expr (LESS | LEQ | GTR | GEQ) right=expr         # RelExpr
                  | <assoc=right> left=expr (EQUAL | NEQ) right=expr      # EqExpr
                  | left=expr AND right=expr                              # AndExpr
                  | left=expr OR right=expr                               # OrExpr
                  | '(' expr ')'                                          # ParenExpr
                  | arrayname=ID '.' 'length'                             # ArrayLengthExpr
                  | ID                                                    # IDExpr
                  | constant                                              # ConstExpr
                  ;

// Lexer rules
// Keywords
BOOL              : 'boolean' ;
INT               : 'int' ;
STR               : 'str' ;
VAR               : 'var' ;

PROC              : 'proc' ;
FUNC              : 'func' ;
EXTERN            : 'extern' ;
RETURN            : 'return' ;
WHILE             : 'while' ;
SELECT            : 'select' ;
END               : 'end' ;
IF                : 'if' ;
THEN              : 'then' ;
ELSE              : 'else' ;
LENGTH            : 'length' ;

// Operators
AND               : '&' ;
ASSIGN            : '<-' ;
EQUAL					    :	'=' ;
GEQ               : '>=' ;
GTR               : '>' ;
LEQ               : '<=' ;
LESS					    :	'<' ;
MINUS					    :	'-' ;
NEQ               : '~=' ;
OR                : '|' ;
PLUS					    :	'+' ;
DIV 					    :	'/' ;
MUL 					    :	'*' ;
NOT 					    :	'~' ;

// Punctuation and delimiters
COMMA             : ',' ;
SEMICOLON         : ';' ;
ELLIPSIS          : '...' ;
DOT               : '.' ;

LBRACKET          : '[' ;
RBRACKET          : ']' ;
LPAR              : '(' ;
RPAR              : ')' ;
LBRACE					  : '{' ;
RBRACE					  :	'}' ;

// Literals, identifiers, and whitespace
WS 						    :	[ \t\r\n\f]+ -> skip ;
INTEGER					  :	DIGIT+ ;
BOOLEAN           : 'true' | 'false' ;
ID      				  : LETTER (LETTER|DIGIT|UNDERSCORE)* ;
STRING			      : '"' ('\\'. | ~[\n])*? '"' ;

// Comments
COMMENT				     : 	(INLINE_COMMENT|STD_COMMENT) -> skip ;
fragment INLINE_COMMENT  :	'#' .*? ('\n'|EOF);
fragment STD_COMMENT	 :	'(*' (STD_COMMENT | .)*? '*)' ;
fragment UNDERSCORE      :   '_' ;

// Fragments
fragment DIGIT			:	[0-9] ;
fragment UPPER          :   [A-Z] ;
fragment LOWER          :   [a-z] ;
fragment LETTER			:	UPPER | LOWER ;