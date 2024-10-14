#!/bin/bash

usevg=${USEVALGRIND}
exe='./run'
if [[ $1 ]]; then
  exe="$1"
  shift
fi

for file in *.mc; do
  echo "running '$file' ..."
  cmd=()
  if [[ $usevg == 1 ]]; then
    cmd+=(valgrind)
  fi
  cmd+=("$exe" "$@" "$file")
  if ! "${cmd[@]}"; then
    exit 1
  fi

done


