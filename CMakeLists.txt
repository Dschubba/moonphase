cmake_minimum_required(VERSION 3.16)

project(moonphase LANGUAGES CXX)

add_executable(moonphase main.cpp)
target_compile_features(moonphase PRIVATE cxx_std_17)
