#!/bin/bash

exe='./run'
if [[ $1 ]]; then
  exe="$1"
  shift
fi

for file in *.mc; do
  echo "running '$file' ..."
  if ! "$exe" "$@" "$file"; then
    exit 1
  fi
done


