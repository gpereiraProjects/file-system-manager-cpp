cmake_minimum_required(VERSION 3.20)

project(
    FileSystemManager
    VERSION 1.0.0
    DESCRIPTION "Object-oriented file system manager"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(FILE_SYSTEM_MANAGER_WARNINGS_AS_ERRORS
       "Treat compiler warnings as errors"
       OFF)

if(NOT WIN32)
    message(WARNING
        "The current source code uses the Windows console API. "
        "Cross-platform support will be added in a later refactoring."
    )
endif()

add_executable(file_system_manager)

target_sources(
    file_system_manager
    PRIVATE
        main.cpp
        src/Diretoria.cpp
        src/Ficheiro.cpp
        src/Item.cpp
        src/Logger.cpp
        src/Menu.cpp
        src/SistemaFicheiros.cpp
        src/Utils.cpp
        src/XML.cpp
)

target_include_directories(
    file_system_manager
    PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
)

if(MSVC)
    target_compile_options(file_system_manager PRIVATE /W4 /permissive- /utf-8)

    if(FILE_SYSTEM_MANAGER_WARNINGS_AS_ERRORS)
        target_compile_options(file_system_manager PRIVATE /WX)
    endif()
else()
    target_compile_options(
        file_system_manager
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
    )

    if(FILE_SYSTEM_MANAGER_WARNINGS_AS_ERRORS)
        target_compile_options(file_system_manager PRIVATE -Werror)
    endif()
endif()

set_target_properties(
    file_system_manager
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin"
)

install(TARGETS file_system_manager RUNTIME DESTINATION bin)
