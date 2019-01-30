#! /bin/bash



RECYCLE_FOLDER="$HOME/.trashCan"

echo "$$" > .monitor.pid

function Application_Exit() {
    popd &>/dev/null
    rm .monitor.pid &>/dev/null
    exit
}

trap Application_Exit EXIT
trap Application_Exit INT

files=()
md5Hashes=()
changedFiles=()
addedFiles=()
removedFiles=()

previousMd5Hashes=()
previousFiles=()

function IndexFolder() {
    files=()
    md5Hashes=()
    changedFiles=()
    addedFiles=()
    removedFiles=()

    for filename in *; do
        [[ -e "$filename" ]] || continue

        md5=$(md5sum "$filename" | awk '{ print $1 }')

        md5Hashes+=("$md5")
        files+=("$filename")
    done
}

pushd "$RECYCLE_FOLDER" &>/dev/null

IndexFolder
previousFiles=(${files[*]})
previousMd5Hashes=(${md5Hashes[*]})

_index=0
while true; do
    clear

    echo "Student: Callum Carmicheal, SID: S1829709"
    echo
    echo "Watching files in [$RECYCLE_FOLDER] ..."

    for item in ${files[@]}; do
        if ! [[ " ${previousFiles[*]} " == *"$item"* ]];
        then addedFiles+=("$item"); fi
    done

    for item in ${previousFiles[@]}; do
        if ! [[ " ${files[*]} " == *"$item"* ]];
        then removedFiles+=("$item"); fi
    done

    _index=0
    for item in ${previousMd5Hashes[@]}; do
        file=${previousFiles[$_index]}

        if [[ ${files[*]} =~ "$file" ]]; then
            md5=${previousMd5Hashes[$_index]}
            newMd5=$(md5sum "$file" | awk '{ print $1 }')

            if ! [[ "$md5" == "$newMd5" ]]; then
                changedFiles+=("$file"); fi;
        fi

        ((_index+=1));
    done

    for item in ${addedFiles[@]}; do
        echo -e " [ \033[32mADDED\e[0m ]   $item"; done
    for item in ${removedFiles[@]}; do
        echo -e " [ \033[31mREMOVED\e[0m ] $item"; done
    for item in ${changedFiles[@]}; do
        echo -e " [ \033[93mCHANGED\e[0m ] $item"; done

    previousFiles=(${files[*]})
    previousMd5Hashes=(${md5Hashes[*]})

    sleep 15
    IndexFolder
done