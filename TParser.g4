/* Copyright (C) 2021-2023 b1f6c1c4
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

parser grammar TParser;

options {
	tokenVocab = TLexer;
}

// These are all supported parser sections:

// Parser file header. Appears at the top in all parser related files. Use e.g. for copyrights.
@parser::header {/* parser/listener/visitor header section */}

// Appears before any #include in h + cpp files.
@parser::preinclude {/* parser precinclude section */}

// Follows directly after the standard #includes in h + cpp files.
@parser::postinclude {
/* parser postinclude section */
#ifndef _WIN32
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
}

// Directly preceeds the parser class declaration in the h file (e.g. for additional types etc.).
@parser::context {/* parser context section */}

// Appears in the private part of the parser in the h file.
// The function bodies could also appear in the definitions section, but I want to maximize
// Java compatibility, so we can also create a Java parser from this grammar.
// Still, some tweaking is necessary after the Java file generation (e.g. bool -> boolean).
@parser::members {/* public parser declarations/members section */}

// Appears in the public part of the parser in the h file.
@parser::declarations {/* private parser declarations section */}

// Appears in line with the other class member definitions in the cpp file.
@parser::definitions {/* parser definitions section */}

// Additionally there are similar sections for (base)listener and (base)visitor files.
@parser::listenerpreinclude {/* listener preinclude section */}
@parser::listenerpostinclude {/* listener postinclude section */}
@parser::listenerdeclarations {/* listener public declarations/members section */}
@parser::listenermembers {/* listener private declarations/members section */}
@parser::listenerdefinitions {/* listener definitions section */}

@parser::baselistenerpreinclude {/* base listener preinclude section */}
@parser::baselistenerpostinclude {/* base listener postinclude section */}
@parser::baselistenerdeclarations {/* base listener public declarations/members section */}
@parser::baselistenermembers {/* base listener private declarations/members section */}
@parser::baselistenerdefinitions {/* base listener definitions section */}

@parser::visitorpreinclude {/* visitor preinclude section */}
@parser::visitorpostinclude {/* visitor postinclude section */}
@parser::visitordeclarations {/* visitor public declarations/members section */}
@parser::visitormembers {/* visitor private declarations/members section */}
@parser::visitordefinitions {/* visitor definitions section */}

@parser::basevisitorpreinclude {/* base visitor preinclude section */}
@parser::basevisitorpostinclude {/* base visitor postinclude section */}
@parser::basevisitordeclarations {/* base visitor public declarations/members section */}
@parser::basevisitormembers {/* base visitor private declarations/members section */}
@parser::basevisitordefinitions {/* base visitor definitions section */}

// Actual grammar start.
main: (nl | stmt | literal)* EOF;

stmt: debugStmt | clearStmt | conditionalStmt | ruleStmt
    | includeStmt | listStmt | pipeStmt
    | foreachGroupStmt | collectGroupStmt | listGroupStmt
    | fileStmt | templateStmt | executeStmt | metaStmt | poolStmt;

stmts: OpenCurly nl stmt+ CloseCurly;

fragmentStmts: OpenDoubleCurly nl stmt+ CloseDoubleCurly;

debugStmt: KPrint KList ID nl;

clearStmt: KClear KList ID nl;

conditionalStmt: ifStmt nl;

ifStmt: KIf (IsEmpty | IsNonEmpty) Dollar SubID stmts (KElse (ifStmt | stmts))?;

ruleStmt: KRule Token* ((RuleAppend | RuleAppend2) stage+ | assignment+) nl;

includeStmt: KInclude KList ID ListSearch Path;

listStmt: KList ID (listSearchStmt | listEnumStmt | listInlineEnumStmt | listModifyStmt);

listModifyStmt: (KSort KDesc? KUnique? | KUnique) nl;

listSearchStmt: ListSearch Path;

listEnumStmt: ListEnum ListItemNL listEnumStmtItem+ nl?;

listEnumStmtItem: (ListEnumItem | ListEnumRItem) ListItemToken+ ListItemNL;

listInlineEnumStmt: ListEnum ListItemToken+ ListItemNL nl?;

foreachGroupStmt: KForeach ID (Times ID)* (stmts | fragmentStmts) nl;

collectGroupStmt: (Bra collectOperation Ket KAlso | collectOperation) (collectGroupStmt | stmts nl);

collectOperation: stage (Single Token assignment*)? Append;

listGroupStmt: KForeach KList ID ListSearch (OpenCurlyPath nl? stmt+ CloseCurly
    | OpenDoubleCurlyPath nl? stmt+ CloseDoubleCurly) nl;

pipeStmt: (pipe | stage) (Exclamation | NL1? templateInst)? nl;

pipeGroup: Bra NL1? artifact* Ket;

artifact: (stage | pipe) (Tilde Tilde?)? NL1?;

pipe: (stage (NL1? alsoGroup)* | pipeGroup) operAlso;

stage: Stage | KDefault;

operAlso: (NL1? operation NL1? (alsoGroup NL1?)*)+;

operation: (Mult | Single) (Token (assignment+ | (NL1 assignment)+ NL1)? Single)? stage;

alsoGroup: KAlso Bra (operAlso Exclamation? | operAlso? NL1? templateInst)? Ket;

assignment: Assign value?;

value: Dollar ID | Dollar SubID | SingleString | DoubleString;

literal: prolog;

prolog: LiteralProlog LiteralNL | LiteralEmptyText;

fileStmt: KInclude KFile ListSearch Path;

templateStmt: KTemplate Token KList ID (stage? NL1 operAlso? | pipeGroup NL1? operAlso) (Exclamation | NL1? templateInst)? nl;

templateInst: TemplateName value+ Exclamation?;

executeStmt: KExecute ListSearch Path nl?;

metaStmt: KMeta RuleAppend stage+ nl?;

poolStmt: KPool Token (RuleAppend stage+ nl | ListSearch Path nl?);

nl: NL1+;
