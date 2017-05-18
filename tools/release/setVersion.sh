#!/bin/bash -x
if (( $# != 2 )); then
    echo "Improper usage of this script. Please invoke with arguments <OLD_VERSION> <NEW_VERSION> "
    exit 1
fi

os=`uname`

function _sed {
    if [[ "$os" =~ "Linux" ]]; then
        sed -i -e "$1" ${@:2}
    else
        sed -i '' -e "$1" ${@:2}
    fi
}

oldVersion=$1
newVersion=$2

_sed 's/'$oldVersion'/'$newVersion'/g' \
package.json \
NOTICE-JS

echo "prepare git patch"

countFoundOldVersions=$(git grep -F ${oldVersion} * | grep -v ReleaseNotes | wc -l)
if (($countFoundOldVersions > 0)); then
    echo "WARNING: a grep over your workspace emphasised that the oldVersion is still present in some of your resources. Please check manually!"
    git grep -F ${oldVersion} * | grep -v ReleaseNotes
else
    git add -A && git commit -m "[Release] set version to $newVersion"
fi
