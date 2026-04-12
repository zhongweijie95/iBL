cmake_minimum_required(VERSION 3.22)

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
    #    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_PROJECT_NAME GD32G553_BL)

include(${CMAKE_SOURCE_DIR}/gcc-arm-none-eabi.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

add_subdirectory(cmake)

### USER DEFINED AREA

MACRO(HEADER_DIRECTORIES return_list)
    FILE(GLOB_RECURSE new_list User/*.h User/*.hpp)
    SET(dir_list "")
    FOREACH(file_path ${new_list})
        GET_FILENAME_COMPONENT(dir_path ${file_path} PATH)
        SET(dir_list ${dir_list} ${dir_path})
    ENDFOREACH()
    LIST(REMOVE_DUPLICATES dir_list)
    SET(${return_list} ${dir_list})
ENDMACRO()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MCU} -fsingle-precision-constant -flto -ffast-math")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MCU} -fsingle-precision-constant -flto -ffast-math")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -ffunction-sections -fdata-sections")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -Wall")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti -fno-threadsafe-statics -ffunction-sections -fdata-sections")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wno-volatile")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

# User Sources and headers
file(GLOB_RECURSE src User/*.c)
file(GLOB_RECURSE headers User/*.h)

HEADER_DIRECTORIES(header_dir_list)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${header_dir_list})

target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${src} ${headers})

# elf -> bin with arm-none-eabi-objcopy
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND arm-none-eabi-size --format=berkeley "${CMAKE_PROJECT_NAME}.elf"
        COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_PROJECT_NAME}.elf ${CMAKE_PROJECT_NAME}.bin)
### USER DEFINED AREA

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths

)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${src}
    ${headers}
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    gd32

    # Add user defined libraries
)