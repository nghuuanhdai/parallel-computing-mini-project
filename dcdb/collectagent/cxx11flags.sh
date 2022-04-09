#!/bin/bash

CFLAGSOK=0
LFLAGSOK=0

function test_cflags {
CXX=$1
shift 1
FLAGS+=$*

cat << EOF | $CXX $FLAGS -c -x c++ - -o $TMPFILE1 &>/dev/null
#include <system_error>
int main() {
    std::system_error e(0, std::system_category(), "");
    return 0;
}
EOF
}

function test_lflags {
CXX=$1
shift 1
FLAGS+=$*

$CXX $FLAGS -o $TMPFILE2 $TMPFILE1 &>/dev/null
}

if [ "$#" -lt "1" ]; then
    echo "C++11 Compiler Flags Checker"
    echo "Usage: $0 <C++_compiler_name> [additional compiler flags]"
    exit 0
fi 

# Create a temporary file for the C++ compiler output.
tempdir=`basename $0`
TMPFILE1=`mktemp -t ${tempdir}.XXXXXXXXXXXXXXX` || exit 1
TMPFILE2=`mktemp -t ${tempdir}.XXXXXXXXXXXXXXX` || exit 1

# Test c++ 11 flags for g++...
if [ "$CFLAGSOK" -eq "0" ]; then
FLAGS="--std=c++11 "
test_cflags $@
if [ "$?" -eq "0" ]; then
    echo $FLAGS
    CFLAGSOK=1
fi
fi

# Test c++ 11 flags for llvm clang
if [ "$CFLAGSOK" -eq "0" ]; then
FLAGS="-std=c++11 -stdlib=libc++ "
test_cflags $@
if [ "$?" -eq "0" ]; then
    echo $FLAGS
    CFLAGSOK=1
fi
fi

# No idea how to get c++11 code compiled on this machine -> return error!
if [ "$CFLAGSOK" -eq "0" ]; then
    exit 1
fi

# Test c++ 11 linker flags for g++ (nothing required)
if [ "$LFLAGSOK" -eq "0" ]; then
FLAGS=" "
test_lflags $@
if [ "$?" -eq "0" ]; then
    echo $FLAGS
    LFLAGSOK=1
fi
fi

# Test c++ 11 linker flags for llvm clang
if [ "$LFLAGSOK" -eq "0" ]; then
FLAGS="-lc++ "
test_lflags $@
if [ "$?" -eq "0" ]; then
    echo $FLAGS
    LFLAGSOK=1
fi
fi

# No idea how to get c++11 code linked on this machine -> return error!
if [ "$LFLAGSOK" -eq "0" ]; then
    exit 1
fi

