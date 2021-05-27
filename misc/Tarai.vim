"
" vim-scripts tarai function
"
" How to run:
"   (shell) % vim -S Tarai.vim
"
"
function TaraiMain(x, y, z)
  let result = Tarai(a:x, a:y, a:z)
  echo result
endfunction

function Tarai(x, y, z)
  if a:y >= a:x
    return a:y
  endif
  return Tarai(Tarai((a:x - 1), a:y, a:z), Tarai((a:y - 1), a:z, a:x), Tarai((a:z - 1), a:x, a:y))
endfunction

:call TaraiMain(12,6,0)
:q!
