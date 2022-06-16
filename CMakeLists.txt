cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 20)
project(vulkanapp)

macro(shader)
    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/shaders_bin/${ARGV0}.spv
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${ARGV0} ${CMAKE_CURRENT_SOURCE_DIR}/shaders/common.glsl
            COMMAND /usr/bin/glslc
            ARGS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${ARGV0} -o ${CMAKE_CURRENT_SOURCE_DIR}/shaders_bin/${ARGV0}.spv -O --target-env=vulkan1.2
            COMMENT building shaders
            VERBATIM)
    SET(shader_src ${shader_src} ${CMAKE_CURRENT_SOURCE_DIR}/shaders_bin/${ARGV0}.spv)
endmacro()

shader("triangle.vert")
shader("triangle.frag")

shader("raygen.rgen")
shader("miss.rmiss")
shader("hit.rchit")

file(GLOB_RECURSE src CONFIGURE_DEPENDS "src/*.cpp")
add_executable(vulkanapp ${src} ${shader_src})

target_include_directories(vulkanapp PRIVATE include)

#target_precompile_headers(vulkanapp PRIVATE "include/precomp.h")

add_subdirectory(${CMAKE_SOURCE_DIR}/external/glfw)
link_directories(${CMAKE_SOURCE_DIR}/external/glfw)
include_directories(vulkanapp PUBLIC ${CMAKE_SOURCE_DIR}/external/glfw/include/)
target_link_libraries(vulkanapp glfw)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/glm)
link_directories(${CMAKE_SOURCE_DIR}/external/glm)
include_directories(vulkanpp PUBLIC ${CMAKE_SOURCE_DIR}/external/glm/glm/)
target_link_libraries(vulkanapp glm)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/spdlog)
link_directories(${CMAKE_SOURCE_DIR}/external/spdlog)
include_directories(vulkanapp PRIVATE ${CMAKE_SOURCE_DIR}/external/spdlog/include/)
target_link_libraries(vulkanapp spdlog)

find_package(Vulkan REQUIRED)
target_link_libraries(vulkanapp Vulkan::Vulkan)
