cmake_minimum_required(VERSION 3.18)
project(patchcore_infer LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ONNX Runtime
find_package(OnnxRuntime QUIET)
if(NOT OnnxRuntime_FOUND)
    # 手动指定路径（根据实际安装位置修改）
    set(ONNXRUNTIME_ROOT "" CACHE PATH "ONNX Runtime install path")
    if(ONNXRUNTIME_ROOT)
        include_directories(${ONNXRUNTIME_ROOT}/include)
        link_directories(${ONNXRUNTIME_ROOT}/lib)
    endif()
endif()

# OpenCV
find_package(OpenCV REQUIRED)

# CUDA
find_package(CUDA REQUIRED)

add_executable(patchcore_infer
    main.cpp
    patchcore_engine.cpp
)

target_include_directories(patchcore_infer PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${CUDA_INCLUDE_DIRS}
)

target_link_libraries(patchcore_infer PRIVATE
    ${OpenCV_LIBS}
    onnxruntime
    ${CUDA_LIBRARIES}
)
