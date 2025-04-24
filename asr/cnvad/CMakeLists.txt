# --- 基本项目设置 ---
cmake_minimum_required(VERSION 3.10) # 建议提高最低版本以使用更现代的CMake特性
project(cnvad C) # 指定语言为 C

# --- 选项 ---
option (USE_POCKETFFT "Use pocketfft instead of kissfft" ON)

# --- 平台检测与平台特定设置 ---
set(CMAKE_C_STANDARD 99) # 显式设置 C 标准
set(CMAKE_C_STANDARD_REQUIRED ON)

# 通用标志 (后面会被平台特定标志追加)
set(CMAKE_C_FLAGS_INIT "")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "")
set(CMAKE_EXE_LINKER_FLAGS_INIT "")

# 位置无关代码 (PIC) - 对共享库和链接到共享库的可执行文件通常是必需的
# 在大多数现代系统 (包括 Linux 和 macOS) 上，这对于生成共享库是必需的或默认的
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 优化标志
if (CMAKE_BUILD_TYPE MATCHES "Release")
  # -Os 优化大小。可以根据需要调整 (e.g., -O2, -O3)
  add_compile_options(-Os)
endif()

# 平台特定设置
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  message(STATUS "Configuring for Linux")
  set(PLATFORM_LINUX TRUE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-result")
  # 版本脚本仅适用于 GNU ld (Linux)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--version-script,${PROJECT_SOURCE_DIR}/src/libcnvad.ver")
  # Linux 需要显式链接 math 库
  list(APPEND PLATFORM_LIBS m)

elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  message(STATUS "Configuring for macOS")
  set(PLATFORM_MACOS TRUE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-result")
  # macOS (clang) 不使用 GNU 版本脚本。符号可见性通常通过代码属性或 -exported_symbols_list 处理
  # macOS 通常会自动链接 math 库，但显式添加也无害
  list(APPEND PLATFORM_LIBS m) # 通常不需要，但加上也行

elseif(ANDROID) # CMake 会在配置 Android NDK 构建时设置 ANDROID 变量
  message(STATUS "Configuring for Android")
  set(PLATFORM_ANDROID TRUE)
  # -fPIC 通常由 NDK/CMake 自动处理，但可以显式设置
  # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC") # 可能不需要显式设置
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--version-script,${PROJECT_SOURCE_DIR}/src/libcnvad.ver")
  # Android 通常需要链接 math 库
  list(APPEND PLATFORM_LIBS m)

else()
  message(WARNING "Unsupported build platform: ${CMAKE_SYSTEM_NAME}. Trying Linux defaults.")
  set(PLATFORM_LINUX TRUE) # 作为后备
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wno-unused-result")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--version-script,${PROJECT_SOURCE_DIR}/src/libcnvad.ver")
  list(APPEND PLATFORM_LIBS m)
endif()

# --- 源文件定义 ---
set(
    LIB_CCNR_SOURCE_BASE
    src/vad.c
    src/engine/vad_engine.c
    src/ccnn_core.c
    src/ring_buf.c
    src/fft_tool.c
)

if (USE_POCKETFFT)
    list(PREPEND LIB_CCNR_SOURCE_BASE src/pocketfft.c)
    add_compile_definitions(USE_POCKETFFT) # 使用 compile definition 替代 C_FLAGS
else()
    list(PREPEND LIB_CCNR_SOURCE_BASE src/fft.c)
endif()

set(LIB_CCNR_SOURCE ${LIB_CCNR_SOURCE_BASE}) # 最终源文件列表

# --- 包含目录 ---
include_directories(src)
include_directories(model)
include_directories(src/engine)

# --- 库目标 ---
add_library(
    cnvad SHARED
    ${LIB_CCNR_SOURCE}
)
# 设置共享库版本 (可选，但推荐)
# set_target_properties(cnvad PROPERTIES VERSION 1.0.0 SOVERSION 1)

add_library(
    cnvad_static STATIC
    ${LIB_CCNR_SOURCE}
)

# --- 链接库 ---
# 对静态库链接平台特定库 (如 libm)
if(PLATFORM_LIBS)
  target_link_libraries(cnvad_static PUBLIC ${PLATFORM_LIBS})
endif()

# 对共享库链接平台特定库 (如 libm)
# 注意: 链接器标志 (如版本脚本) 已通过 CMAKE_SHARED_LINKER_FLAGS 应用
if(PLATFORM_LIBS)
  target_link_libraries(cnvad PUBLIC ${PLATFORM_LIBS})
endif()

# --- 可执行目标 (测试程序) ---
add_executable(cnvadtest src/main.c)
# 测试程序链接静态库版本
target_link_libraries(cnvadtest PRIVATE cnvad_static)

# --- 安装/复制 和 Strip (更好的方式是使用 install() ) ---
# 创建目标目录
set(LIBS_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/libs)
set(CNVAD_OUTPUT_LOCATION ${LIBS_OUTPUT_DIR})
file(MAKE_DIRECTORY ${LIBS_OUTPUT_DIR})

# 复制共享库到 libs 目录
add_custom_command(TARGET cnvad POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CNVAD_OUTPUT_LOCATION} ${LIBS_OUTPUT_DIR})

# 获取复制后的库文件的完整路径
# 注意: CMAKE_SHARED_LIBRARY_PREFIX (lib), TARGET_FILE_BASE_NAME (cnvad), CMAKE_SHARED_LIBRARY_SUFFIX (.so/.dylib)
set(COPIED_LIB_PATH "${LIBS_OUTPUT_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}cnvad${CMAKE_SHARED_LIBRARY_SUFFIX}")

# Strip 命令 (平台特定)
if(CMAKE_BUILD_TYPE MATCHES "Release") # 通常只在 Release 模式下 strip
    if(PLATFORM_LINUX OR PLATFORM_ANDROID)
        # 尝试查找 strip 工具 (Android 可能需要 NDK 的 strip)
        if(ANDROID AND DEFINED ANDROID_TOOLCHAIN_PREFIX)
            set(STRIP_COMMAND "${ANDROID_TOOLCHAIN_PREFIX}strip")
        else()
            find_program(STRIP_COMMAND strip)
        endif()
        if(STRIP_COMMAND)
            add_custom_command(TARGET cnvad POST_BUILD
                COMMAND ${STRIP_COMMAND} "${COPIED_LIB_PATH}"
                COMMENT "Stripping Linux/Android shared library: ${COPIED_LIB_PATH}"
                VERBATIM # 确保命令按原样执行
            )
        else()
            message(WARNING "strip command not found. Skipping stripping.")
        endif()

    elseif(PLATFORM_MACOS)
        find_program(STRIP_COMMAND strip) # macOS 自带 strip
        if(STRIP_COMMAND)
            add_custom_command(TARGET cnvad POST_BUILD
                COMMAND ${STRIP_COMMAND} -x "${COPIED_LIB_PATH}" # macOS strip 使用 -x 移除调试符号
                COMMENT "Stripping macOS shared library: ${COPIED_LIB_PATH}"
                VERBATIM
            )
        else()
            message(WARNING "strip command not found. Skipping stripping.")
        endif()
    endif()
endif()

# --- (推荐) 使用 install() 命令代替 custom commands ---
# 如果你想让构建更规范，特别是在安装到系统或打包时，
# 应该使用 install() 命令而不是 add_custom_command 来复制和处理库。
# 例如:
# install(TARGETS cnvad cnvad_static
#     LIBRARY DESTINATION lib  # 安装到 <prefix>/lib
#     ARCHIVE DESTINATION lib  # 静态库也到 <prefix>/lib
#     RUNTIME DESTINATION bin  # 如果有可执行文件
# )
# install(FILES ${PROJECT_SOURCE_DIR}/src/cnvad.h DESTINATION include) # 安装头文件
#
# 结合 CPack 可以轻松创建安装包 (TGZ, DEB, RPM, DMG 等)
# include(CPack)
