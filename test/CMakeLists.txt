find_package(Catch2 3 REQUIRED)

add_executable(ctq_test ctq.cc)
target_link_libraries(ctq_test ctq-reader ctq-writer Catch2::Catch2WithMain)

file(COPY dataset DESTINATION ${CMAKE_CURRENT_BINARY_DIR})