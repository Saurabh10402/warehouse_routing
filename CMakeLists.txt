cmake_minimum_required(VERSION 3.22)

project(
    Private5GEnergyAwareRouting
    VERSION 0.1.0
    DESCRIPTION "Energy-Aware Low-Latency Routing framework for private 5G robotic networks"
    LANGUAGES CXX
)

include(GNUInstallDirs)

option(EALR_ENABLE_NS3 "Enable ns-3 and 5G-LENA integration when the packages are available" ON)
option(EALR_BUILD_TESTS "Build EALR tests" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(EalrDependencies)

set(EALR_BOOST_SOURCE "" CACHE PATH "Optional extracted official Boost source tree (offline portable build)")
if(EALR_BOOST_SOURCE)
    if(NOT EXISTS "${EALR_BOOST_SOURCE}/boost/json/src.hpp")
        message(FATAL_ERROR "EALR_BOOST_SOURCE must contain boost/json/src.hpp")
    endif()
    file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/boost_json_source.cpp" CONTENT "#include <boost/json/src.hpp>\n")
    add_library(ealr_boost_json STATIC "${CMAKE_CURRENT_BINARY_DIR}/boost_json_source.cpp")
    target_include_directories(ealr_boost_json SYSTEM PUBLIC "${EALR_BOOST_SOURCE}")
    target_compile_features(ealr_boost_json PUBLIC cxx_std_17)
    add_library(Boost::json ALIAS ealr_boost_json)
else()
    find_package(boost_json CONFIG REQUIRED)
endif()

add_library(ealr_project_options INTERFACE)
target_compile_features(ealr_project_options INTERFACE cxx_std_17)
ealr_enable_warnings(ealr_project_options)

add_library(ealr_core
    STATIC
        src/framework.cpp
        src/config/configuration.cpp
        src/config/warehouse_configuration.cpp
        src/config/fabric_configuration.cpp
        src/simulation/fabric_simulator.cpp
        src/metrics/fabric_metrics.cpp
        src/network/device_policy.cpp
        src/safety/temperature_safety_controller.cpp
        src/metrics/metrics_collector.cpp
        src/robot/robot.cpp
        src/routing/routing_types.cpp
        src/routing/dijkstra_routing.cpp
        src/routing/greedy_routing.cpp
        src/routing/ealr_routing.cpp
)
add_library(EALR::core ALIAS ealr_core)
target_link_libraries(ealr_core PUBLIC Boost::json ealr_project_options)
target_include_directories(ealr_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_definitions(ealr_core PRIVATE EALR_PROJECT_VERSION="${PROJECT_VERSION}")
add_executable(ealr-fabric src/app/ealr_fabric_main.cpp)
target_link_libraries(ealr-fabric PRIVATE EALR::core)

if(EALR_ENABLE_NS3)
    ealr_find_ns3()
endif()

if(EALR_ENABLE_NS3 AND EALR_NS3_AVAILABLE)
    add_library(ealr_ns3
        STATIC
            src/simulation/ns3_private_5g_scenario.cpp
            src/metrics/ns3_flow_monitor_collector.cpp
            src/visualization/netanim_visualization.cpp
    )
    add_library(EALR::ns3 ALIAS ealr_ns3)
    target_link_libraries(ealr_ns3
        PUBLIC EALR::core
        PUBLIC ns3::core ns3::network ns3::mobility ns3::netanim ns3::flow-monitor
    )
    target_include_directories(ealr_ns3 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
    # Match the external package's public-header requirements without changing the core.
    set(_ealr_ns3_standard 20)
    if(EALR_NS3_VERSION VERSION_GREATER_EQUAL "3.46.1")
        set(_ealr_ns3_standard 23)
    endif()
    set(EALR_NS3_CXX_STANDARD "${_ealr_ns3_standard}" CACHE STRING "C++ standard required by the installed ns-3 headers")
    target_compile_features(ealr_ns3 PUBLIC cxx_std_${EALR_NS3_CXX_STANDARD})

    add_executable(ealr-ns3-scenario src/app/ealr_ns3_main.cpp)
    target_link_libraries(ealr-ns3-scenario PRIVATE EALR::ns3)

    if(EALR_5G_LENA_AVAILABLE)
        target_sources(ealr_ns3 PRIVATE src/simulation/nr_access_network.cpp)
        target_link_libraries(ealr_ns3 PRIVATE ns3::nr)
        target_compile_definitions(ealr_ns3 PRIVATE EALR_HAS_5G_LENA=1)
    endif()
endif()

add_custom_target(ealr)

if(EALR_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()
endif()

install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

message(STATUS "EALR ${PROJECT_VERSION}: repository bootstrap configured")
message(STATUS "ns-3 support: ${EALR_NS3_STATUS}")
message(STATUS "5G-LENA NR support: ${EALR_5G_LENA_STATUS}")
