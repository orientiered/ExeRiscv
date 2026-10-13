cmake_minimum_required(VERSION 3.26)
project(ExeRiscV
  LANGUAGES CXX
  VERSION 0.1
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# normal variables have higher priority than options
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# add_subdirectory(lib_deps)
# add_subdirectory(src)

set(EXEC_NAME interp)

option(ENABLE_ASAN "Enable Address Sanitizer" OFF)

option(INCLUDE_DEBUG_SYMBOLS "Include debug symbols" OFF)

if (INCLUDE_DEBUG_SYMBOLS)
  add_compile_options("-g2")
endif()

if(ENABLE_ASAN AND NOT WIN32)
  set(SAN_FLAGS "-fsanitize=address,undefined")
  add_compile_options(${SAN_FLAGS} -fno-omit-frame-pointer)
  add_link_options(${SAN_FLAGS})
endif()


file(GLOB_RECURSE PROJ_SRC src/*.cpp)
message("Project sources: ${PROJ_SRC}")

add_executable(${EXEC_NAME} ${PROJ_SRC})