#!/bin/bash
# Bash autocomplete for the `build` tool.

_build()
{
    local cur prev opts
    opts='-e -f -h -s -j -v --help'
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    stargets='default\|before\|after'

    [ "$(echo $cur | cut -c -1)" = "-" ] && {
        COMPREPLY=($(compgen -W "${opts}" -- "${cur}"))
        return
    }

    [ "$prev" = "-f" ] && {
        compopt -o filenames
        COMPREPLY=($(compgen -f -- "${cur}"))
    } || {
        # Autocomplete the target names from the buildfile, apart from the
        # already selected ones.
        [ -f buildfile ] && opts="$(cat buildfile | grep "^@.*" | \
            sed 's/^@\(\w*\).*/\1/' | grep -v $stargets)"
        for word in ${COMP_WORDS[@]}; do
            [ "$word" = "$cur" ] && continue
            [ "$(echo $word | cut -c -1)" = "-" ] && continue
            opts=$(echo $opts | sed "s/$word//g")
        done
        COMPREPLY=($(compgen -W "${opts}" -- "${cur}"))
    }
}

complete -F _build build
