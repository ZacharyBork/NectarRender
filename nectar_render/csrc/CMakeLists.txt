cmake_minimum_required(VERSION 3.18)
find_program(CMAKE_CUDA_COMPILER nvcc
    HINTS
        $ENV{CUDA_HOME}
        $ENV{CUDA_PATH}
        /usr/local/cuda/bin
)
project(nectar_render LANGUAGES CXX CUDA)

# C++ and CUDA standards
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find dependencies
find_package(pybind11 REQUIRED)
find_package(CUDAToolkit REQUIRED)

# Gather sources
file(GLOB_RECURSE SOURCES "*.cpp" "*.cu")

# Build extension module
pybind11_add_module(_pathtracer
    bindings/bindings.cpp
    ${SOURCES}
)

# Link CUDA runtime
target_link_libraries(_pathtracer PRIVATE CUDA::cudart)

# cuBLAS
find_package(CUDA REQUIRED)
target_link_libraries(_pathtracer PRIVATE cublas)

# Include csrc/ as root for includes
target_include_directories(_pathtracer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# CLI override for CUDA architecture:
# pip install . --config-settings=cmake.args="-DCUDA_ARCHITECTURES=70;75;80;86;89;90"
set(CUDA_ARCHITECTURES "75" CACHE STRING "CUDA architectures to target")
set_target_properties(_pathtracer PROPERTIES CUDA_ARCHITECTURES ${CUDA_ARCHITECTURES})

# Install package
install(TARGETS _pathtracer DESTINATION .)

# Generate pybind11 stubs
# find_program(STUBGEN_EXECUTABLE pybind11-stubgen)
# add_custom_command(
#     TARGET _pathtracer
#     POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E env
#             "PYTHONPATH=$<TARGET_FILE_DIR:_pathtracer>"
#             ${STUBGEN_EXECUTABLE} _pathtracer --output-dir ${CMAKE_SOURCE_DIR}/stubs
#     COMMENT "Generating pybind11 stubs for _pathtracer"
#     VERBATIM
# )
