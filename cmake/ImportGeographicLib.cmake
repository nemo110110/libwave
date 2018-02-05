# - Defines an imported target for GeographicLib
# This script is meant to be used after find_package(Eigen)
# It adds the imported target Eigen3::Eigen if it doesn't already exist, making
# Eigen below 3.3.1 consistent with newer versions.
#
# Eigen can then be used with
#     TARGET_LINK_LIBRARIES(< my_target > Eigen3::Eigen)
# instead of with INCLUDE_DIRECTORIES

IF(GeographicLib_FOUND)
    ADD_LIBRARY(GeographicLib INTERFACE IMPORTED)
    SET_PROPERTY(TARGET GeographicLib PROPERTY
            INTERFACE_INCLUDE_DIRECTORIES "${GeographicLib_INCLUDE_DIR}")
ENDIF()
