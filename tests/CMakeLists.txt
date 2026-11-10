# Automated CTest Test Suite
enable_testing()

macro(add_smartcity_unit_test test_name test_source)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE SmartCityCore)
    target_include_directories(${test_name} PRIVATE 
        "${CMAKE_CURRENT_SOURCE_DIR}/../src"
        "${CMAKE_CURRENT_SOURCE_DIR}/../include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../external/glad/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../external/glm"
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endmacro()

add_smartcity_unit_test(test_math unit/test_math.cpp)
add_smartcity_unit_test(test_time unit/test_time.cpp)
add_smartcity_unit_test(test_camera unit/test_camera.cpp)
add_smartcity_unit_test(test_intersection unit/test_intersection.cpp)
add_smartcity_unit_test(test_lane_graph unit/test_lane_graph.cpp)
add_smartcity_unit_test(test_idm unit/test_idm.cpp)
add_smartcity_unit_test(test_collision_avoidance unit/test_collision_avoidance.cpp)
add_smartcity_unit_test(test_frustum unit/test_frustum.cpp)
add_smartcity_unit_test(test_transform unit/test_transform.cpp)
add_smartcity_unit_test(test_camera_matrices unit/test_camera_matrices.cpp)
add_smartcity_unit_test(test_resource_manager unit/test_resource_manager.cpp)
