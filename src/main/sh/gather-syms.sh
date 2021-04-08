#!/bin/sh

OBJDUMP=$(which objdump)
ARGS="--dwarf=frames --syms"
${OBJDUMP} --all-headers $1 | grep architecture
${OBJDUMP} ${ARGS} $1 | gzip -c > symbols.dump
