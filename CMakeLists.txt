cmake_minimum_required(VERSION 3.10)
project(KernelDrive)

set(CMAKE_CXX_STANDARD 20)

# Add include directories
include_directories(include)

# Add source files
file(GLOB SOURCES "src/*.cpp")

# Find SDL2 and SDL2_ttf
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

# Create the executable
add_executable(KernelDrive ${SOURCES})

# Link the libraries
target_link_libraries(KernelDrive SDL2::SDL2 SDL2_ttf::SDL2_ttf pthread)

# Copy the font file to the build directory
file(COPY ${CMAKE_SOURCE_DIR}/src/ob.ttf DESTINATION ${CMAKE_BINARY_DIR})
