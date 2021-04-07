lexer grammar TLexer;

// These are all supported lexer sections:

// Lexer file header. Appears at the top of h + cpp files. Use e.g. for copyrights.
@lexer::header {/* lexer header section */}

// Appears before any #include in h + cpp files.
@lexer::preinclude {/* lexer precinclude section */}

// Follows directly after the standard #includes in h + cpp files.
@lexer::postinclude {
/* lexer postinclude section */
#ifndef _WIN32
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
}

// Directly preceds the lexer class declaration in the h file (e.g. for additional types etc.).
@lexer::context {/* lexer context section */}

// Appears in the public part of the lexer in the h file.
@lexer::members {/* public lexer declarations section */}

// Appears in the private part of the lexer in the h file.
@lexer::declarations {/* private lexer declarations/members section */}

// Appears in line with the other class member definitions in the cpp file.
@lexer::definitions {/* lexer definitions section */}

KList: 'list';
KRule: 'rule';

ID: 'a'..'z' | 'A'..'Z';
SubID: ID [0-9];

ListSearch: ':=' -> mode(prePath);
ListEnum: '::=';
ListEnumItem: '+=';
RuleAppend: '|=';

Times: '*';

Mult: '>>';
Single: '--';

fragment LETTER : [a-zA-Z\u0080-\u{10FFFF}];
Token: LETTER ((LETTER | '0'..'9' | '_' | '-')* (LETTER | '0'..'9'))?;

OpenCurly: '{';
CloseCurly: '}';

Comment : '#' ~[\r\n]* NL1 -> skip;
WS: [ \t]+ -> skip;
BNL: '\\' '\r'? '\n' -> skip;
NL1: '\r'? '\n';

OpenPar: '(' -> more, mode(stage);

Dollar: '$' -> more, mode(assign);

mode prePath;
PathWS: [ \t]+ -> skip;
PrePathText: . -> more, mode(path);

mode path;
Path: NL1 -> mode(DEFAULT_MODE);
PathText: . -> more;

mode stage;
Stage: ')' -> mode(DEFAULT_MODE);
StageText: . -> more;

mode assign;
Assign: '=' -> mode(DEFAULT_MODE);
AssignText: ~'=' -> more;
