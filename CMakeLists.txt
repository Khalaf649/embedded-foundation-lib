cmake_minimum_required(VERSION 3.20)
project(Embedded_Project1 C)

# Use C17 standard for modern embedded simulation
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 1. Tell CMake where to find your header files (Requirement A1, A2, A3)
include_directories(include)

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
        include/Message.h
        src/Message.c
        include/Gpio.h
        src/Gpio.c
        include/GPIO_private.h
        include/Keypad.h
        include/Keypad_private.h
        src/Keypad.c
        include/SevenSeg.h
        include/SevenSeg_private.h
        src/SevenSeg.c
        include/RCC_private.h
        include/RCC.h
        src/RCC.c
)

# Optional: Add compiler flags to catch common embedded mistakes
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()