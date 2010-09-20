#!/usr/bin/env bash

echo "$1 -> $2"

sed -i.old -e "s/$1/$2/g" *.[ch]
