cmake_minimum_required(VERSION 2.8...3.22)
project(cnvad)

option (USE_POCKETFFT "use pocketfft" ON)

if (BUILD_TARGET)
if (BUILD_TARGET MATCHES "Android")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()
else()
  set(BUILD_TARGET "Linux")
endif()
if (BUILD_TARGET MATCHES "Linux")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wno-unused-result")
  # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
endif()

if (CMAKE_BUILD_TYPE MATCHES "Release")
  add_compile_options(-Os)
endif()

set(
    LIB_CCNR_SOURCE
    src/cnvad.c
    src/dqvad/dqvad.c
    src/ccnn_core.c
    src/ring_buf.c
    src/fft_tool.c
)

if (USE_POCKETFFT)
    set(LIB_CCNR_SOURCE src/pocketfft.c
                      ${LIB_CCNR_SOURCE})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_POCKETFFT")
else()
    set(LIB_CCNR_SOURCE src/fft.c
                      ${LIB_CCNR_SOURCE})
endif()

include_directories(src)
include_directories(model)
include_directories(src/dqvad)

add_library(
    cnvad SHARED
    ${LIB_CCNR_SOURCE}
)

add_library(
    cnvad_static STATIC
    ${LIB_CCNR_SOURCE}
)

target_link_libraries(cnvad_static m)

if (BUILD_TARGET MATCHES "Linux")
  target_link_libraries(cnvad m)
else()
  # target_link_libraries(cnvad m)
endif()

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--version-script,${PROJECT_SOURCE_DIR}/src/libcnvad.ver")

add_executable(cnvadtest src/cnvadtest.c)
target_link_libraries(cnvadtest cnvad_static)

add_custom_command(TARGET cnvad POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/libcnvad.so ${CMAKE_CURRENT_BINARY_DIR}/libs/libcnvad.so)

if (BUILD_TARGET MATCHES "Linux")
  add_custom_command(TARGET cnvad POST_BUILD
    COMMAND "${ANDROID_TOOLCHAIN_PREFIX}strip"
    "${CMAKE_CURRENT_BINARY_DIR}/libs/libcnvad.so"
    COMMENT "Strip debug symbols done on final binary.")
endif()
