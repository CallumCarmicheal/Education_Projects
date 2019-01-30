#!/bin/bash
# Lab 2: rm-execs.sh - remove ELF executables
# http://stackoverflow.com/questions/3231804/in-bash-how-to-add-are-you-sure-y-n-to-any-command-or-alias

USAGE="usage: $0 [directory]" 

if [[ "$#" > 1 ]]; then echo "$USAGE" 1>&2; exit 1; fi

default_dir="$HOME"
    
start_dir=${1:-$default_dir}

confirm () {
    # call with a prompt string or use a default
    read -r -p "${1:-Are you sure? [y/N]} " response
    case $response in
        [yY][eE][sS]|[yY]) 
            true
            ;;
        *)
            false
            ;;
    esac
}

delete () {
	for file in $found
	do
		rm -f "$file"
	done
}

found="$(find $start_dir -exec file {} \; | grep -i elf | cut -d: -f 1)"

if [[ -z "$found" ]];
then echo "no files found..."
else echo "$found";
     confirm "Are you sure you want to delete these files?" && delete;
fi

exit 0
	

