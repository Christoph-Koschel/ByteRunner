#!/bin/bash

set -e

CFLAGS="-Wall -Wextra -Wswitch-enum -Wmissing-prototypes -Wimplicit-fallthrough -Wconversion -fno-strict-aliasing -O3 -std=c11 -pedantic"
CC="/usr/bin/cc"
LIBBASM="src/libbasm/libbasm.h"

PLATFORM_WINDOWS="WINDOWS"
PLATFORM_LINUX="LINUX"

function make_image() {
    PLATFORM=$1

    echo "Make image for $PLATFORM"

    if [ ! -d images ]
    then
        mkdir ./images
    fi

    $CC $CFLAGS -o ./images/$PLATFORM.o -D="$PLATFORM" -c ./src/image.c
}

function compile_raw_tests() {
  for FILE in ./test/src/*.basm
  do
    OUTPUT=${FILE%.*}
    echo "./basm $FILE -o $OUTPUT"
    ./basm $FILE -o $OUTPUT
  done
}

function compile_elf_tests() {
      if [ ! -d ./test/temp ]
      then
          mkdir ./test/temp
      fi

  for FILE in ./test/src/*.basm
    do
      OUTPUT=${FILE%.*}
      echo "./basm $FILE -o ./test/temp/code"
      ./basm $FILE -o ./code
      ld -r -b binary ./code -o ./test/temp/code.o
      mv ./code ./test/temp/code
      $CC ./images/LINUX.o ./test/temp/code.o -o $OUTPUT.exe
  done
}

function run_raw_tests() {
  FAILS=0

  for FILE in ./test/src/*.basm
  do
    NEW=0
    FILE=${FILE%.*}

    printf "%-40s" "Test '$FILE' "

    OUTPUT=$(./br -i $FILE)

    if [ ! -f ./test/expected/`basename $FILE`.txt ]
    then
      NEW=1
      ./br -i $FILE > ./test/expected/`basename $FILE`.txt
    fi

    EXPECTED=$(cat ./test/expected/`basename $FILE`.txt)

    if [ "$EXPECTED" = "$OUTPUT" ]
    then
      printf "[OK]"
    else
      FAILS=1
      printf "[FAILURE]"
    fi

    if [ $NEW = 1 ]
    then
      echo " [NEW]"
    else
      echo ""
    fi
  done

  if [ $FAILS = 1 ]
  then
    echo "Errors occurred"
    exit 1
  fi
}

function run_elf_tests() {
  FAILS=0

  for FILE in ./test/src/*.basm
  do
    NEW=0
    FILE=${FILE%.*}

    printf "%-40s" "Test '$FILE.exe' "

    OUTPUT=$($FILE.exe)

    if [ ! -f ./test/expected/`basename $FILE`.txt ]
    then
      NEW=1
      $FILE.exe > ./test/expected/`basename $FILE`.txt
    fi

    EXPECTED=$(cat ./test/expected/`basename $FILE`.txt)

    if [ "$EXPECTED" = "$OUTPUT" ]
    then
      printf "[OK]"
    else
      FAILS=1
      printf "[FAILURE]"
    fi

    if [ $NEW = 1 ]
    then
      echo " [NEW]"
    else
      echo ""
    fi
  done

  if [ $FAILS = 1 ]
  then
    echo "Errors occurred"
    exit 1
  fi
}

echo "============================================"
echo "==              BUILDING                  =="
echo "============================================"
echo ""
echo "Compile basm"
$CC $CFLAGS -o basm src/basm.c $LIBBASM
echo "Compile br"
$CC $CFLAGS -o br src/br.c $LIBBASM
echo "Compile dbasm"
$CC $CFLAGS -o dbasm src/dbasm.c $LIBBASM
echo "Compile basm2nasm"
$CC $CFLAGS -o basm2nasm src/basm2nasm.c $LIBBASM

make_image $PLATFORM_LINUX

compile_raw_tests
compile_elf_tests

echo ""
echo ""
echo "============================================"
echo "==              RAW TESTS                 =="
echo "============================================"
echo ""
run_raw_tests
echo ""
echo ""
echo "============================================"
echo "==              ELF TESTS                 =="
echo "============================================"
echo ""
run_elf_tests
