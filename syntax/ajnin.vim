" ajnin build file syntax.
" Language: https://github.com/b1f6c1c4/ajnin

" if exists("b:current_syntax")
"   finish
" endif

let s:cpo_save = &cpo
set cpo&vim

syn case match

syn match ajninComment "#.*$" contains=@Spell,ajninTodo
syn region ajninLiteral start="^> " end=".*$" contains=ajninEnv
syn region ajninLiteral start="^$> " end=".*$" contains=ajninEnv

syn match ajninKeyword "^\s*rule\>" nextgroup=ajninRule skipwhite
syn match ajninKeyword "^\s*list\>" nextgroup=ajninList skipwhite
syn match ajninKeyword "^\s*foreach\>" nextgroup=ajninList skipwhite
syn match ajninKeyword "^\s*include\>" nextgroup=ajninKeyword skipwhite
syn match ajninKeyword "\<if\>" nextgroup=ajninKeyword skipwhite
syn match ajninKeyword "\<else\>" nextgroup=ajninKeyword skipwhite
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

syn match ajninList "\(list\s\+\)\@<=[a-zA-Z]"
syn match ajninList "[a-zA-Z]\(\s*[*{]\)\@="
syn match ajninListRef "\$[a-zA-Z][0-9]\?"

syn match ajninEnv "\${[^}]*}" contained

syn match ajninRule "\(rule\s\+\)\@<=[0-9a-zA-Z _-]\+"
syn match ajninRule "\(--\|>>\)\@<=[0-9a-zA-Z_-]\+\(--\|&\)\@="
syn match ajninRule ">>"

syn match ajninAssignment "&[^+=]\++\?="

syn region ajninPath start="(" end=")" contains=ajninListRef,ajninGlob,ajninEnv
syn region ajninPath start="\(:=\)\@<=" end="$" contains=ajninListRef,ajninGlob,ajninEnv

syn region ajninSingleString start="'" skip="\$'" end="'" contains=ajninEnv
syn region ajninDoubleString start="\"" skip="\$\"" end="\"" contains=ajninListRef,ajninEnv

syn keyword ajninTodo TODO FIXME contained

hi def link ajninComment Comment
hi def link ajninLiteral Debug
hi def link ajninKeyword Keyword
hi def link ajninOperator Keyword
hi def link ajninGlob Identifier
hi def link ajninList Function
hi def link ajninListRef Identifier
hi def link ajninEnv Identifier
hi def link ajninRule Type
hi def link ajninAssignment Number
hi def link ajninSingleString Float
hi def link ajninDoubleString Float
hi def link ajninPath String
hi def link ajninTodo Todo

let b:current_syntax = "ajnin"

let &cpo = s:cpo_save
unlet s:cpo_save

" au BufRead,BufNewFile *.ajnin set filetype=ajnin