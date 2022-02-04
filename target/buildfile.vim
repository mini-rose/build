" buildfile syntax file

if exists("b:current_syntax")
	finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn case ignore

syn match bfOption      "^\w*"
syn match bfTarget      "^@\w*"
syn match bfComment     "#.*$"
syn match bfEnv         "\$\w\w*"

hi def link bfOption    Keyword
hi def link bfTarget    Function
hi def link bfComment   Comment
hi def link bfEnv       Identifier

let b:current_syntax = "buildfile"

let &cpo = s:cpo_save
unlet s:cpo_save
