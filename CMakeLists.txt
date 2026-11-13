cmake_minimum_required(VERSION 3.16)

project(TechDictionary VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Qt6 QUIET COMPONENTS Widgets)
if(NOT Qt6_FOUND)
    message(FATAL_ERROR "Qt 6 Widgets is required for the TechDictionary desktop application.")
endif()
if(Qt6_FOUND)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    add_executable(TechDictionary WIN32
        src/main.cpp
        src/app_controller.cpp
        include/app_controller.h
        src/dictionary_core.cpp
        src/dictionary_search.cpp
        src/dictionary_edit.cpp
        src/dictionary_history.cpp
        src/dictionary_display.cpp
        src/dictionary_storage.cpp
        ui/main_window.cpp
        ui/main_window.h
        ui/search_widget.cpp
        ui/search_widget.h
        ui/word_dialog.cpp
        ui/word_dialog.h
        ui/delete_confirm.cpp
        ui/delete_confirm.h
        resources/resources.qrc
    )

    target_include_directories(TechDictionary PRIVATE include ui)
    target_link_libraries(TechDictionary PRIVATE Qt6::Widgets)
    add_custom_command(TARGET TechDictionary POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:TechDictionary>/data"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/data/dictionary.txt"
            "$<TARGET_FILE_DIR:TechDictionary>/data/dictionary.txt")
    if(MSVC)
        target_compile_options(TechDictionary PRIVATE /utf-8)
    endif()
endif()

include(CTest)
if(BUILD_TESTING)
    if(Qt6_FOUND)
        add_executable(AppControllerTests
            tests/app_controller_test.cpp
            src/app_controller.cpp
            include/app_controller.h
            ui/main_window.cpp
            ui/main_window.h
            src/dictionary_core.cpp
            src/dictionary_search.cpp
            src/dictionary_edit.cpp
            src/dictionary_history.cpp
            src/dictionary_display.cpp
            src/dictionary_storage.cpp
            ui/search_widget.cpp
            ui/search_widget.h
            ui/word_dialog.cpp
            ui/word_dialog.h
            ui/delete_confirm.cpp
            ui/delete_confirm.h
            resources/resources.qrc
        )
        target_include_directories(AppControllerTests PRIVATE include ui)
        target_link_libraries(AppControllerTests PRIVATE Qt6::Widgets)
        add_custom_command(TARGET AppControllerTests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:AppControllerTests>/data"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/data/dictionary.txt"
                "$<TARGET_FILE_DIR:AppControllerTests>/data/dictionary.txt")
        if(MSVC)
            target_compile_options(AppControllerTests PRIVATE /utf-8)
        endif()
        add_test(NAME AppControllerBoundary COMMAND AppControllerTests)
        set_tests_properties(AppControllerBoundary PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            TIMEOUT 30)

        add_executable(WordDialogTests
            tests/word_dialog_test.cpp
            ui/word_dialog.cpp
            ui/word_dialog.h
        )
        target_include_directories(WordDialogTests PRIVATE ui)
        target_link_libraries(WordDialogTests PRIVATE Qt6::Widgets)
        if(MSVC)
            target_compile_options(WordDialogTests PRIVATE /utf-8)
        endif()
        add_test(NAME WordDialogBoundary COMMAND WordDialogTests)
        set_tests_properties(WordDialogBoundary PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

        add_executable(MainWindowTests
            tests/main_window_test.cpp
            ui/main_window.cpp
            ui/main_window.h
            ui/search_widget.cpp
            ui/search_widget.h
            resources/resources.qrc
        )
        target_include_directories(MainWindowTests PRIVATE ui)
        target_link_libraries(MainWindowTests PRIVATE Qt6::Widgets)
        if(MSVC)
            target_compile_options(MainWindowTests PRIVATE /utf-8)
        endif()
        add_test(NAME MainWindowBoundary COMMAND MainWindowTests)
        set_tests_properties(MainWindowBoundary PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endif()

    add_executable(DictionarySearchTests
        tests/test_search.cpp
        src/dictionary_core.cpp
        src/dictionary_search.cpp
    )
    target_include_directories(DictionarySearchTests PRIVATE include)
    add_test(NAME DictionarySearch COMMAND DictionarySearchTests)

    add_executable(DictionaryTV6Tests
        tests/dictionary_tv6_test.cpp
        src/dictionary_core.cpp
        src/dictionary_search.cpp
        src/dictionary_display.cpp
        src/dictionary_storage.cpp
    )
    target_include_directories(DictionaryTV6Tests PRIVATE include)
    if(MSVC)
        target_compile_options(DictionaryTV6Tests PRIVATE /utf-8)
    endif()
    add_test(NAME DictionaryTV6 COMMAND DictionaryTV6Tests)

    add_executable(DictionaryEditTests
        tests/dictionary_edit_test.cpp
        src/dictionary_core.cpp
        src/dictionary_search.cpp
        src/dictionary_edit.cpp
        src/dictionary_history.cpp
        src/dictionary_display.cpp
        src/dictionary_storage.cpp
    )
    target_include_directories(DictionaryEditTests PRIVATE include)
    if(MSVC)
        target_compile_options(DictionaryEditTests PRIVATE /utf-8)
    endif()
    add_test(NAME DictionaryEdit COMMAND DictionaryEditTests)
endif()
