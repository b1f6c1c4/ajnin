" ajnin build file indent
" Language:	https://github.com/b1f6c1c4/ajnin
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

if exists('b:did_indent')
  finish
endif
let b:did_indent = 1

setlocal nolisp
setlocal autoindent
setlocal indentexpr=AjninIndent(v:lnum)
setlocal indentkeys+=0=},0=],-,>,<o>,&,=

if exists('*AjninIndent')
  finish
endif

function! AjninIndent(lnum)
  let l:prevlnum = prevnonblank(a:lnum-1)
  if l:prevlnum == 0
    return 0
  endif

  let l:prevl = substitute(getline(l:prevlnum), '#.*$', '', '')
  let l:thisl = substitute(getline(a:lnum), '#.*$', '', '')
  let l:previ = indent(l:prevlnum)

  let l:ind = l:previ

  " [ { ::=
  "     <text>
  if l:prevl =~ '\[\s*$' || l:prevl =~ '{' || l:prevl =~ '::=\s*$'
    let l:ind += shiftwidth()
  endif
  "     += -=
  " <text>
  if l:prevl =~ '^\s*[+-]=' && l:thisl !~ '^\s*[+-]='
    let l:ind -= shiftwidth()
  endif
  "         --rule-- (stage)]
  " <text>
  if l:prevl =~ '^\s*\(--\|>>\|<\).*)\s*]\s*$'
    let l:ind -= 2 * shiftwidth()
  endif
  "     --rule-- (stage)
  " <text>
  if l:prevl =~ '^\s*\(--\|>>\).*)\s*\(!\s*\)\?$'
    let l:ind -= shiftwidth()
  endif
  "     <tmpl>
  " <text>
  if l:prevl =~ '^\s*<.*>'
    let l:ind -= shiftwidth()
  endif
  "     also[--rule-- (stage)]
  " <text>
  if l:prevl =~ '^\s*also.*\]\s*$'
    let l:ind -= shiftwidth()
  endif
  "     also[
  "         <text>
  if l:prevl =~ '^\s*also\s*[\s*$'
    let l:ind -= shiftwidth()
  endif
  "         &var=
  "     --rule--
  if l:prevl =~ '^\s*\(&\)'
    let l:ind -= shiftwidth()
    if l:thisl =~ '^\s*\(--\)'
      let l:ind -= shiftwidth()
    endif
  endif
  " <text>
  "     --rule-- <tmpl> also[ &var=
  if l:thisl =~ '^\s*\(--\|>>\|<\|also\|&\)'
    let l:ind += shiftwidth()
  endif
  " ] }
  if l:thisl =~ '^\s*[\]}]'
    let l:ind -= shiftwidth()
  endif
  return l:ind
endfunction
