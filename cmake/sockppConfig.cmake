# sockppConfig.cmake
#
# The following import target is always created:
#
#   Sockpp::sockpp
#
# This is for whichever target is build (shared or static). 
# If both are built it is for the shared target.
#
# One or both of the following import targets will be created depending
# on the configuration:
#
#   Sockpp::sockpp-shared
#   Sockpp::sockpp-static
#

include(CMakeFindDependencyMacro)

if(NOT TARGET Sockpp::sockpp-shared AND NOT TARGET Sockpp::sockpp-static)
	include("${CMAKE_CURRENT_LIST_DIR}/sockppTargets.cmake")

    if(TARGET Sockpp::sockpp-shared)
        add_library(Sockpp::sockpp ALIAS Sockpp::sockpp-shared)
    else()
        add_library(Sockpp::sockpp ALIAS Sockpp::sockpp-static)
    endif()

endif()

