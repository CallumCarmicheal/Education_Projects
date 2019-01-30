#!/bin/bash
# Lab 1: arg-print.sh - print out command line arguments

echo "\$0 is $0; \$1 is $1; \$2 is $2..."
echo "\$# is $#"
echo "\"\$@\" is" "$@"

exit 0 # exit status to parent shell process