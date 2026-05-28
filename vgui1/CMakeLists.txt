cmake_minimum_required(VERSION 3.5)
project(vgui1)
set(CMAKE_CXX_STANDARD 11)
file(GLOB VGUI_SOURCES "src/*.cpp")

# Build as shared library for Android (vgui_support.so)
# Build as static library otherwise
if(VGUI_BUILD_SHARED)
    add_library(vgui_support SHARED ${VGUI_SOURCES})
    target_include_directories(vgui_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    if(NOT MSVC)
        target_compile_options(vgui_support PRIVATE -fvisibility=default -Wno-unused-parameter -fPIC)
    endif()
    set_target_properties(vgui_support PROPERTIES
        OUTPUT_NAME "vgui_support"
        PREFIX "lib"
        SUFFIX ".so"
    )
else()
    add_library(vgui STATIC ${VGUI_SOURCES})
    target_include_directories(vgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    if(NOT MSVC)
        target_compile_options(vgui PRIVATE -fvisibility=default -Wno-unused-parameter)
    endif()
endif()
