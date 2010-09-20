#!/usr/bin/env bash

./broccoli -f tests/abominable.brocc > tests/abominable.txt

diff -w tests/abominable.txt tests/abominable-ans.brocc > tests/abominable-diff.txt

errors=`wc -l tests/abominable-diff.txt | cut -c7-8`
errors=$((errors/4))

echo "There were ${errors} errors in the Abominable tests!"
