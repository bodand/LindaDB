#!/usr/bin/env sh

FBRK="################################################################################"
for file in vcpkg/buildtrees/*/*.log; do
  printf "%s\n%s\n%s" $FBRK "$file" $FBRK
  cat "$file"
done
