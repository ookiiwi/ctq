cmake_minimum_required(VERSION 3.0.0)

project(CTQ)

set(CMAKE_CXX_STANDARD 17)

option(BUILD_CTQ_WRITER "Build writer" OFF)
option(BUILD_CTQ_READER "Build reader" ON)
option(BUILD_CTQ_CLI "CLI for writer and reader" OFF)
option(BUILD_TESTING "Build tester" OFF)

find_package(LibXml2)
find_path(LZ4_INCLUDE_DIR NAMES lz4hc.h lz4.h)
find_library(LZ4_LIBRARY NAMES lz4)

include_directories(include/)
include_directories(third-party/xcdat/include)

set(BUILD_SHARED_LIBS_SAVED "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF)

if (LZ4_INCLUDE_DIR AND LZ4_LIBRARY)
    include_directories(${LZ4_INCLUDE_DIR})
    link_libraries(${LZ4_LIBRARY})
else()
    message("Not found")
    set(LZ4_BUILD_CLI OFF)
    set(LZ4_BUILD_LEGACY_LZ4C OFF)
    add_subdirectory(third-party/lz4/build/cmake)
    link_libraries(lz4_static)
endif()

set(BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS_SAVED}")

if (BUILD_CTQ_CLI OR BUILD_TESTING)
    set(BUILD_CTQ_READER ON)
    set(BUILD_CTQ_WRITER ON)
endif()

if (BUILD_CTQ_READER)
    set(SOURCES src/reader.cc)
endif()

if (BUILD_CTQ_WRITER)
    if (NOT LibXml2_FOUND)
        message(FATAL_ERROR "LibXml2 is required to build ctq writer")
    endif()

    set(SOURCES ${SOURCES} src/writer.cc)
endif() 

if (SOURCES)
    add_library(ctq ${SOURCES})
    message("SOURCES: ${SOURCES}")
    
    if (BUILD_CTQ_WRITER)
        target_link_libraries(ctq PUBLIC LibXml2::LibXml2)
        add_definitions( -DCTQ_WRITER_VERSION_MAJOR=0 )
        add_definitions( -DCTQ_WRITER_VERSION_MINOR=0 )
        add_definitions( -DCTQ_WRITER_VERSION_PATCH=1 )
    endif()
    
    if (BUILD_CTQ_READER)
        add_definitions( -DCTQ_READER_VERSION_MAJOR=0 )
        add_definitions( -DCTQ_READER_VERSION_MINOR=0 )
        add_definitions( -DCTQ_READER_VERSION_PATCH=0 )
    endif()
endif()

if (BUILD_CTQ_CLI)
    include(FetchContent)
    FetchContent_Declare(
        argparse
        GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    )
    FetchContent_MakeAvailable(argparse)

    add_executable(ctq_cli src/cli.cc)
    target_link_libraries(ctq_cli ctq argparse)
    add_definitions( -DCTQ_WRITE_PROGRESS )

endif()

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
