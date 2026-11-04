cmake_minimum_required(VERSION 3.16)

project(TechDictionary VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(TechDictionary WIN32
    src/main.cpp
    src/app_controller.cpp
    include/app_controller.h
    ui/main_window.cpp
    ui/main_window.h
    resources/resources.qrc
)

target_include_directories(TechDictionary PRIVATE include ui)
target_link_libraries(TechDictionary PRIVATE Qt6::Widgets)
if(MSVC)
    target_compile_options(TechDictionary PRIVATE /utf-8)
endif()

# Dictionary operation and component modules join this target when implemented.
# Keeping unfinished files out avoids unresolved symbols and misleading stubs.
