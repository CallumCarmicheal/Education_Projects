#!/bin/bash
set -x

# For my document structure i am not using the home
# directory i have Labs/sp-lab1, Labs/sp-lab2 so to
# zip up the folders i need to do back a directory
# to ensure this script will work anywhere including in PATH
# i looked up some resources and found this snippit of code
# in stack overflow https://stackoverflow.com/a/1638397 to
# get the abs path of the script

# Absolute path to this script, e.g. /home/user/bin/foo.sh
SCRIPT=$(readlink -f "$0")
# Absolute path this script is in, thus /home/user/bin
SCRIPTPATH=$(dirname "$SCRIPT")

# Set the file name to the hostname and date appended
# the file will be placed into the /home/{username} directory
# or where ever $HOME is defined to (~)
output="$(hostname)_$(date +"%F-(%H.%M.%S)").zip"

# Zipping the files
zip -r "$output" *

# Move the zip
mv $output "~/Desktop"