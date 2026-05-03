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

add_subdirectory(${CMAKE_SOURCE_DIR}/modules/debug-lib_0.0.4)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/vulkan-lib_0.0.2)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/utils-lib_0.0.1)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/imgui_docking_v1.92.4-docking)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/entt_v3.15.0)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/spdlog_v1.16.0)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/syris-vulkan_0.0.1-vulkan)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/named_type_v1.1.0)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/googletests_v1.17.0)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/vulkan_v1.4.332)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/glm_1.0.2)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/glfw_3.4)
add_subdirectory(${CMAKE_SOURCE_DIR}/modules/nlohmann_json_v3.12.0)

add_executable(lumir)
target_sources(lumir
    PRIVATE ${SOURCES}
)
target_compile_definitions(lumir PRIVATE $<$<CONFIG:Debug>:DEBUG>)
target_compile_definitions(lumir PRIVATE $<$<CONFIG:Release>:RELEASE>)

target_include_directories(lumir PUBLIC
	${CMAKE_SOURCE_DIR}/modules/syris-vulkan_0.0.1-vulkan
	${CMAKE_SOURCE_DIR}/modules/named_type_v1.1.0
	${CMAKE_SOURCE_DIR}/modules/fast_noise
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(lumir PUBLIC syris NamedType)

target_compile_definitions(lumir PRIVATE )

add_compile_definitions(PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

# Enable multi-processor compilation (if supported by the compiler)
if(MSVC)
    add_compile_options(/MP)
else()
    add_compile_options(-j$(nproc))
endif()
