cmake_minimum_required(VERSION 3.0.0)

project(CTQ)

option(CTQ_WRITER "Build writer" OFF)
option(BUILD_TESTING "Build tester" OFF)

find_package(LibXml2)
find_path(LZ4_INCLUDE_DIR NAMES lz4hc.h lz4.h)
find_library(LZ4_LIBRARY NAMES lz4)

include_directories(include/)
include_directories(third-party/xcdat/include)
include_directories(${LZ4_INCLUDE_DIR})
link_libraries(${LZ4_LIBRARY})

if (CTQ_WRITER OR BUILD_TESTING)
    add_library(ctq-writer src/writer.cc)
    target_link_libraries(ctq-writer PUBLIC LibXml2::LibXml2)
endif()

if (NOT CTQ_WRITER OR BUILD_TESTING)
    add_library(ctq-reader src/reader.cc)
endif()

add_subdirectory(test)