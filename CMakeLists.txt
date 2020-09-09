cmake_minimum_required(VERSION 3.17)
project(Kruor)

set(CMAKE_CXX_STANDARD 14)

add_executable(Client client/clientDriver.c
        client/clientTools.c client/clientTools.h)
add_executable(Server server/serverDriver.c server/serverTools.c server/serverTools.h)
add_executable(trial trial.c)

