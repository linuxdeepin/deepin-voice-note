#!/bin/bash
# this file is used to auto-generate .qm file from .ts file.
# Maintainer: V4fr3e <liujinli@uniontech.com>
# this script should only be called in cmake file.
# parameters:
#   $1 <translations dir>
#   $2 <update_trans>   true for update ts
#   $3 [source dir]
#   $4 [project name]

translations_dir=$1
need_update_trans=$2
source_dir=$3
project=$4
en_ts_file=${translations_dir}/${project}.ts

#check if need to update ts file
if echo "$need_update_trans" | grep -qwi "true"
then
    printf "\nBegin--> ${en_ts_file}\n"
    lupdate ${source_dir}  -ts ${en_ts_file}
fi

ts_list=(`ls ${translations_dir}/*.ts`)

if [ -f "/usr/bin/lrelease6" ]; then
    LRELEASE=lrelease6
elif [ -f "/usr/lib/qt6/bin/lrelease" ]; then
    LRELEASE=/usr/lib/qt6/bin/lrelease
elif [ -f "/usr/lib64/qt6/bin/lrelease" ]; then
    RELEASE=/usr/lib64/qt6/bin/lrelease
else
    echo "lrelease: command not found."
    exit 1
fi

for ts in "${ts_list[@]}"
do
    printf "\nprocess ${ts}\n"
    ${LRELEASE} "${ts}"
done
