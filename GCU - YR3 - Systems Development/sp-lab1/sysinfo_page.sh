#!/bin/bash

# sysinfo_page - A script to produce an HTML file

###########################
##### Constants
###########################

TITLE="My System Information"
CURRENT_TIME=$(date +"%x %r %Z")
TIME_STAMP="Updated on $CURRENT_TIME by $USER"

###########################
##### Functions
###########################

system_info() {
    echo "<h2>System release info</h2>"

    # Find any release files in /etc

    if ls /etc/*release 1>/dev/null 2>&1; then
        echo "<h2>System release info</h2>"
        echo "<pre>"
        for i in /etc/*release; do

            # Since we can't be sure of the
            # length of the file, only
            # display the first line.

            head -n 1 $i
        done

        uname -orp
        echo "</pre>"
    fi

} # End of system_info

show_uptime() {
    echo "<h2>System uptime</h2>"
    echo "<pre>$(uptime)</pre>"
} # End of show_uptime

drive_space() {
    echo "<h2>Filesystem space</h2>"
    echo "<pre>$(df)</pre>"
} # End of drive_space

home_space() {
    # Only the superuser can get this information

    echo "<h2>Home directory space by user</h2>"
    echo "<pre>"

    format="%8s%10s%10s   %-s\n"
    printf "$format" "Dirs" "Files" "Blocks" "Directory"
    printf "$format" "----" "-----" "------" "---------"

    # Check if the user is sudoer, if not
    # then use home directory.
    if [ $(id -u) = "0" ];
    then    dir_list="/home/*"
    else    dir_list=$HOME
    fi

    for home_dir in ${dir_list}; do
        total_dirs=$(find ${home_dir} -type d | wc -l)
        total_files=$(find ${home_dir} -type f | wc -l)
        total_blocks=$(du -s ${home_dir})

        printf "$format" $total_dirs $total_files $total_blocks
    done

    echo "</pre>"

} # End of home_space

write_page() {
    cat <<- _EOF_
    <html>
        <head>
            <title>${TITLE} $HOSTNAME</title>
        </head>
        <body>
            <h1>${TITLE} $HOSTNAME/h1>
            <p>${TIME_STAMP}</p>
            $(system_info)
            $(show_uptime)
            $(drive_space)
            $(home_space)
        </body>
    </html>
_EOF_
} # End of write_page

usage() {
    echo "usage: sysinfo_page [[[-f file] [-i]] | [-h]]"
} # End of usage

###########################
##### Main: Variables
###########################

interactive=
filename=~/sysinfo_page.html


###########################
##### Main: Startup
###########################

while [ "$1" != "" ]; do
    case $1 in
        -f | --file )           shift
                                filename=$1
                                ;;

        -i | --interactive )    interactive=1
                                ;;

        -h | --help )           usage
                                exit
                                ;;

        * )                     usage
                                exit 1
    esac
    shift
done

# Test code to verify command line processing

if [ "$interactive" = "1" ];
then    echo "interactive is on"
else    echo "interactive is off"
fi

echo "output file = $filename"


###########################
##### Main: Interactive
###########################

if [ "$interactive" = "1" ]; then

    response=
    echo -n "Enter name of the output file [$filename] > "
    read response

    if [ -n "$response" ]; then
        filename=$response
    fi

    if [ -f $filename ]; then
        echo -n "Output file exists. Overwrite? [y/N] > "
        read response

        if [ "$response" != "y" ]; then
            echo "Exiting program."
            exit 1
        fi
    fi
fi



###########################
##### Main:
###########################

# Write page

# write_page > $filename