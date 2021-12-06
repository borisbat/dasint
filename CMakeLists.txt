set(CMAKE_OSX_ARCHITECTURES "arm64")

cmake_minimum_required(VERSION 3.16)

# set the project name
project(DasInterpreter)

set(EXECUTABLE_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/bin/)

# add the executable
add_executable(dasint
src/test.c
src/profile.c
src/profile.h
dasint.h
)
target_include_directories(dasint PUBLIC
${PROJECT_SOURCE_DIR}
${PROJECT_SOURCE_DIR}/src
)
