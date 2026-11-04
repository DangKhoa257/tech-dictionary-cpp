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

# Add dictionary_core.cpp with clearDictionary before enabling node-producing
# operations. Add each remaining dictionary/UI source only after its declared
# functions are implemented, their dependencies link, and controller wiring is
# reviewed. Keeping placeholders out avoids unresolved symbols and fake behavior.

include(CTest)
if(BUILD_TESTING)
    add_executable(AppControllerTests
        tests/app_controller_test.cpp
        src/app_controller.cpp
        include/app_controller.h
        ui/main_window.cpp
        ui/main_window.h
        resources/resources.qrc
    )
    target_include_directories(AppControllerTests PRIVATE include ui)
    target_link_libraries(AppControllerTests PRIVATE Qt6::Widgets)
    if(MSVC)
        target_compile_options(AppControllerTests PRIVATE /utf-8)
    endif()
    add_test(NAME AppControllerBoundary COMMAND AppControllerTests)
    set_tests_properties(AppControllerBoundary PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
