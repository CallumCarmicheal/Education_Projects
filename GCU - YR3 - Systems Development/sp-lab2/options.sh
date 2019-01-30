#!/bin/bash
# Lab 2: options.sh - getopts options utility demo

USAGE="usage: $0 [-l] [-p] [-g string]" 

while getopts :lpg: args #options
do
  case $args in
     l) ls -al;;
     p) pwd;;
     g) grep $OPTARG * 2>/dev/null;;
     :) echo "data missing, option -$OPTARG" 1>&2;;
    \?) echo "unknown option: -$OPTARG" 1>&2;;
  esac
done

