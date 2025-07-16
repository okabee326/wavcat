#!/bin/bash
#
x86_64-w64-mingw32-g++ wavcat.cpp wave_divider.cpp wave_reader.cpp wave_writer.cpp -o wavcat.exe -O3 -static-libgcc -static-libstdc++ -D __PRODUCTION__
