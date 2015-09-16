# - find Sqlite 3
# SQLITE_INCLUDE_DIR - Where to find Sqlite 3 header files (directory)
# SQLITE_LIBRARIES - Sqlite 3 libraries
# SQLITE_LIBRARY_RELEASE - Where the release library is
# SQLITE_LIBRARY_DEBUG - Where the debug library is
# SQLITE_FOUND - Set to TRUE if we found everything (library, includes and executable)

# Copyright (c) 2010 Pau Garcia i Quiles, <pgquiles@elpauer.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# Generated by CModuler, a CMake Module Generator - http://gitorious.org/cmoduler

IF( SQLITE_INCLUDE_DIR AND SQLITE_LIBRARY_RELEASE AND SQLITE_LIBRARY_DEBUG )
    SET(SQLITE_FIND_QUIETLY TRUE)
ENDIF( SQLITE_INCLUDE_DIR AND SQLITE_LIBRARY_RELEASE AND SQLITE_LIBRARY_DEBUG )

FIND_PATH( SQLITE_INCLUDE_DIR sqlite3.h  )

FIND_LIBRARY(SQLITE_LIBRARY_RELEASE NAMES sqlite3 )

FIND_LIBRARY(SQLITE_LIBRARY_DEBUG NAMES sqlite3 sqlite3d  HINTS /usr/lib/debug/usr/lib/ )

IF( SQLITE_LIBRARY_RELEASE OR SQLITE_LIBRARY_DEBUG AND SQLITE_INCLUDE_DIR )
        SET( SQLITE_FOUND TRUE )
ENDIF( SQLITE_LIBRARY_RELEASE OR SQLITE_LIBRARY_DEBUG AND SQLITE_INCLUDE_DIR )

IF( SQLITE_LIBRARY_DEBUG AND SQLITE_LIBRARY_RELEASE )
        # if the generator supports configuration types then set
        # optimized and debug libraries, or if the CMAKE_BUILD_TYPE has a value
        IF( CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE )
                SET( SQLITE_LIBRARIES optimized ${SQLITE_LIBRARY_RELEASE} debug ${SQLITE_LIBRARY_DEBUG} )
        ELSE( CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE )
    # if there are no configuration types and CMAKE_BUILD_TYPE has no value
    # then just use the release libraries
                SET( SQLITE_LIBRARIES ${SQLITE_LIBRARY_RELEASE} )
        ENDIF( CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE )
ELSEIF( SQLITE_LIBRARY_RELEASE )
        SET( SQLITE_LIBRARIES ${SQLITE_LIBRARY_RELEASE} )
ELSE( SQLITE_LIBRARY_DEBUG AND SQLITE_LIBRARY_RELEASE )
        SET( SQLITE_LIBRARIES ${SQLITE_LIBRARY_DEBUG} )
ENDIF( SQLITE_LIBRARY_DEBUG AND SQLITE_LIBRARY_RELEASE )

IF( SQLITE_FOUND )
        IF( NOT SQLITE_FIND_QUIETLY )
                MESSAGE( STATUS "Found Sqlite3 header file in ${SQLITE_INCLUDE_DIR}")
                MESSAGE( STATUS "Found Sqlite3 libraries: ${SQLITE_LIBRARIES}")
        ENDIF( NOT SQLITE_FIND_QUIETLY )
ELSE(SQLITE_FOUND)
        IF( SQLITE_FIND_REQUIRED)
                MESSAGE( FATAL_ERROR "Could not find Sqlite3" )
        ELSE( SQLITE_FIND_REQUIRED)
                MESSAGE( STATUS "Optional package Sqlite3 was not found" )
        ENDIF( SQLITE_FIND_REQUIRED)
ENDIF(SQLITE_FOUND)