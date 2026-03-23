cmake_minimum_required(VERSION 3.20)
project(Embedded_Project1 C)

# Use C17 standard for modern embedded simulation
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 1. Tell CMake where to find your header files (Requirement A1, A2, A3)
include_directories(inc)

# 2. Collect all your source files
file(GLOB SOURCES
        "src/*.c"
)

# 3. Create the executable for simulation (Part C)
add_executable(${PROJECT_NAME} ${SOURCES}
        include/STD_TYPES.h
        include/BIT_MATH.h
        include/MemScanner.h
        src/MemScanner.c
        src/MsgCodec.c
)

# Optional: Add compiler flags to catch common embedded mistakes
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()