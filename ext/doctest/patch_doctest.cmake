# Patch script to update doctest's CMake minimum version requirement
file(READ "${SOURCE_DIR}/CMakeLists.txt" FILE_CONTENTS)
string(REPLACE "cmake_minimum_required(VERSION 3.5)" "cmake_minimum_required(VERSION 3.14)" FILE_CONTENTS "${FILE_CONTENTS}")
file(WRITE "${SOURCE_DIR}/CMakeLists.txt" "${FILE_CONTENTS}")
message(STATUS "Patched doctest CMakeLists.txt to require CMake 3.14")