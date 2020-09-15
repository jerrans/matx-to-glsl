cmake_minimum_required(VERSION 3.14)

set(MODULE_NAME shaderGen)
project(${MODULE_NAME} CXX)

set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_CURRENT_SOURCE_DIR}/build/MaterialX/submodules/json/nlohmann_jsonConfig.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/build/MaterialX/installed/cmake/MaterialXConfig.cmake)
find_package(MaterialX REQUIRED COMPONENTS shadergen)

include_directories(${MATERIALX_INCLUDE_DIRS})

set(MODULE_SOURCES
	main.cpp
)

add_executable(${MODULE_NAME}
        ${MODULE_SOURCES}
)

target_link_libraries(${MODULE_NAME}
    ${MATERIALX_CORE_LIBS}
    ${MATERIALX_GENERATOR_LIBS}
)
