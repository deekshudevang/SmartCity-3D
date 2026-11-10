# Automated CTest Test Suite
enable_testing()

macro(add_smartcity_unit_test test_name test_source)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE SmartCityCore)
    target_include_directories(${test_name} PRIVATE 
        "${CMAKE_CURRENT_SOURCE_DIR}/../src"
        "${CMAKE_CURRENT_SOURCE_DIR}/../include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../external"
        "${CMAKE_CURRENT_SOURCE_DIR}/../external/glad/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../external/glm"
    )
    if(MSVC)
        target_compile_options(${test_name} PRIVATE /UNDEBUG)
    else()
        target_compile_options(${test_name} PRIVATE -UNDEBUG)
    endif()
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
add_smartcity_unit_test(test_road_network unit/test_road_network.cpp)
add_smartcity_unit_test(test_obb unit/test_obb.cpp)
add_smartcity_unit_test(test_vehicle_sim unit/test_vehicle_sim.cpp)
add_smartcity_unit_test(test_traffic_signals unit/test_traffic_signals.cpp)
add_smartcity_unit_test(test_pedestrians unit/test_pedestrians.cpp)
add_smartcity_unit_test(test_environment_fx unit/test_environment_fx.cpp)
add_smartcity_unit_test(test_analytics unit/test_analytics.cpp)
add_smartcity_unit_test(test_cg_algos unit/test_cg_algos.cpp)
add_smartcity_unit_test(test_experiments unit/test_experiments.cpp)
add_smartcity_unit_test(test_vehicle_json unit/test_vehicle_json.cpp)
add_smartcity_unit_test(test_vehicle_importer unit/test_vehicle_importer.cpp)
add_smartcity_unit_test(test_vehicle_registry unit/test_vehicle_registry.cpp)
add_smartcity_unit_test(test_vehicle_assets unit/test_vehicle_assets.cpp)
add_smartcity_unit_test(test_vehicle_customization unit/test_vehicle_customization.cpp)
