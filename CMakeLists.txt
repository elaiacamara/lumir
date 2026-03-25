cmake_minimum_required(VERSION 3.28)

project(lumir CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the output directory structure
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output/lib/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output/lib/$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/output/bin/$<CONFIG>")

file(GLOB_RECURSE SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)
file(GLOB_RECURSE CXX_MODULES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.ixx
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


add_subdirectory(${CMAKE_SOURCE_DIR}/modules/glfw_3.4)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/syris-vulkan_0.0.1-vulkan)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/glm_1.0.2)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/googletests_v1.17.0)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/vulkan-lib_0.0.2)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/spdlog_v1.16.0)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/vulkan_v1.4.332)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/nlohmann_json_v3.12.0)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/entt_v3.15.0)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/imgui_docking_v1.92.4-docking)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/debug-lib_0.0.3)

# Check if Doxygen is available
find_package(Doxygen REQUIRED)

# If Doxygen is found, add the target to generate docs
if(DOXYGEN_FOUND)
    # Configure Doxygen with the path to your Doxyfile
    set(DOXYGEN_IN_PATH "${CMAKE_SOURCE_DIR}/Doxyfile")  # Replace with your path if different

    # Create a custom target to generate documentation
    add_custom_target(doc_lumir
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_IN_PATH}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)

    # Make the doc target depend on the build target
    add_dependencies(doc_lumir ${PROJECT_NAME})  # Replace ${PROJECT_NAME} with your target name if needed
endif()

enable_testing()

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(CTEST_BINARY_DIRECTORY "${CMAKE_SOURCE_DIR}/Testing")

add_executable(lumir)
target_sources(lumir
    PRIVATE ${SOURCES}
    PRIVATE FILE_SET CXX_MODULES FILES ${CXX_MODULES}
)
target_compile_definitions(lumir PRIVATE $<$<CONFIG:Debug>:DEBUG>)
target_compile_definitions(lumir PRIVATE $<$<CONFIG:Release>:RELEASE>)

target_include_directories(lumir PUBLIC
	${CMAKE_SOURCE_DIR}/modules/syris-vulkan_0.0.1-vulkan
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(lumir PUBLIC syris)

target_compile_definitions(lumir PRIVATE )

add_compile_definitions(PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
# Enable multi-processor compilation (if supported by the compiler)
if(MSVC)
    add_compile_options(/MP)
else()
    add_compile_options(-j$(nproc))
endif()
