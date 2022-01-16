" ajnin build file syntax.
" Language: https://github.com/b1f6c1c4/ajnin
"
" Copyright (C) 2021-2022 b1f6c1c4
"
" This file is part of ajnin.
"
" ajnin is free software: you can redistribute it and/or modify it under the
" terms of the GNU Affero General Public License as published by the Free
" Software Foundation, version 3.
"
" ajnin is distributed in the hope that it will be useful, but WITHOUT ANY
" WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
" FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for
" more details.
"
" You should have received a copy of the GNU Affero General Public License
" along with ajnin.  If not, see <https://www.gnu.org/licenses/>.

if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn case match

syn match ajninComment "#.*$" contains=@Spell,ajninTodo
syn region ajninLiteral start="^> " end=".*$" contains=ajninEnv

syn match ajninKeyword "\<rule\>" nextgroup=ajninRule skipwhite
syn match ajninKeyword "\<list\>" nextgroup=ajninList skipwhite
syn match ajninKeyword "\<foreach\>" nextgroup=ajninList skipwhite
syn match ajninKeyword "\<include\>" nextgroup=ajninKeyword skipwhite
syn match ajninKeyword "\<if\>" skipwhite
syn match ajninKeyword "\<else\>" skipwhite
syn match ajninKeyword "\<also\>" skipwhite
syn match ajninKeyword "\<sort\>" skipwhite
syn match ajninKeyword "\<uniq\>" skipwhite
syn match ajninKeyword "\<desc\>" skipwhite
syn match ajninKeyword "\<print\>" skipwhite
syn match ajninKeyword "\<clear\>" skipwhite
syn match ajninKeyword "\<file\>" skipwhite
syn match ajninKeyword "\<template\>" skipwhite
syn match ajninKeyword "\<execute\>" skipwhite
syn match ajninKeyword "\<meta\>" skipwhite
syn match ajninKeyword "\<pool\>" skipwhite
syn match ajninKeyword "\<default\>" skipwhite

syn match ajninKeyword "\(if\s\+\)\@<=-n\>" nextgroup=ajninKeyword skipwhite
syn match ajninKeyword "\(if\s\+\)\@<=-z\>" nextgroup=ajninKeyword skipwhite

syn match ajninOperator ":=" nextgroup=ajninPath
syn match ajninOperator "::="
syn match ajninOperator "[+-]="
syn match ajninOperator "|=" nextgroup=ajninPath
syn match ajninOperator "\*"
syn match ajninOperator "\~"
syn match ajninOperator "[\[\]]"

syn match ajninGlob "\$\$"

" Ctrl-D
syn match ajninDollar "\%U1b" contained

syn match ajninList "\(list\s\+\)\@<=[a-zA-Z]"
syn match ajninList "[a-zA-Z]\(\s*[*{]\)\@="
syn match ajninListRef "\$[a-zA-Z][0-9]\?"
syn match ajninListRef "\$@"

syn match ajninEnv "\${[^}]*}" contained

syn match ajninRule "\(rule\s\+\)\@<=[0-9a-zA-Z ._-]\+"
syn match ajninRule "\(--\|>>\)\@<=[0-9a-zA-Z._-]\+\(--\|>>\|<<\|&\|$\)\@="
syn match ajninRule ">>\(\s*[0-9a-zA-Z._-]\)\@!"
syn match ajninRule "\([0-9a-zA-Z._-]\s*\)\@<!<<"

syn match ajninTemplate "\(template\s\+\)\@<=[0-9a-zA-Z._-]\+"
syn match ajninTemplate "<[0-9a-zA-Z._-]\+>"

syn match ajninAssignment "&[^+=]\++\?="

syn region ajninPath start="(" end=")" contains=ajninListRef,ajninGlob,ajninEnv,ajninDollar
syn region ajninPath start="\(:=\)\@<=" end=" \({$\)\@=\|$" contains=ajninListRef,ajninGlob,ajninEnv,ajninDollar
syn region ajninPath start="\(:=\)\@<=" end=" \({{$\)\@=\|$" contains=ajninListRef,ajninGlob,ajninEnv,ajninDollar

syn region ajninSingleString start="'" skip="\$'" end="'" contains=ajninEnv,ajninDollar
syn region ajninDoubleString start="\"" skip="\$\"" end="\"" contains=ajninListRef,ajninEnv,ajninDollar

syn keyword ajninTodo TODO FIXME contained

hi def link ajninComment Comment
hi def link ajninLiteral Debug
hi def link ajninKeyword Keyword
hi def link ajninOperator Keyword
hi def link ajninGlob Identifier
hi def link ajninDollar Identifier
hi def link ajninList Function
hi def link ajninListRef Identifier
hi def link ajninEnv Identifier
hi def link ajninRule Type
hi def link ajninTemplate Function
hi def link ajninAssignment Number
hi def link ajninSingleString Float
hi def link ajninDoubleString Float
hi def link ajninPath String
hi def link ajninTodo Todo

let b:current_syntax = "ajnin"

let &cpo = s:cpo_save
unlet s:cpo_save

" au BufRead,BufNewFile *.ajnin set filetype=ajnin
