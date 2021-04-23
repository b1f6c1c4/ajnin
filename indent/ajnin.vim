" ajnin build file indent
" Language:	https://github.com/b1f6c1c4/ajnin

if exists('b:did_indent')
  finish
endif
let b:did_indent = 1

setlocal nolisp
setlocal autoindent
setlocal indentexpr=AjninIndent(v:lnum)
setlocal indentkeys+=0=},0=],-,>,<o>,&

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

  if l:prevl =~ '\[\s*$' || l:prevl =~ '{' || l:prevl =~ '::=\s*$'
    let l:ind += shiftwidth()
  endif
  if l:prevl =~ '^\s*[+-]=' && l:thisl !~ '^\s*[+-]='
    let l:ind -= shiftwidth()
  endif
  if l:prevl =~ '^\s*\(\]\s*\)\?\(--\|>>\).*)\s*$'
    let l:ind -= shiftwidth()
  endif
  if l:prevl =~ '^\s*also.*\]\s*$'
    let l:ind -= shiftwidth()
  endif
  if l:prevl =~ '^\s*\(&\)'
    let l:ind -= shiftwidth()
    if l:thisl =~ '^\s*\(--\)'
      let l:ind -= shiftwidth()
    endif
  endif
  if l:thisl =~ '^\s*\(--\|>>\|also\|&\)'
    let l:ind += shiftwidth()
  endif
  if l:thisl =~ '^\s*[\]}]'
    let l:ind -= shiftwidth()
  endif
  return l:ind
endfunction
