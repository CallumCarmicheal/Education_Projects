#!/bin/bash
# Lab 2: menu.sh - general purpose menu driven utility tool

USAGE="usage: $0" 

PS3="select option> "

if (( $# == 0 ))
then
  select menu_list in "list files" "pwd" "locate lines" "exit"
  do
    case $menu_list in
       "list files") ls -al;;
       "pwd") pwd;;
       "locate lines") echo -n "pattern? "
                       read pattern
                       grep $pattern * 2>/dev/null;;
       "exit") exit 0;;
        *) echo "unknown option" 1>&2;;
    esac
  done
else echo $USAGE 1>&2; exit 1
fi

