cmake_minimum_required(VERSION 3.20)

project(husk)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find LLVM
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Main compiler executable
add_executable(husk src/main.cpp)

# Link against LLVM libraries (including native target support)
llvm_map_components_to_libnames(llvm_libs 
  support 
  core 
  irreader
  native
  aarch64codegen  # For Apple Silicon
  x86codegen      # For Intel
)
target_link_libraries(husk ${llvm_libs})

# Testing setup
enable_testing()

# Test executable using amalgamated Catch2
add_executable(tests 
  tests/compiler_tests.cpp
  external/catch_amalgamated.cpp
)
target_include_directories(tests PRIVATE external)
target_link_libraries(tests PRIVATE ${llvm_libs})

# Register with CTest
include(CTest)
add_test(NAME CompilerTests COMMAND tests)
