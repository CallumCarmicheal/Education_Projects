#! /bin/bash

##  @student    Callum Carmicheal
##  @id         S1829709
##  @username   CCARMI201

###########################
#####
##### Constants and vars
#####
###########################
USAGE="usage: $0 <fill in correct usage>"
RECYCLE_FOLDER="$HOME/.trashCan"
trap_skip_fileCount=false
SWD=$(pwd) # starting working directory

####################################
#####
##### Exit Codes
#####
####################################
#Generic:
#  0  Success
#  1  A unspecified issue occurred
#
#Recovery:
#  10  Failed to recover file: FILE_NOT_FOUND
#  11  Failed to recover file: EXISTS_LOCALLY
#
#Deletion:
#  20  Failed to delete file: UNKNOWN (A issue with the mv command)
#  21  Failed to delete file: NOT_FOUND
#
####################################
#####
##### Functions and Command line
#####   implementations.
#####
####################################

#
# Documentation structure
#   {description}
#   @parameters {param}
#   @var        {CONST var_name} [{values}]
#   @returns    {description}
#
# .ref
#   This means consult the documentation (function doc) of the specified function for more details as this
#   variables results may change. This is to keep documentation consistent.
#
#
# Parameter format
#   $N name, $N name, *$N optional_param
#
#   $1 firstName, *$2 lastName
#   - First name is required while lastName is not required
#
# Var format
#
#   {CONST var_name} [{values}]
#
#   SELECTION_STATUS [1=CANCEL, 2=SELECTED]
#   - If SELECTION_STATUS = 1 then CANCELLED, if SELECTION_STATUS = 2 then file has been selected.
#
# Return format
#
#   If this is specified it means one of the parameters accept a variable name to store the output into.
#


# Gets the trashbin size
# @parameters $1 the variable output
# @returns size of the trash bin
function fGetTrashBinSize() {
    # Push our folder
    pushd "$RECYCLE_FOLDER" &>/dev/null

    # Folder size = 4096, just subtract it to get the bytes
    local duFolderSize=$(du -b | cut -f1)
    duFolderSize=$(($duFolderSize - 4096))
    eval $1="'$duFolderSize'"

    # Return the folder
    popd &>/dev/null
} # end of fGetTrashBinSize

# List contents of trash can
# @parameters
function fList() {
    pushd "$RECYCLE_FOLDER" &>/dev/null

    # We want to disable the trap for this function
    # it will just be echo'ing out the same information
    trap_skip_fileCount=true

    # Count the files
    fileCount=$(find . -type f | wc -l)
    fGetTrashBinSize "size"

    echo "There are currently $fileCount files in the trash can using $size bytes of space,"
    echo "file size defaults in bytes, unless it can be simplified to Kb/Mb etc."
    echo

    format="%9s   %9s   %-s\n"
    printf "$format" "File Type" "File Size" "File name"
    printf "$format" "---------" "---------" "---------"

    for filename in *; do
        # If it exists
        [[ -e "$filename" ]] || continue

        #Set the file type
        filetype="Unknown"
        if [[ -f "$filename" ]]; then
            filetype="File"
        elif [[ -d "$filename" ]]; then
            filetype="Dir";
        fi

        # Get the file size then convert it and print it.
        filesize=$(ls -lah "$filename" | awk '{print $5}')
        humansize=$(numfmt --to=iec-i --from=auto "$filesize")
        printf "$format" "$filetype" "$humansize" "$filename"
    done

    popd &>/dev/null
    echo
} # end of fList

# Recover a specified file from the trash can and place it inside
# the current directory.
# @parameters $1 fileName
function fRecover() {
    # Loop the files
    for fileName in "$@"; do
        # Check if $fileName is not a file
        echo "Recovering file '$fileName'"
        if ! [[ -f "$RECYCLE_FOLDER/$fileName" ]] ; then
            echo -e "    \033[31mERROR\e[0m '$fileName' does not exist in trash can!"
            fExit 2
        fi

        # Check if $fileName already exists in the current folder
        if [[ -f "$fileName" ]] ; then
            echo -e "    \033[31mERROR\e[0m '$fileName' already exists in current folder!"
            fExit 3
        fi

        # Move the file to the current directory
        mv "$RECYCLE_FOLDER/$fileName" .

        # Print if success
        if [ $? -eq 0 ]; then
            echo "    OK"
        else
            echo "    FAILED"
        fi
    done

    return
} # end of fRecover

# Deletes a file from the trash can
# @parameters $@ files
function fDeleteFiles() {
    # Loop our files to be deleted
    for fileName in "$@"; do
         # Check if $fileName is not a file
        echo "Deleting file '$fileName' from trash can"

        # Check if $file is a file
        if ! [[ -f "$fileName" ]] ; then
            echo -e "    \033[31mERROR\e[0m '$fileName' is not a file!"
            continue
        fi

        # Check if .trashBin/$file exists
        if [[ -f "$RECYCLE_FOLDER/$fileName" ]] ; then
            rm "$RECYCLE_FOLDER/$fileName" &>/dev/null
            echo "    OK"
            continue
        else
            echo -e "    \033[31mERROR\e[0m '$fileName' is not in the trashcan!!"
            continue
        fi
    done
} # end of fDeleteFiles

# Sends a file from the trash can directory
# @parameters $@ files
function fRecycleFiles() {
    # Loop our files to be deleted
    for fileName in "$@"; do
        echo "Moving '$fileName' to trash can"

        # Check if $file is a file
        if ! [[ -f "$fileName" ]] ; then
            echo -e "    \033[31mERROR\e[0m '$fileName' is not a file!"
            continue
        fi

        # Check if .trashBin/$file exists
        if [[ -f "$RECYCLE_FOLDER/$fileName" ]] ; then
            echo -e "    \033[31mERROR\e[0m '$fileName' is already in the trash bin (cannot be deleted)!"
            continue
        fi

        # Move the file
        mv "$fileName" "$RECYCLE_FOLDER/$fileName"

        # Print if success
        if [ $? -eq 0 ]; then
            echo "    OK"
        else
            echo "    FAILED"
        fi
    done
} # end of fRecycleFiles

# Display the total number of files and bytes taken up by the trash can directory
# @parameters
function fTotal() {
    pushd "$RECYCLE_FOLDER" &>/dev/null

    # We want to disable the trap for this function
    # it will just be echo'ing out the same information
    trap_skip_fileCount=true

    # Get the amounts
    fileCount=$(find . -type f | wc -l)
    fGetTrashBinSize "size"

    echo "There are currently $fileCount files in the trash can using $size bytes of space."

    popd &>/dev/null
} # end of fTotal

# Starts the monitor daemon
# @parameters
function fMonitor_Start() {
    pushd "$SWD" &>/dev/null
    xfce4-terminal -x ./monitor.sh
    popd &>/dev/null
} # end of fMonitor_Start

# Kills the current monitoring process
# @parameters
function fMonitor_Kill() {
    pushd "$SWD" &>/dev/null

    # Check if we have a stored pid (active monitor)
    if [[ -f ".monitor.pid" ]]; then
        # Get the pid
        PID=$(more .monitor.pid)

        # Check if the pid is running
        if [ -n "$PID" -a -e /proc/$PID ]; then
            # Kill the monitor
            echo "Killing monitor..."
            kill -9 $PID

            # Remove the pid file
            rm .monitor.pid &>/dev/null
        else
            echo "Monitor is not running!"

            # Remove the pid file
            rm .monitor.pid &>/dev/null
        fi
    fi

    popd &>/dev/null
} # end of fKill

# Exit the application (also show the warning)
# @parameters *$1 Exit Code
function fExit() {
    code=0
    if [[ ! -v $1 ]]; then code=$1; fi
    exit "$code"
} # end of fExit

# Automatically executed at the end of the script
# @parameters
function Application_Exit() {
    echo

    # Check if we are over our set SIZE limit (1024 bytes)
    pushd "$RECYCLE_FOLDER" &>/dev/null

    if [ "$trap_skip_fileCount" = false ] ; then
        # Count the files
        fileCount=$(find . -type f | wc -l)
        fGetTrashBinSize "size"

        echo "There are currently $fileCount files in the trash can using $size bytes of space,"
        echo "file size defaults in bytes, unless it can be simplified to Kb/Mb etc."
        echo "   (Developed by Callum Carmicheal - S1829709)"
        echo
    fi

    # Count the files
    fGetTrashBinSize "size"

    if [ "$size" -gt "1024" ]; then
         echo -e "\033[93mWARNING\e[0m trash can exceeds 1KB!"
    fi

    popd &>/dev/null
    echo
} # end of Application_Exit

####################################
#####
##### Interactive Mode functions
#####
####################################

# Read keyboard key up
# @parameters $1 Output variable
function f_mReadKeyUp() {
    # Store the escape character
    characterEscape=$(printf "\u1b")

    # Read 1 character
    read -rsn1 key

    # Check if we are using a escape character, if so then read
    # 2 more characters from the input.
    if [[ $key == $characterEscape ]]; then
        read -rsn2 key # Read 2 more characters into the
    fi

    eval $1="'$key'" &>/dev/null
} # end of f_ReadKeyUp

# Render Menu Selection Item with Highlighting
# @parameters $1 Text, $2 Active Index, $3 Highlight Index, *$4 Prefix
function f_mRenderItem() {
    local text=$1
    local active=$2
    local highlight=$3
    local prefix=" "
    if [[ ! -v $4 ]]; then prefix="$4 "; fi

    if [[ "$active" -eq "$highlight" ]]; then
        echo -e "$prefix\e[7m > $text\e[7m < \e[0m"
    else
        echo -e "$prefix  $text\e[0m"
    fi
} # end of f_mRenderItem

# Shows a confirmation message to select the file and then returns the result
# @parameters $1 file
# @var        SELECTION_STATE [1=CANCELLED, 2=SELECTED]
function f_ConfirmFileSelection() {
    SELECTION_STATE=1

	echo
	echo "File selected: $1"
    read -r -p "Are you sure? [y/N] " response

	if [[ "$response" =~ ^([yY])+$ ]]; then
	    clear
		SELECTION_STATE=2
		return
    fi
} # end of f_ConfirmFileDeletion

# Triggers a file selection dialog that confirms and then recycles the file
# @parameters $1 selectionIndex, *$2 Allow directory selection
# @var        SELECTED_FILE_STATUS [1=NONE, 2=BACK_DIR, 3=ENTER_DIR, 4=FILE_SELECT, 5=CANCEL]
# @var        SELECTED_FILE_NAME   [full path of the file]
function f_buttonSelectFile() {
    SELECTED_FILE_STATUS=1
    SELECTED_FILE_NAME=""
    local selectionIndex=$1
    local directorySelection=true

    if [[ ! -z $2 ]]; then directorySelection="$2"; fi

    # Check if selectionIndex is 2 then we want to go back a directory
    if [ "$directorySelection" = "true" ]; then
        if [ "$1" = "2" ]; then
            cd ..;
            SELECTED_FILE_STATUS=2;
            return;
        fi
    fi

    local fileIndex=3 # we start at 3

    # Loop our files to find our selected index
    for file in * .[^.]*; do
        # Check if we are on the file index
        if [ "$fileIndex" = "$selectionIndex" ]; then
            # Check if file is a directory
            if [ "$directorySelection" = "true" ]; then
                if [ -d "$file" ]; then
                    cd "$file";
                    SELECTED_FILE_STATUS=3; # ENTER_DIR
                    return
                fi
            fi

            # Check if file is a file
            if [ -f "$file" ]; then
                # Confirm if we are deleting the file, sets the SELECTED_FILE_STATUS flag.
                f_ConfirmFileSelection "$file";

                # Set our selected file
                SELECTED_FILE_NAME=$(readlink -f "$file")

                # Return our state
                if [ "$SELECTION_STATE" = "2" ]; # SELECTED
                then SELECTED_FILE_STATUS=4; # FILE_SELECT
                else SELECTED_FILE_STATUS=5; # CANCEL
                fi

                return
            fi
        fi

        # Increment our file index
        ((fileIndex+=1));
    done
} # end of f_buttonSelectFile

# Render a select file menu
# @parameters *$1 Amount of items to render, *$2 Allow directory selection
# @var        SELECTED_FILE_STATUS  .ref f_buttonSelectFile
# @var        SELECTED_FILE_NAME    .ref f_buttonSelectFile
function f_renderSelectFileMenu() {
    # set our variables
    local looping=true
    local activeIndex=1
    local highlightIndex=1
    local startingIndex=1
    local maxItems=20       # Customise how many max items you want
    local swd=$(pwd)        # Starting Working Directory
    local fileResult=0
    local directorySelection=true

    if [[ ! -z $1 ]]; then maxItems="$1"; fi
    if [[ ! -z $2 ]]; then directorySelection="$2"; fi

    # Push our current directory onto the stack.
    pushd . &>/dev/null

    while [ "$looping" = "true" ]; do
        SELECTED_FILE_STATUS=1 # None
        SELECTED_FILE_NAME="" # No file

        # Capping the selection to fileCount and 0
        # This is needed for when the the folder is changed
		if ((highlightIndex > fileCount)); then
            ((highlightIndex=fileCount)); fi
        if ((highlightIndex < 1)); then
            ((highlightIndex=1)); fi

		# Store our current active index
        local newActiveIndex="$activeIndex"

        # Render the current folder
        local cwd=$(pwd)
        local fileCount=$(find . -maxdepth 1 | wc -l) # includes . and ..
        ((fileCount++)) # Increment the fileCount because the . directory

        local endingIndex=0
        ((endingIndex=startingIndex+maxItems-1))

        # Clear the output
        clear

		# Write tutorial information and Debug information
        echo "Please select a file."

        if [ "$directorySelection" = "false" ]; then
            echo -e "\033[93mWARNING\e[0m DIRECTORY SELECTION IS DISABLED!"; echo; fi

		echo "(ENTER = SELECT, UPARROW = UP, DOWNARROW = DOWN, Q = QUIT / BACK TO MENU)"
#        echo
#        echo "activeIndex = $activeIndex, highlightIndex = $highlightIndex"
#        echo "startingIndex = $startingIndex, endingIndex = $endingIndex"
#        echo "fileCount = $fileCount, maxItems = $maxItems"
        echo
        echo "[$cwd]:"
		echo "   ($highlightIndex/$fileCount)"

        # If our starting index is 0 render the back item
        if [ "$startingIndex" = "1" ]; then
            f_mRenderItem "." "$activeIndex" "$highlightIndex" " "; fi

        ((activeIndex+=1));

        # If our starting index is 1 or lower then render our current folder item (default selection)
        if [ "$startingIndex" = "2" ] || [[ "$startingIndex" -lt "2" && "$maxItems" -ge "2" ]]; then
            f_mRenderItem ".." "$activeIndex" "$highlightIndex" "d"; fi

        ((activeIndex+=1));

        # Loop our files to be deleted
        for fileName in * .[^.]*; do
            # _RenderItem = (activeIndex == startingIndex || activeIndex > startingIndex) ||
            #               (activeIndex == endingIndex || activeIndex < endingIndex)
            local _RenderItem=false
            if [ "$activeIndex" = "$startingIndex" ] || [ "$activeIndex" = "$endingIndex" ]; then
                _RenderItem=true; fi
            if [ "$activeIndex" -ge "$startingIndex" ] && [ "$activeIndex" -lt "$endingIndex" ]; then
                _RenderItem=true; fi

            # Cap the files displayed, this will stop "*" from showing up in empty folders
            if ((activeIndex > fileCount)); then
                _RenderItem=false; fi

            # This is the all file selector, this appears if there are no
            # files in the current directory.
            if [ "$fileName" = ".[^.]*" ]; then
                _RenderItem=false; fi;

                # If we can render the item
                if [ "$_RenderItem" = "true" ]; then
                    # Render the file
                    if [ -f "$fileName" ]; then
                        f_mRenderItem "$fileName" "$activeIndex" "$highlightIndex" "f";

                    # Render directory
                    elif [ -d "$fileName" ]; then
                        if [ "$directorySelection" = "true" ]; then
                            f_mRenderItem "$fileName" "$activeIndex" "$highlightIndex" "d";
                        else
                            f_mRenderItem "$fileName" "$activeIndex" "$highlightIndex" "d";
                        fi

                    # Render unknown type
                    else
                        f_mRenderItem "$fileName" "$activeIndex" "$highlightIndex" "?";
                fi

                # Debug: Display the files hidden by the selection
                #else echo -e "\033[93m*    $fileName\e[0m"
			fi

            ((activeIndex+=1))
        done

        f_mReadKeyUp "keyPress"
        case "$keyPress" in
            'q') SELECTED_FILE_STATUS=5; return;; # Exit
            '[A') ((highlightIndex-=1)) ;; # Up
            '[B') ((highlightIndex+=1)) ;; # Down
            '')     f_buttonSelectFile "$highlightIndex" "$directorySelection";
                    if [ "$SELECTED_FILE_STATUS" = "4" ]; then
                        return; fi # File selected

                    highlightIndex=2
                ;; # Enter -> Select, reset highlightIndex
        esac

        # Scroll the menu
        local newStartingIndex="$startingIndex"
        local newEndingIndex="$endingIndex"

        # If we are exceeding our starting index
        # GO A PAGE UP
        if ((highlightIndex < startingIndex )); then
            # If we have a new page
            if ((startingIndex - maxItems < 1)); then
                ((newStartingIndex=1))
            else
                ((newStartingIndex=startingIndex - maxItems))
            fi

            ((newEndingIndex=newStartingIndex + maxItems))
        fi

        # If we are exceeding our ending index
        # GO A PAGE DOWN
        if ((highlightIndex > endingIndex )); then
            # If we have a new page
            if ((endingIndex < fileCount)); then
                # Set our starting index to the ending index + 1
                ((newStartingIndex=endingIndex+1));

                # Now set our ending index to newStartingIndex + maxItems
                ((newEndingIndex=newStartingIndex+maxItems))

                # Now cap our endingIndex
                if ((newEndingIndex > fileCount)); then
                    ((newEndingIndex=fileCount)); fi
            fi
        fi

        startingIndex="$newStartingIndex"
        activeIndex=1
    done

    # Reset our directory stack back to where we originally were.
    popd &>/dev/null
} # end of f_renderSelectFileMenu

# Send files to the trashcan interactively
# @parameters
function f_menuTrashItems() {
    # Reset our variables
    SELECTED_FILE_STATUS=1 # None
    SELECTED_FILE_NAME="" # No file

    # Render the file menu
    f_renderSelectFileMenu 20 "true"

#    echo "Selected: $SELECTED_FILE_NAME"
#    echo "Status:   $SELECTED_FILE_STATUS"

    # Select a file
    if [ "$SELECTED_FILE_STATUS" = "4" ]; then
        baseFileName=$(basename "$SELECTED_FILE_NAME")
        baseDirName=$(dirname "$SELECTED_FILE_NAME")

        pushd "$baseDirName" &>/dev/null
        fRecycleFiles "$baseFileName"
        popd &>/dev/null
        echo
    fi

    sleep 2
} # end of f_menuTrashItems

# Delete files interactively
# @parameters
function f_menuDeleteItems() {
    # Reset our variables
    SELECTED_FILE_STATUS=1 # None
    SELECTED_FILE_NAME="" # No file

    pushd "$RECYCLE_FOLDER" &>/dev/null

    # Render the file menu
    f_renderSelectFileMenu 20 "false"

#    echo "Selected: $SELECTED_FILE_NAME"
#    echo "Status:   $SELECTED_FILE_STATUS"

    # Select a file
    if [ "$SELECTED_FILE_STATUS" = "4" ]; then
        baseFileName=$(basename "$SELECTED_FILE_NAME")

        fDeleteFiles "$baseFileName"
        echo
    fi

    popd &>/dev/null
    sleep 2
} # end of f_menuDeleteFiles

# Select a file to recover
# @parameters
function f_menuRecoverFile() {
    # Reset our variables
    SELECTED_FILE_STATUS=1 # None
    SELECTED_FILE_NAME="" # No file

    # Render the file menu
    local currentFolder=$(pwd)

    # Set our CD to the trash directory
    cd "$RECYCLE_FOLDER"
    f_renderSelectFileMenu 20 "false"

    # Select a file
    if [ "$SELECTED_FILE_STATUS" = "4" ]; then
        baseFileName=$(basename "$SELECTED_FILE_NAME")
        baseDirName=$(dirname "$SELECTED_FILE_NAME")

        # Go back to our original directory and recover the file
        cd "$currentFolder"
        sleep 1
        fRecover "$baseFileName"

        echo
    fi

    sleep 2
} # end of f_menuRecoverFile

####################################
#####
##### Misc scripts
#####
####################################

#
# Check if the trashcan folder does not exist
#
if ! [[ -d "$RECYCLE_FOLDER" ]] ; then echo "Warning: folder not found, creating recycling folder.";
mkdir -p "$RECYCLE_FOLDER"; fi

#
# Create our exit trap
#
trap Application_Exit EXIT

####################################
#####
##### Command Line parameters
#####
####################################

#
#   Loop the parameters
#
# debug: "e:"
while getopts :r:dlkmts args #options
do
  case $args in
     l) fList ;;
     r) shift; fRecover $@;;
#    e) fExit $OPTARG;;
     d) f_menuDeleteItems;;
     s) f_menuTrashItems;;  # select items to send to trash can
     t) fTotal;;
     m) fMonitor_Start;;
     k) fMonitor_Kill;;
     :) echo "data missing, option -$OPTARG";;
    \?) echo "$USAGE";;
  esac
done

((pos = OPTIND - 1))
shift $pos

PS3='option> '

###########################
#####
##### Interactive Menu
#####
###########################
if (( $# == 0 )); then
    if (( $OPTIND == 1 )); then
        while true; do
            echo "Created by Callum Carmicheal (S1829709)"
            select menu_list in list recover delete total watch kill exit

            do case $menu_list in
                "list")         echo; fList; echo; break;;
                "recover")      f_menuRecoverFile; break;;
                "delete")       f_menuDeleteItems; break;;
                "total")        echo; fTotal; echo; break;;
                "watch")        fMonitor_Start; break;;
                "kill")         fMonitor_Kill; break;;
                "exit")         exit 0;;
                *) echo "unknown option"; break;;
            esac; done
        done
    fi
else fRecycleFiles "$@"
fi