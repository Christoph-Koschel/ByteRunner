cmake_minimum_required(VERSION 3.23)
project(IllusionScript)

set(CMAKE_C_FLAGS "-Wall -Wextra -Wswitch-enum -Wmissing-prototypes -Wimplicit-fallthrough -Wconversion -fno-strict-aliasing -O3 -std=c11 -pedantic")
list(APPEND LIB_BASM src/basm/libbasm.h)

# The BASM PART
add_executable(basm src/basm/basm.c ${LIB_BASM})

add_executable(basm2nasm src/basm/basm2nasm.c ${LIB_BASM})

add_executable(dbasm src/basm/dbasm.c ${LIB_BASM})

add_executable(br src/basm/br.c ${LIB_BASM})

add_executable(image src/basm/image.c ${LIB_BASM})

add_library(byterunner SHARED src/basm/wrapper.c src/basm/wrapper.h)