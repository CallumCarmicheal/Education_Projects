#!/bin/bash
# Lab 2: options1.sh - getopts/shift options utility demo

USAGE="usage: $0 [-l] [-p] [-g string] [file] ..." 

while getopts :lpg: args 
do
  case $args in
     l) ls -al;;
     p) pwd;;
     g) grep $OPTARG *;;
     :) echo "data missing, option -$OPTARG" 1>&2;;
    \?) echo "unknown option: -$OPTARG" 1>&2;;
  esac
done

((pos = OPTIND - 1))
shift $pos

for file in "$@"
do
  if [[ -f $file ]] && [[ -r $file ]]
  then echo "$file::"
       more $file
  else echo "can't cat $file -$OPTARG" 1>&2 
  fi
done

