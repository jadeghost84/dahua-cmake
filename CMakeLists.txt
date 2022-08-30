cmake_minimum_required(VERSION 2.6)
project(pro)
add_definitions(-std=c++11)

option(CUDA_USE_STATIC_CUDA_RUNTIME ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/workspace)

# 如果你是不同显卡，请设置为显卡对应的号码参考这里：https://developer.nvidia.com/zh-cn/cuda-gpus#compute
set(CUDA_GEN_CODE "-gencode=arch=compute_86,code=sm_86")

# 如果你的opencv找不到，可以自己指定目录
set(OpenCV_DIR   "/usr/local/include/opencv4")

set(CUDA_DIR     "/usr/local/cuda-11.7")
set(CUDNN_DIR    "/usr/soft/cudnn-linux-x86_64-8.4.0.27_cuda11.6-archive")
set(TENSORRT_DIR "/usr/soft/TensorRT-8.4.1.5")
set(PROTOBUF_DIR "/usr/local")
#链接大华sdk库
include_directories(${PROJECT_SOURCE_DIR}/3rdparts/dhSDK/include)
link_directories(${PROJECT_SOURCE_DIR}/3rdparts/dhSDK/lib/)


#链接ffmpeg库
include_directories(/usr/soft/FFmpeg-master/target/include)
link_directories(/usr/soft/FFmpeg-master/target/lib)

find_package(CUDA REQUIRED)
find_package(OpenCV)
include_directories(
    ${PROTOBUF_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
    ${CUDA_DIR}/include
    ${TENSORRT_DIR}/include
    ${CUDNN_DIR}/include
)

# 切记，protobuf的lib目录一定要比tensorRT目录前面，因为tensorRTlib下带有protobuf的so文件
# 这可能带来错误
link_directories(
    ${PROTOBUF_DIR}/lib
    ${TENSORRT_DIR}/lib
    ${CUDA_DIR}/lib64
    ${CUDNN_DIR}/lib
)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -O0 -Wfatal-errors -pthread -w -g")
set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -std=c++11 -O0 -Xcompiler -fPIC -g -w ${CUDA_GEN_CODE}")
file(GLOB_RECURSE cpp_srcs ${PROJECT_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE cuda_srcs ${PROJECT_SOURCE_DIR}/src/*.cu)
cuda_add_library(cucodes SHARED ${cuda_srcs})

add_executable(${PROJECT_NAME} ${cpp_srcs})

# 如果提示插件找不到，请使用dlopen(xxx.so, NOW)的方式手动加载可以解决插件找不到问题
target_link_libraries(cucodes nvinfer nvonnxparser)
target_link_libraries(cucodes cuda cublas cudart cudnn)
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})
target_link_libraries(${PROJECT_NAME} cucodes)
target_link_libraries(${PROJECT_NAME}
  "avnetsdk";"dhconfigsdk";"dhnetsdk";"dhplay";"HWDec";"IvsDrawer";"StreamConvertor"
)
target_link_libraries(${PROJECT_NAME}
avfilter avformat avcodec avutil swscale
)
add_custom_target(
    run
    DEPENDS pro
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/workspace
    COMMAND ./pro
)