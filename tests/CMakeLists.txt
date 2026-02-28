cmake_minimum_required(VERSION 3.15)
project(mcdk-tests LANGUAGES CXX)

add_executable(search_test search_test.cpp)

target_include_directories(search_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(search_test PRIVATE
    cppjieba
    nlohmann_json::nlohmann_json
)

target_compile_definitions(search_test PRIVATE
    MCDK_KNOWLEDGE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../knowledge"
    MCDK_DICTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../dicts"
)
