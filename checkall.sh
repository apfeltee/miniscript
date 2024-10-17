#!/bin/bash

usevg=${USEVALGRIND}
heredir="$(readlink -e "$(dirname "$0")")"
exe="$heredir/run"

if [[ $1 ]]; then
  exe="$1"
  shift
fi

#set -x

failcount=0
failfiles=()
for file in "$heredir"/*.mc; do
  echo "running '$file' ..."
  cmd=()
  if [[ $usevg == 1 ]]; then
    cmd+=(valgrind)
  fi
  cmd+=("$exe" "$@" "$file")
  if ! "${cmd[@]}"; then
    failcount=$((failcount + 1))
    failfiles+=("$file")
  fi
done

echo "-----------------------"
echo "-----------------------"
if [[ $failcount == 0 ]]; then
  echo "**all good**"
else
  echo "FAILED $failcount tries!"
  for file in "${failfiles[@]}"; do
    echo "-> $file"
  done
fi


