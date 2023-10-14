#!/usr/bin/env sh

if [ "x${CG_TOOLCHAIN}" = "xgcc" ]; then
  echo "Using gcc toolchain"
  ln -fs "$(which gcc)" /usr/bin/cc
  echo "linked cc"
  ln -fs "$(which g++)" /usr/bin/c++
  echo "linked c++"
fi

if [ "x${CG_TOOLCHAIN}" = "xclang" ]; then
  echo "Using clang toolchain"
  ln -fs "$(which clang)" /usr/bin/cc
  echo "linked cc"
  ln -fs "$(which clang++)" /usr/bin/c++
  echo "linked c++"
fi
