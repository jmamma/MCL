#!/bin/sh

printf '%x\n' "$((2#${1}))"
echo "$((2#${1}))"
