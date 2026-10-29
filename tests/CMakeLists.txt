add_executable(ealr_framework_smoke framework_smoke.cpp)
target_link_libraries(ealr_framework_smoke PRIVATE EALR::core)

add_executable(ealr_configuration_tests configuration_tests.cpp)
target_link_libraries(ealr_configuration_tests PRIVATE EALR::core)
target_compile_definitions(ealr_configuration_tests PRIVATE EALR_SOURCE_DIRECTORY="${PROJECT_SOURCE_DIR}")

add_executable(ealr_warehouse_configuration_tests warehouse_configuration_tests.cpp)
target_link_libraries(ealr_warehouse_configuration_tests PRIVATE EALR::core)
target_compile_definitions(ealr_warehouse_configuration_tests PRIVATE EALR_SOURCE_DIRECTORY="${PROJECT_SOURCE_DIR}")

add_executable(ealr_temperature_safety_controller_tests temperature_safety_controller_tests.cpp)
target_link_libraries(ealr_temperature_safety_controller_tests PRIVATE EALR::core)

add_executable(ealr_robot_tests robot_tests.cpp)
target_link_libraries(ealr_robot_tests PRIVATE EALR::core)

add_executable(ealr_routing_interface_tests routing_interface_tests.cpp)
target_link_libraries(ealr_routing_interface_tests PRIVATE EALR::core)

add_executable(ealr_dijkstra_tests dijkstra_tests.cpp)
target_link_libraries(ealr_dijkstra_tests PRIVATE EALR::core)

add_executable(ealr_greedy_tests greedy_tests.cpp)
target_link_libraries(ealr_greedy_tests PRIVATE EALR::core)

add_executable(ealr_ealr_tests ealr_tests.cpp)
target_link_libraries(ealr_ealr_tests PRIVATE EALR::core)

add_executable(ealr_metrics_tests metrics_tests.cpp)
target_link_libraries(ealr_metrics_tests PRIVATE EALR::core)

add_custom_target(ealr_tests DEPENDS ealr_framework_smoke ealr_configuration_tests ealr_warehouse_configuration_tests ealr_temperature_safety_controller_tests ealr_robot_tests ealr_routing_interface_tests ealr_dijkstra_tests ealr_greedy_tests ealr_ealr_tests ealr_metrics_tests)
add_test(NAME ealr_framework_smoke COMMAND ealr_framework_smoke)
add_test(NAME ealr_configuration_tests COMMAND ealr_configuration_tests)
add_test(NAME ealr_warehouse_configuration_tests COMMAND ealr_warehouse_configuration_tests)
add_test(NAME ealr_temperature_safety_controller_tests COMMAND ealr_temperature_safety_controller_tests)
add_test(NAME ealr_robot_tests COMMAND ealr_robot_tests)
add_test(NAME ealr_routing_interface_tests COMMAND ealr_routing_interface_tests)
add_test(NAME ealr_dijkstra_tests COMMAND ealr_dijkstra_tests)
add_test(NAME ealr_greedy_tests COMMAND ealr_greedy_tests)
add_test(NAME ealr_ealr_tests COMMAND ealr_ealr_tests)
add_test(NAME ealr_metrics_tests COMMAND ealr_metrics_tests)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME ealr_plotting_tests COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/plotting_tests.py")
endif()

if(TARGET ealr_ns3)
    add_executable(ealr_ns3_scenario_tests ns3_scenario_tests.cpp)
    target_link_libraries(ealr_ns3_scenario_tests PRIVATE EALR::ns3)
    target_compile_definitions(ealr_ns3_scenario_tests PRIVATE EALR_SOURCE_DIRECTORY="${PROJECT_SOURCE_DIR}")
    add_dependencies(ealr_tests ealr_ns3_scenario_tests)
    add_test(NAME ealr_ns3_scenario_tests COMMAND ealr_ns3_scenario_tests)

    add_executable(ealr_flow_monitor_tests flow_monitor_tests.cpp)
    target_link_libraries(ealr_flow_monitor_tests
        PRIVATE
            EALR::ns3
            ns3::applications
            ns3::internet
            ns3::point-to-point
    )
    add_dependencies(ealr_tests ealr_flow_monitor_tests)
    add_test(NAME ealr_flow_monitor_tests COMMAND ealr_flow_monitor_tests)

    add_executable(ealr_netanim_tests netanim_tests.cpp)
    target_link_libraries(ealr_netanim_tests PRIVATE EALR::ns3)
    add_dependencies(ealr_tests ealr_netanim_tests)
    add_test(NAME ealr_netanim_tests COMMAND ealr_netanim_tests)
endif()
