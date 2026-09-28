cmake_minimum_required(VERSION 3.22)

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

set(FILE_SYSTEM_MANAGER_SOURCES
        src/Diretoria.cpp
        src/Ficheiro.cpp
        src/Item.cpp
        src/Logger.cpp
        src/SistemaFicheiros.cpp
        src/Utils.cpp
        src/XML.cpp
)

add_library(file_system_manager_core STATIC ${FILE_SYSTEM_MANAGER_SOURCES})

find_package(Threads REQUIRED)

target_include_directories(
    file_system_manager_core
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(file_system_manager_core PUBLIC Threads::Threads)

function(file_system_manager_enable_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive- /utf-8)

        if(FILE_SYSTEM_MANAGER_WARNINGS_AS_ERRORS)
            target_compile_options(${target_name} PRIVATE /WX)
        endif()
    else()
        target_compile_options(
            ${target_name}
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wshadow
        )

        if(FILE_SYSTEM_MANAGER_WARNINGS_AS_ERRORS)
            target_compile_options(${target_name} PRIVATE -Werror)
        endif()
    endif()
endfunction()

file_system_manager_enable_warnings(file_system_manager_core)

add_executable(file_system_manager main.cpp src/Menu.cpp)
target_link_libraries(file_system_manager PRIVATE file_system_manager_core)
file_system_manager_enable_warnings(file_system_manager)

set_target_properties(
    file_system_manager
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin"
)

install(TARGETS file_system_manager RUNTIME DESTINATION bin)

include(CTest)

if(BUILD_TESTING)
    add_executable(
        file_system_manager_characterization_tests
        tests/characterization_tests.cpp
    )
    target_link_libraries(
        file_system_manager_characterization_tests
        PRIVATE
            file_system_manager_core
    )
    file_system_manager_enable_warnings(
        file_system_manager_characterization_tests
    )

    add_test(
        NAME file_system_manager_characterization
        COMMAND file_system_manager_characterization_tests
    )

    if(WIN32)
        cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH compiler_runtime_dir)
        set_tests_properties(
            file_system_manager_characterization
            PROPERTIES
                ENVIRONMENT_MODIFICATION
                    "PATH=path_list_prepend:${compiler_runtime_dir}"
        )
    endif()
endif()
