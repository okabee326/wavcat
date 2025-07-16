#!/bin/bash
#
g++-14 wavcat.cpp wave_divider.cpp wave_reader.cpp wave_writer.cpp test_wave_divider.cc test_wave_reader.cc -o wavcat -lgtest -pthread -lgtest_main -O2
