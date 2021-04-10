" ajnin build file syntax.
" Language: https://github.com/b1f6c1c4/ajnin

" if exists("b:current_syntax")
"   finish
" endif

let s:cpo_save = &cpo
set cpo&vim

syn case match

syn match ajninComment "#.*$" contains=@Spell,ajninTodo
syn match ajninLiteral "^> .*$"
syn match ajninLiteral "^$> .*$"

syn match ajninKeyword "^list\>" nextgroup=ajninList skipwhite
syn match ajninKeyword "^rule\>" nextgroup=ajninRule skipwhite

syn match ajninOperator ":=" nextgroup=ajninPath
syn match ajninOperator "::="
syn match ajninOperator "+="
syn match ajninOperator "|="
syn match ajninOperator "\*"

syn match ajninGlob "\$\$"

syn match ajninList "\(list\s\+\)\@<=[a-zA-Z]"
syn match ajninList "[a-zA-Z]\(\s*[*{]\)\@="
syn match ajninListRef "\$[a-zA-Z][0-9]\?"

syn match ajninRule "\(rule\s\+\)\@<=\S\+"
syn match ajninRule "\(--\|>>\)\@<=\S\+\(--\)\@="
syn match ajninRule ">>"

syn region ajninPath start='(' end=')' contains=ajninListRef,ajninGlob
syn region ajninPath start='\(:=\)\@<=' end='$' contains=ajninListRef,ajninGlob

syn keyword ajninTodo TODO FIXME contained

hi def link ajninComment Comment
hi def link ajninLiteral String
hi def link ajninKeyword Keyword
hi def link ajninOperator Keyword
hi def link ajninGlob Identifier
hi def link ajninList Function
hi def link ajninListRef Identifier
hi def link ajninRule Type
hi def link ajninPath String
hi def link ajninTodo Todo

let b:current_syntax = "ajnin"

let &cpo = s:cpo_save
unlet s:cpo_save

" au BufRead,BufNewFile *.ajnin set filetype=ajnin
