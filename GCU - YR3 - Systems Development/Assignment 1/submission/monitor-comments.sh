#! /bin/bash


##  @student    Callum Carmicheal
##  @id         S1829709
##  @username   CCARMI201

RECYCLE_FOLDER="$HOME/.trashCan"

# Set our PID
echo "$$" > .monitor.pid

function Application_Exit() {
    # Attempt to delete our pid
    popd &>/dev/null
    rm .monitor.pid &>/dev/null
    exit
}

#
# Create our exit trap
#
trap Application_Exit EXIT
trap Application_Exit INT

# Variables
files=()
md5Hashes=()
changedFiles=()
addedFiles=()
removedFiles=()

previousMd5Hashes=()
previousFiles=()

# Index the trash folder into files array
# @parameters
function IndexFolder() {
    # Reset the arrays
    files=()
    md5Hashes=()
    changedFiles=()
    addedFiles=()
    removedFiles=()

    # Loop the files in the current folder
    for filename in *; do
        # If $filename is not a file then continue and ignore
        [[ -e "$filename" ]] || continue

        # Calulate the md5
        md5=$(md5sum "$filename" | awk '{ print $1 }')

        # Add the results to the array
        md5Hashes+=("$md5")
        files+=("$filename")
    done
} # end of IndexFolder


# Change directory to the trash folder
pushd "$RECYCLE_FOLDER" &>/dev/null

# Index our folder for the first time, and set our previous files and MD5 values
# so they dont appear as false positives on the first pass.
IndexFolder
previousFiles=(${files[*]})
previousMd5Hashes=(${md5Hashes[*]})

# Constantly loop
_index=0
while true; do
    # Clear the screen
    clear

    echo "Student: Callum Carmicheal, SID: S1829709"
    echo
    echo "Watching files in [$RECYCLE_FOLDER] ..."

    # Check for any added files and existing files
    for item in ${files[@]}; do
        if ! [[ " ${previousFiles[*]} " == *"$item"* ]];
        then addedFiles+=("$item"); fi
    done

    # Check for any deleted files
    for item in ${previousFiles[@]}; do
        if ! [[ " ${files[*]} " == *"$item"* ]];
        then removedFiles+=("$item"); fi
    done

    # Check for any changed files
    _index=0
    for item in ${previousMd5Hashes[@]}; do
        file=${previousFiles[$_index]}
        #echo "hash! $item, $file"

        if [[ ${files[*]} =~ "$file" ]]; then
            # Get the MD5 and Calculate the new one.
            md5=${previousMd5Hashes[$_index]}
            newMd5=$(md5sum "$file" | awk '{ print $1 }')

            # Check if we have a new MD5 (file changed)
            if ! [[ "$md5" == "$newMd5" ]]; then
                changedFiles+=("$file"); fi;
        fi

        ((_index+=1));
    done

    # Render folder changes
    for item in ${addedFiles[@]}; do
        echo -e " [ \033[32mADDED\e[0m ]   $item"; done
    for item in ${removedFiles[@]}; do
        echo -e " [ \033[31mREMOVED\e[0m ] $item"; done
    for item in ${changedFiles[@]}; do
        echo -e " [ \033[93mCHANGED\e[0m ] $item"; done

    # Store our current results into previous fields.
    # Also reset our old data fields.
    previousFiles=(${files[*]})
    previousMd5Hashes=(${md5Hashes[*]})

    # Wait 15 seconds and then reindex the folder
    sleep 15
    IndexFolder
done