cmake_minimum_required (VERSION 3.1)
project (ESCAPE C)

#set(CMAKE_C_STANDARD ...)
set(CMAKE_C_FLAGS "-g -Wall -Wextra -O3 -std=c11 -Wshadow -D_GNU_SOURCE")

macro (add_executable _name)
  _add_executable(${ARGV})
  if (TARGET ${_name})
    target_link_libraries(${_name} err)
  endif()
endmacro()

add_library(err err.c)

add_executable(manager manager.c)
target_link_libraries(manager rt pthread)

add_executable(player player.c)
target_link_libraries(player rt pthread)

install(TARGETS DESTINATION .)