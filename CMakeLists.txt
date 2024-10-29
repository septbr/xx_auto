cmake_minimum_required(VERSION 3.30)

project(xx_auto VERSION 0.1.1)

set(CMAKE_CXX_STANDARD 17)

add_library(libxx_auto SHARED)
set_target_properties(libxx_auto PROPERTIES OUTPUT_NAME_DEBUG "xx_autod" OUTPUT_NAME_RELEASE "xx_auto")
target_compile_options(libxx_auto PRIVATE /utf-8)
target_compile_definitions(libxx_auto PRIVATE UNICODE _UNICODE XX_AUTO_SHARED)
target_include_directories(libxx_auto PRIVATE "./third_party/onnxruntime/include")
target_link_directories(libxx_auto PRIVATE "./third_party/onnxruntime/lib")
target_link_libraries(libxx_auto PRIVATE "onnxruntime" "onnxruntime_providers_shared")
target_sources(libxx_auto PRIVATE
    "./xx_auto/xx_auto.cpp"
    "./xx_auto/xx_auto_ocr.cpp"
    "./xx_auto/xx_auto_capturer.cpp"
    "./xx_auto/xx_auto_app.cpp"
)

add_executable(xx_auto WIN32)
set_target_properties(xx_auto PROPERTIES OUTPUT_NAME_DEBUG "xx_autod" OUTPUT_NAME_RELEASE "xx_auto")
target_compile_options(xx_auto PRIVATE /utf-8)
target_compile_definitions(xx_auto PRIVATE UNICODE _UNICODE)
target_link_libraries(xx_auto PRIVATE "libxx_auto")
target_sources(xx_auto PRIVATE
    "./xx_auto/xx_auto.rc"
    "./xx_auto/xx_auto_exe.cpp"
)

add_library(xx_auto_entry SHARED)
set_target_properties(xx_auto_entry PROPERTIES OUTPUT_NAME_DEBUG "xx_auto_entryd" OUTPUT_NAME_RELEASE "xx_auto_entry")
target_compile_options(xx_auto_entry PRIVATE /utf-8)
target_compile_definitions(xx_auto_entry PRIVATE UNICODE _UNICODE)
target_link_libraries(xx_auto_entry PRIVATE "libxx_auto")
target_sources(xx_auto_entry PRIVATE
    "./xx_auto/xx_auto_entry.cpp"
)

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install/xx_auto")
install(TARGETS libxx_auto DESTINATION "bin")
install(TARGETS xx_auto_entry DESTINATION "bin")
install(TARGETS xx_auto DESTINATION "bin")
install(FILES "./xx_auto/xx_auto.hpp" DESTINATION "include")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/onnxruntime/lib/onnxruntime.dll" DESTINATION "bin")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/onnxruntime/lib/onnxruntime_providers_shared.dll" DESTINATION "bin")
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/models/PPOCRv3" DESTINATION "bin/models")