/* Copyright (C) 2021 b1f6c1c4
 *
 * This file is part of ajnin.
 *
 * ajnin is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free
 * Software Foundation, version 3.
 *
 * ajnin is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with ajnin.  If not, see <https://www.gnu.org/licenses/>.
 */

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
ListEnum: '::=' -> mode(listItem);
ListEnumItem: '+=' -> mode(listItem);
ListEnumRItem: '-=' -> mode(listItem);
RuleAppend: '|=' -> mode(prePath);

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

LiteralProlog : '> ' -> mode(literal);
LiteralEpilog : '$> ' -> mode(literal);

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

mode listItem;
ListItemNL: '\r'? '\n' -> mode(DEFAULT_MODE);
ListItemToken: ~[ \t\r\n]+;
ListItemWS: [ \t]+ -> skip;

mode literal;
LiteralNL: '\r'? '\n' -> mode(DEFAULT_MODE);
LiteralText: . -> more;
