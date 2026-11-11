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

# TV4 modules are built independently. The controller wiring remains TV1's task.
add_library(TV4DictionaryEdit STATIC
    src/dictionary_edit.cpp
    src/dictionary_history.cpp
    include/dictionary_history.h
)
target_include_directories(TV4DictionaryEdit PUBLIC include)

add_library(TV4WordDialog STATIC
    ui/word_dialog.cpp
    ui/word_dialog.h
    ui/delete_confirm.cpp
    ui/delete_confirm.h
)
target_include_directories(TV4WordDialog PUBLIC include ui)
target_link_libraries(TV4WordDialog PUBLIC Qt6::Widgets)

if(MSVC)
    target_compile_options(TV4DictionaryEdit PRIVATE /utf-8)
    target_compile_options(TV4WordDialog PRIVATE /utf-8)
endif()

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
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )

    # Use the real TV2 hash/cleanup implementation; no production test doubles.
    add_executable(DictionaryEditTests
        tests/dictionary_edit_test.cpp
        src/dictionary_core.cpp
        src/dictionary_edit.cpp
        src/dictionary_history.cpp
    )
    target_include_directories(DictionaryEditTests PRIVATE include)
    add_test(NAME DictionaryEditUnitTests COMMAND DictionaryEditTests)
endif()
