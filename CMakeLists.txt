cmake_minimum_required(VERSION 3.30)

project(xx_auto VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)

add_library(xx_auto)
target_compile_options(xx_auto PRIVATE /utf-8)
target_compile_definitions(xx_auto PRIVATE UNICODE _UNICODE)
target_include_directories(xx_auto PRIVATE "./third_party/onnxruntime/include")
file(GLOB_RECURSE xx_auto_src "./xx_auto/xx_auto.rc" "./xx_auto/*.cpp")
target_sources(xx_auto PRIVATE ${xx_auto_src})

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install/xx_auto")
install(TARGETS xx_auto DESTINATION lib)  
install(FILES "./xx_auto/xx_auto.hpp" DESTINATION "include")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/onnxruntime/lib/onnxruntime.dll" DESTINATION "bin")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/onnxruntime/lib/onnxruntime_providers_shared.dll" DESTINATION "bin")
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/models/PPOCRv3" DESTINATION "bin/models")