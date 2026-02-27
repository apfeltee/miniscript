#!/bin/sh

regex='GenericList<\s*[\w:]+\s*(\s*\*)?>\s*\*\s*\b\w+\b\s*;'
regex='GenericList<\s*[\w:]+\s*\s*\*>\s*\b\w+\b\s*;'
grep --color=auto -P "$regex" main.cpp  -n

