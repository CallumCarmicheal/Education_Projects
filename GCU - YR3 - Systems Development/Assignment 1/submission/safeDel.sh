#! /bin/bash

USAGE="usage: $0 [OPTION]... file..."
RECYCLE_FOLDER="$HOME/.trashCan"
trap_skip_fileCount=false
SWD=$(pwd)

function fGetTrashBinSize() {
    pushd "$RECYCLE_FOLDER" &>/dev/null

    local duFolderSize=$(du -b | cut -f1)
    duFolderSize=$(($duFolderSize - 4096))
    eval $1="'$duFolderSize'"

    popd &>/dev/null
}

function fList() {
    pushd "$RECYCLE_FOLDER" &>/dev/null

    trap_skip_fileCount=true

    fileCount=$(find . -type f | wc -l)
    fGetTrashBinSize "size"

    echo "There are currently $fileCount files in the trash can using $size bytes of space,"
    echo "file size defaults in bytes, unless it can be simplified to Kb/Mb etc."
    echo

    format="%9s   %9s   %-s\n"
    printf "$format" "File Type" "File Size" "File name"
    printf "$format" "---------" "---------" "---------"

    for filename in *; do
        [[ -e "$filename" ]] || continue

        filetype="Unknown"
        if [[ -f "$filename" ]]; then
            filetype="File"
        elif [[ -d "$filename" ]]; then
            filetype="Dir";
        fi

        filesize=$(ls -lah "$filename" | awk '{print $5}')
        humansize=$(numfmt --to=iec-i --from=auto "$filesize")
        printf "$format" "$filetype" "$humansize" "$filename"
    done

    popd &>/dev/null
    echo
}

function fRecover() {
    for fileName in "$@"; do
        echo "Recovering file '$fileName'"
        if ! [[ -f "$RECYCLE_FOLDER/$fileName" ]] ; then
            echo -e "    \033[31mERROR\e[0m '$fileName' does not exist in trash can!"
            fExit 2
        fi

        if [[ -f "$fileName" ]] ; then
            echo -e "    \033[31mERROR\e[0m '$fileName' already exists in current folder!"
            fExit 3
        fi

        mv "$RECYCLE_FOLDER/$fileName" .

        if [ $? -eq 0 ]; then
            echo "    OK"
        else
            echo "    FAILED"
        fi
    done

    return
}

function fDeleteFiles() {
    for fileName in "$@"; do
        echo "Deleting file '$fileName' from trash can"

        if ! [[ -f "$fileName" ]] ; then
            echo -e "    \033[31mERROR\e[0m '$fileName' is not a file!"
            continue
        fi

        if [[ -f "$RECYCLE_FOLDER/$fileName" ]] ; then
            rm "$RECYCLE_FOLDER/$fileName" &>/dev/null
            echo "    OK"
            continue
        else
            echo -e "    \033[31mERROR\e[0m '$fileName' is not in the trashcan!!"
            continue
        fi
    done
}

function fRecycleFiles() {
    for fileName in "$@"; do
        echo "Moving '$fileName' to trash can"

        if ! [[ -f "$fileName" ]] ; then
            echo -e "    \033[31mERROR\e[0m '$fileName' is not a file!"
            continue
        fi

        if [[ -f "$RECYCLE_FOLDER/$fileName" ]] ; then
            echo -e "    \033[31mERROR\e[0m '$fileName' is already in the trash bin (cannot be deleted)!"
            continue
        fi

        mv "$fileName" "$RECYCLE_FOLDER/$fileName"

        if [ $? -eq 0 ]; then
            echo "    OK"
        else
            echo "    FAILED"
        fi
    done
}

function fTotal() {
    pushd "$RECYCLE_FOLDER" &>/dev/null

    trap_skip_fileCount=true

    fileCount=$(find . -type f | wc -l)
    fGetTrashBinSize "size"

    echo "There are currently $fileCount files in the trash can using $size bytes of space."

    popd &>/dev/null
}

function fMonitor_Start() {
    pushd "$SWD" &>/dev/null
    xfce4-terminal -x ./monitor.sh
    popd &>/dev/null
}

function fMonitor_Kill() {
    pushd "$SWD" &>/dev/null

    if [[ -f ".monitor.pid" ]]; then
        PID=$(more .monitor.pid)

        if [ -n "$PID" -a -e /proc/$PID ]; then
            echo "Killing monitor..."
            kill -9 $PID

            rm .monitor.pid &>/dev/null
        else
            echo "Monitor is not running!"

            rm .monitor.pid &>/dev/null
        fi
    fi

    popd &>/dev/null
}

function fExit() {
    code=0
    if [[ ! -v $1 ]]; then code=$1; fi
    exit "$code"
}

function Application_Exit() {
    echo

    pushd "$RECYCLE_FOLDER" &>/dev/null

    if [ "$trap_skip_fileCount" = false ] ; then
        fileCount=$(find . -type f | wc -l)
        fGetTrashBinSize "size"

        echo "There are currently $fileCount files in the trash can using $size bytes of space,"
        echo "file size defaults in bytes, unless it can be simplified to Kb/Mb etc."
        echo "   (Developed by Callum Carmicheal - S1829709)"
        echo
    fi

    fGetTrashBinSize "size"

    if [ "$size" -gt "1024" ]; then
         echo -e "\033[93mWARNING\e[0m trash can exceeds 1KB!"
    fi

    popd &>/dev/null
    echo
}

function f_mReadKeyUp() {
    characterEscape=$(printf "\u1b")

    read -rsn1 key

    if [[ $key == $characterEscape ]]; then
        read -rsn2 key
    fi

    eval $1="'$key'" &>/dev/null
}

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
}

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
}

function f_buttonSelectFile() {
    SELECTED_FILE_STATUS=1
    SELECTED_FILE_NAME=""
    local selectionIndex=$1
    local directorySelection=true

    if [[ ! -z $2 ]]; then directorySelection="$2"; fi

    if [ "$directorySelection" = "true" ]; then
        if [ "$1" = "2" ]; then
            cd ..;
            SELECTED_FILE_STATUS=2;
            return;
        fi
    fi

    local fileIndex=3

    for file in * .[^.]*; do
        if [ "$fileIndex" = "$selectionIndex" ]; then
            if [ "$directorySelection" = "true" ]; then
                if [ -d "$file" ]; then
                    cd "$file";
                    SELECTED_FILE_STATUS=3;
                    return
                fi
            fi

            if [ -f "$file" ]; then
                f_ConfirmFileSelection "$file";

                SELECTED_FILE_NAME=$(readlink -f "$file")

                if [ "$SELECTION_STATE" = "2" ];
                then SELECTED_FILE_STATUS=4;
                else SELECTED_FILE_STATUS=5;
                fi

                return
            fi
        fi

        ((fileIndex+=1));
    done
}

function f_renderSelectFileMenu() {
    local looping=true
    local activeIndex=1
    local highlightIndex=1
    local startingIndex=1
    local maxItems=20
    local swd=$(pwd)
    local fileResult=0
    local directorySelection=true

    if [[ ! -z $1 ]]; then maxItems="$1"; fi
    if [[ ! -z $2 ]]; then directorySelection="$2"; fi

    pushd . &>/dev/null

    while [ "$looping" = "true" ]; do
        SELECTED_FILE_STATUS=1
        SELECTED_FILE_NAME=""

		if ((highlightIndex > fileCount)); then
            ((highlightIndex=fileCount)); fi
        if ((highlightIndex < 1)); then
            ((highlightIndex=1)); fi

        local newActiveIndex="$activeIndex"

        local cwd=$(pwd)
        local fileCount=$(find . -maxdepth 1 | wc -l)
        ((fileCount++))

        local endingIndex=0
        ((endingIndex=startingIndex+maxItems-1))

        clear

        echo "Please select a file."

        if [ "$directorySelection" = "false" ]; then
            echo -e "\033[93mWARNING\e[0m DIRECTORY SELECTION IS DISABLED!"; echo; fi

		echo "(ENTER = SELECT, UPARROW = UP, DOWNARROW = DOWN, Q = QUIT / BACK TO MENU)"
        echo
        echo "[$cwd]:"
		echo "   ($highlightIndex/$fileCount)"

        if [ "$startingIndex" = "1" ]; then
            f_mRenderItem "." "$activeIndex" "$highlightIndex" " "; fi

        ((activeIndex+=1));

        if [ "$startingIndex" = "2" ] || [[ "$startingIndex" -lt "2" && "$maxItems" -ge "2" ]]; then
            f_mRenderItem ".." "$activeIndex" "$highlightIndex" "d"; fi

        ((activeIndex+=1));

        for fileName in * .[^.]*; do
            local _RenderItem=false
            if [ "$activeIndex" = "$startingIndex" ] || [ "$activeIndex" = "$endingIndex" ]; then
                _RenderItem=true; fi
            if [ "$activeIndex" -ge "$startingIndex" ] && [ "$activeIndex" -lt "$endingIndex" ]; then
                _RenderItem=true; fi

            if ((activeIndex > fileCount)); then
                _RenderItem=false; fi

            if [ "$fileName" = ".[^.]*" ]; then
                _RenderItem=false; fi;

                if [ "$_RenderItem" = "true" ]; then
                    if [ -f "$fileName" ]; then
                        f_mRenderItem "$fileName" "$activeIndex" "$highlightIndex" "f";

                    elif [ -d "$fileName" ]; then
                        if [ "$directorySelection" = "true" ]; then
                            f_mRenderItem "$fileName" "$activeIndex" "$highlightIndex" "d";
                        else
                            f_mRenderItem "$fileName" "$activeIndex" "$highlightIndex" "d";
                        fi
                    else
                        f_mRenderItem "$fileName" "$activeIndex" "$highlightIndex" "?";
                fi
			fi

            ((activeIndex+=1))
        done

        f_mReadKeyUp "keyPress"
        case "$keyPress" in
            'q') SELECTED_FILE_STATUS=5; return;;
            '[A') ((highlightIndex-=1)) ;;
            '[B') ((highlightIndex+=1)) ;;
            '')     f_buttonSelectFile "$highlightIndex" "$directorySelection";
                    if [ "$SELECTED_FILE_STATUS" = "4" ]; then
                        return; fi

                    highlightIndex=2
                ;;
        esac

        local newStartingIndex="$startingIndex"
        local newEndingIndex="$endingIndex"

        if ((highlightIndex < startingIndex )); then
            if ((startingIndex - maxItems < 1)); then
                ((newStartingIndex=1))
            else
                ((newStartingIndex=startingIndex - maxItems))
            fi

            ((newEndingIndex=newStartingIndex + maxItems))
        fi

        if ((highlightIndex > endingIndex )); then
            if ((endingIndex < fileCount)); then
                ((newStartingIndex=endingIndex+1));

                ((newEndingIndex=newStartingIndex+maxItems))

                if ((newEndingIndex > fileCount)); then
                    ((newEndingIndex=fileCount)); fi
            fi
        fi

        startingIndex="$newStartingIndex"
        activeIndex=1
    done

    popd &>/dev/null
}

function f_menuTrashItems() {
    SELECTED_FILE_STATUS=1
    SELECTED_FILE_NAME=""

    f_renderSelectFileMenu 20 "true"

    if [ "$SELECTED_FILE_STATUS" = "4" ]; then
        baseFileName=$(basename "$SELECTED_FILE_NAME")
        baseDirName=$(dirname "$SELECTED_FILE_NAME")

        pushd "$baseDirName" &>/dev/null
        fRecycleFiles "$baseFileName"
        popd &>/dev/null
        echo
    fi

    sleep 2
}

function f_menuDeleteItems() {
    SELECTED_FILE_STATUS=1
    SELECTED_FILE_NAME=""

    pushd "$RECYCLE_FOLDER" &>/dev/null

    f_renderSelectFileMenu 20 "false"

    if [ "$SELECTED_FILE_STATUS" = "4" ]; then
        baseFileName=$(basename "$SELECTED_FILE_NAME")

        fDeleteFiles "$baseFileName"
        echo
    fi

    popd &>/dev/null
    sleep 2
}

function f_menuRecoverFile() {
    SELECTED_FILE_STATUS=1
    SELECTED_FILE_NAME=""

    local currentFolder=$(pwd)

    cd "$RECYCLE_FOLDER"
    f_renderSelectFileMenu 20 "false"

    if [ "$SELECTED_FILE_STATUS" = "4" ]; then
        baseFileName=$(basename "$SELECTED_FILE_NAME")
        baseDirName=$(dirname "$SELECTED_FILE_NAME")

        cd "$currentFolder"
        sleep 1
        fRecover "$baseFileName"

        echo
    fi

    sleep 2
}

if ! [[ -d "$RECYCLE_FOLDER" ]] ; then echo "Warning: folder not found, creating recycling folder.";
mkdir -p "$RECYCLE_FOLDER"; fi

trap Application_Exit EXIT

while getopts :r:dlkmts args
do
  case $args in
     l) fList ;;
     r) shift; fRecover $@;;
     d) f_menuDeleteItems;;
     s) f_menuTrashItems;;
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