#!/bin/bash

BOLD="\033[1m"
RED="\033[0;31m"
GREEN="\033[0;32m"
RESET="\033[0m"

OVERALL_PASS=1

printf "$BOLD*** TEST SUMMARY ***$RESET\n"

FILES="../tests/results/*"
for fullfile in $FILES; do
    filename=$(basename -- "$fullfile")
    extension="${filename##*.}"
    filename="${filename%.*}"
    printf "%-32s " $filename 
    if [[ "$extension" == "pass" ]]; then
        printf "$GREEN PASS $RESET\n"
    else
        printf "$RED FAIL $RESET\n"
        OVERALL_PASS=0
    fi
done

if [ "$OVERALL_PASS" -eq "1" ]; then
    printf "$BOLD*** OVERALL PASS ***$RESET\n"
else
    printf "$BOLD*** OVERALL FAIL ***$RESET\n"
fi