#compdef build
# ZSH autocomplete for the `build` tool.

_add_flags()
{
	_arguments                                       \
		'-e[explain what is going on]'               \
		'-h[show the help page]'                     \
		'--help'                                     \
		'-s[only setup, do not start compiling]'     \
		'-v[show the version number]'
}

_build()
{
	local targets filtered_targets found

	# Find any targets in the buildfile
	targets=()
	[ -f ./buildfile ] && targets=(                                 \
		$(cat buildfile | grep -e '^@.*' | sed 's/^@\(\w*\).*/\1/'))

	filtered_targets=()
	# Autocomplete the target names from the buildfile, apart from
	# the already selected ones.
	for target in ${targets[@]}; do
		found=0
		for word in ${words[@]}; do
			[ "$target" = "$word" ] && {
				found=1
				break
			}
		done

		[ $found = 0 ] && filtered_targets+=($target)
	done

	_describe 'build' filtered_targets
	_add_flags
}

_build
