cmake_minimum_required(VERSION 3.17)
project(Kruor)

set(CMAKE_CXX_STANDARD 14)

add_executable(Client client/main.c client/clientDriver.c client/clientDriver.h)
add_executable(Server server/main.c server/serverDriver.c server/serverDriver.h server/serverTools.c server/serverTools.h)
add_executable(trial trial.c)
