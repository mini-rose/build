" buildfile syntax file

if exists("b:current_syntax")
	finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn case ignore

syn match bfOption      "^\w*"
syn keyword bfSTarget   before after default contained
syn match bfTarget      "^@\w*" contains=bfSTarget
syn match bfComment     "#.*$"
syn match bfEnv         "\$\w\w*"

hi def link bfOption    Keyword
hi def link bfTarget    Function
hi def link bfSTarget   Constant
hi def link bfComment   Comment
hi def link bfEnv       Identifier

let b:current_syntax = "buildfile"

let &cpo = s:cpo_save
unlet s:cpo_save
