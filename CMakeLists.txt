cmake_minimum_required(VERSION 3.15)
project(cadu_solve VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DWIN32)
elseif(UNIX AND NOT APPLE)
    add_definitions(-DLINUX)
endif()

# Source files
set(SOURCES
    cadu_solve.cpp
    Scrambler.cc
    utils.c
    socket_comm.c
    init_rs.c
    _nrzm.c
    _psr.c
    _rs_decode.c
)

# CCSDS library sources
set(CCSDS_SOURCES
    ccsds/conv/cpu_mode_ppc.c
    ccsds/conv/fec.c
    ccsds/conv/viterbi27.c
    ccsds/conv/viterbi27_port.c
    ccsds/rs/decode_rs_char.c
    ccsds/rs/init_rs_char.c
    ccsds/_bch.c
    ccsds/_check_rs.c
    ccsds/_conv.c
    ccsds/_nrzm.c
    ccsds/_psr.c
    ccsds/_rs_decode.c
)

# RS library sources
set(RS_SOURCES
    rs/decode_rs_char.c
    rs/init_rs_char.c
)

# Header files
set(HEADERS
    common_types.h
    getopt.h
    Scrambler.h
    socket_comm.h
    splash.h
    unistd_win.h
    utils.h
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ccsds
    ${CMAKE_CURRENT_SOURCE_DIR}/ccsds/conv
    ${CMAKE_CURRENT_SOURCE_DIR}/ccsds/rs
    ${CMAKE_CURRENT_SOURCE_DIR}/rs
)

# Create executable
add_executable(cadu_solve
    ${SOURCES}
    ${CCSDS_SOURCES}
    ${RS_SOURCES}
    ${HEADERS}
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(cadu_solve ws2_32)
else()
    target_link_libraries(cadu_solve m pthread)
endif()

# Compiler flags
if(MSVC)
    target_compile_options(cadu_solve PRIVATE /W3)
    target_compile_definitions(cadu_solve PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(cadu_solve PRIVATE -Wall -Wextra)
endif()

# Install target
install(TARGETS cadu_solve DESTINATION bin)

# Print configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
