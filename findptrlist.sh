#!/bin/sh

grep --color=auto -P 'GenericList<\s*[\w:]+\s*(\s*\*)?>\s*\*\s*\b\w+\b\s*;' main.cpp  -n
