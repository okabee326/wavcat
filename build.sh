#!/bin/bash
#
g++-14 wavcat.cpp wave_divider.cpp wave_reader.cpp wave_writer.cpp -o wavcat -O3 -D __PRODUCTION__
