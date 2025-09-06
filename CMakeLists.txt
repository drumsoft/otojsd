cmake_minimum_required(VERSION 3.16)
project(otojsd LANGUAGES CXX)

# V8 build root directory: should be set by -DV8_ROOT_DIR=/path/to/v8
set(V8_ROOT_DIR "$ENV{HOME}/v8" CACHE PATH "Path to V8 build root")

# name of the application
set(APP otojsd)

# source files
file(GLOB SOURCES "src/*.cpp")

# default build type is Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -fno-rtti)

add_compile_definitions(
  V8_COMPRESS_POINTERS
  V8_ENABLE_SANDBOX
)

include_directories(${V8_ROOT_DIR}/include)

link_directories(${V8_ROOT_DIR}/out.gn/arm64.release.sample/obj)

add_executable(${APP} ${SOURCES})

target_link_libraries(${APP}
  v8_monolith
  pthread
  dl
  "-framework CoreFoundation"
  "-framework CoreAudio"
  "-framework AudioUnit"
)

# change compile options for Debug build
set_target_properties(${APP} PROPERTIES
  COMPILE_OPTIONS "$<$<CONFIG:Debug>:-g>"
)

# "run" target
add_custom_target(run
  COMMAND "${CMAKE_BINARY_DIR}/${APP}"
  DEPENDS ${APP}
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
