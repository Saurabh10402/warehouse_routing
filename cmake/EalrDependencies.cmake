set(EALR_NS3_STATUS "disabled" CACHE INTERNAL "Availability of ns-3 dependencies")
set(EALR_NS3_AVAILABLE OFF CACHE INTERNAL "Whether required ns-3 targets are available")
set(EALR_5G_LENA_STATUS "disabled" CACHE INTERNAL "Availability of 5G-LENA NR dependencies")
set(EALR_5G_LENA_AVAILABLE OFF CACHE INTERNAL "Whether the ns3::nr target is available")

function(ealr_find_ns3)
    # The packaged ns-3 NetAnim dependency graph can include the MPI module
    # even when this project does not use distributed simulation directly.
    # Define its imported target before loading ns-3's exported targets.
    find_package(MPI QUIET COMPONENTS CXX)
    find_package(ns3 CONFIG QUIET)

    if(ns3_FOUND AND MPI_CXX_FOUND AND TARGET ns3::core AND TARGET ns3::network AND TARGET ns3::mobility AND TARGET ns3::netanim)
        set(EALR_NS3_STATUS "available" CACHE INTERNAL "Availability of ns-3 dependencies" FORCE)
        set(EALR_NS3_AVAILABLE ON CACHE INTERNAL "Whether required ns-3 targets are available" FORCE)
        message(STATUS "Found ns-3 core, network, mobility, and NetAnim targets.")

        if(TARGET ns3::nr)
            set(EALR_5G_LENA_STATUS "available" CACHE INTERNAL "Availability of 5G-LENA NR dependencies" FORCE)
            set(EALR_5G_LENA_AVAILABLE ON CACHE INTERNAL "Whether the ns3::nr target is available" FORCE)
            message(STATUS "Found 5G-LENA NR target (ns3::nr).")
        else()
            set(EALR_5G_LENA_STATUS "not found (install a 5G-LENA-enabled ns-3 package for NR device setup)" CACHE INTERNAL "Availability of 5G-LENA NR dependencies" FORCE)
            set(EALR_5G_LENA_AVAILABLE OFF CACHE INTERNAL "Whether the ns3::nr target is available" FORCE)
            message(WARNING "5G-LENA NR target ns3::nr was not found. ns-3 mobility and NetAnim integration remains available; NR device setup is disabled.")
        endif()
    else()
        set(EALR_NS3_STATUS "not found (CMake-only bootstrap remains buildable)" CACHE INTERNAL "Availability of ns-3 dependencies" FORCE)
        set(EALR_NS3_AVAILABLE OFF CACHE INTERNAL "Whether required ns-3 targets are available" FORCE)
        set(EALR_5G_LENA_STATUS "not found" CACHE INTERNAL "Availability of 5G-LENA NR dependencies" FORCE)
        set(EALR_5G_LENA_AVAILABLE OFF CACHE INTERNAL "Whether the ns3::nr target is available" FORCE)
        message(WARNING "Required ns-3 targets or MPI C++ support were not found. Configure with -DEALR_ENABLE_NS3=OFF for a quiet bootstrap, or set CMAKE_PREFIX_PATH to the ns-3 installation.")
    endif()
endfunction()
