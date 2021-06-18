cmake_minimum_required(VERSION 3.5)
project(stackOverflowRecovery)

find_package(Threads REQUIRED)

aux_source_directory(src sourceFiles)

add_executable(${PROJECT_NAME} ${sourceFiles})

target_link_libraries(${PROJECT_NAME} Threads::Threads)

