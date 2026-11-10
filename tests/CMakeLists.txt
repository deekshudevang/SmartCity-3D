# Automated CTest Test Suite
enable_testing()

# Helper macro for test binaries
macro(add_smartcity_test test_name test_source)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE SmartCityCore)
    target_include_directories(${test_name} PRIVATE 
        "${CMAKE_CURRENT_SOURCE_DIR}/../src"
        "${CMAKE_CURRENT_SOURCE_DIR}/../external/glad/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../external/glm"
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endmacro()

add_smartcity_test(test_math test_math.cpp)
add_smartcity_test(test_time test_time.cpp)
