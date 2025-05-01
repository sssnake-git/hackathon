cmake_minimum_required(VERSION 3.10)
project(RedBlackTree C)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

add_library(rbtree
    ${PROJECT_SOURCE_DIR}/red_black_tree.c
)

target_include_directories(rbtree PUBLIC ${PROJECT_SOURCE_DIR})

add_executable(main ${PROJECT_SOURCE_DIR}/main.c)
target_link_libraries(main rbtree)

# Platform-specific settings
# [[
if(WIN32)
    message(STATUS "Compiling on Windows")
elseif(UNIX)
    message(STATUS "Compiling on Unix/Linux")
endif()
# ]]

# Installation path for library (optional)
install(TARGETS rbtree DESTINATION lib)
install(FILES ${PROJECT_SOURCE_DIR}/red_black_tree.c DESTINATION include)

