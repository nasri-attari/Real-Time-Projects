cmake_minimum_required(VERSION 3.30)
project(RealTProject3__2_ C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

add_executable(RealTProject3__2_
    include/config.h
    include/gui.h
    include/sem.h
    src/config.c
    src/gang.c
    src/gui.c
    src/main.c
    src/police.c)
