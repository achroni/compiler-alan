#!/bin/bash

var="" 
opt="no"
for i in $@; do
    case ${i} in
    -O)
        opt="y"
        ;;
    -f)
        f_flag="y"
        ;;
    -i)
        i_flag="y"
        ;;
    *.alan)
        var=${i}
        dir=$(dirname "${var}")
        file=$(basename "${var}")
        extension="${file##*.}"
        filename="${file%.*}"
        ;;
    *)
        echo "Usage: ./alan.sh [options] file..."
        echo "Options:"
        echo -e '-O\t optimized'
        echo -e '-f\t program at stdin and final code at stdout'
        echo -e '-i\t program at stdin and intermediate code at stdout'
        exit
     esac
done

if [ "${var}" != "" ]; then
   
    ./alanc < ${var} > "${dir}"/"${filename}".imm || exit 1
    
    if [ ${opt} == "y" ] ; then
        
        opt-6.0 -O3 -S  "${dir}"/"${filename}".imm -o "${dir}"/"${filename}".imm
    fi

    if [ ${opt} == "y" ]; then
        llc-6.0 -O3  "${dir}"/"${filename}".imm -o "${dir}"/"${filename}".asm
    else
        llc-6.0 -O0  "${dir}"/"${filename}".imm -o "${dir}"/"${filename}".asm
    fi

    
    if [ "${f_flag}" == "y" ]; then
        echo "------------------------------- Final Code ----------------------------------"
        cat "${dir}"/"${filename}".asm
        echo ""
    fi

    if [ "${i_flag}" == "y" ]; then
         echo "---------------------------- Intermediate Code --------------------------------"
        cat "${dir}"/"${filename}".imm
         echo ""
    fi

    clang-6.0 "${dir}"/"${filename}".asm lib.a -o "${filename}".out
fi
