cmake_minimum_required(VERSION 3.28)

project(imgui_docking CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the output directory structure
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output/lib/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output/lib/$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output/bin/$<CONFIG>")

file(GLOB SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.h
)
file(GLOB CXX_MODULES
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.ixx
)

# Automatically organize files in Visual Studio
foreach(file ${SOURCES})
    # Get relative path and convert to Windows style
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" relative_path ${file})
    string(REPLACE "/" "\\" relative_path ${relative_path})
    get_filename_component(directory ${relative_path} DIRECTORY)
    
    # Create a source group for each directory
    source_group("${directory}" FILES ${file})
endforeach()
foreach(file ${CXX_MODULES})
    # Get relative path and convert to Windows style
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" relative_path ${file})
    string(REPLACE "/" "\\" relative_path ${relative_path})
    get_filename_component(directory ${relative_path} DIRECTORY)
    
    # Create a source group for each directory
    source_group("${directory}" FILES ${file})
endforeach()



enable_testing()

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(CTEST_BINARY_DIRECTORY "${CMAKE_SOURCE_DIR}/Testing")

add_library(imgui_docking STATIC)
target_sources(imgui_docking
    PRIVATE ${SOURCES}
    #PRIVATE FILE_SET CXX_MODULES FILES ${CXX_MODULES}
)
target_compile_definitions(imgui_docking PRIVATE $<$<CONFIG:Debug>:DEBUG>)
target_compile_definitions(imgui_docking PRIVATE $<$<CONFIG:Release>:RELEASE>)

target_include_directories(imgui_docking PUBLIC

    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)

target_link_libraries(imgui_docking PUBLIC )

target_compile_definitions(imgui_docking PRIVATE )

# Enable multi-processor compilation (if supported by the compiler)
if(MSVC)
    add_compile_options(/MP)
else()
    add_compile_options(-j$(nproc))
endif()
