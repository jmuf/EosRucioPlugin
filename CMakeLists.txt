#-------------------------------------------------------------------------------
# File: CMakeLists.txt
# Author: Elvin-Alin Sindrilaru - CERN
#-------------------------------------------------------------------------------

# ************************************************************************
# * EOS - the CERN Disk Storage System                                   *
# * Copyright (C) 2011 CERN/Switzerland                                  *
# *                                                                      *
# * This program is free software: you can redistribute it and/or modify *
# * it under the terms of the GNU General Public License as published by *
# * the Free Software Foundation, either version 3 of the License, or    *
# * (at your option) any later version.                                  *
# *                                                                      *
# * This program is distributed in the hope that it will be useful,      *
# * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
# * GNU General Public License for more details.                         *
# *                                                                      *
# * You should have received a copy of the GNU General Public License    *
# * along with this program.  If not, see <http://www.gnu.org/licenses/>.*
# ************************************************************************

################################################################################
# Version Definition
################################################################################
set(VERSION_MAJOR "0" )
set(VERSION_MINOR "1")
set(VERSION_PATCH "0")

################################################################################
# main section
################################################################################
cmake_minimum_required (VERSION 2.6)
project (eosrucioplugin)

if (RELEASE)
else (RELEASE)
set(RELEASE "0")
endif (RELEASE)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_FLAGS "-g -Wall -O2 -msse4.2 -std=gnu++0x -DVERSION=\\\"${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}\\\" ")

if(MacOSX)
set(LIB_INSTALL_DIR lib)
else(MacOSX)
set(LIB_INSTALL_DIR lib64)
endif(MacOSX)

find_package(XRootD REQUIRED)
find_package(curl REQUIRED)
find_package(OpenSSL REQUIRED)

set(CMAKE_INSTALL_PREFIX /usr/)
add_subdirectory(src)

################################################################################
# source packaging 
################################################################################
add_custom_target( dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source )

set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME
  "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${RELEASE}")
set(CPACK_SOURCE_IGNORE_FILES "/dsi/;/build/;/.deps/;/.bzr/;~$;'.'o$;/CMakeFiles/;Makefile;/_CPack_Packages/;.tar.gz$;.tar.bz2;CMakeCache.txt;build;install_manifest.txt;lib;/.git/;${CPACK_SOURCE_IGNORE_FILES};")

include(CPack)

################################################################################
# source + binary rpms
################################################################################

add_custom_target( srpm COMMAND rpmbuild -ts ${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${RELEASE}.tar.gz )

add_dependencies( srpm dist )

add_custom_target( rpm COMMAND rpmbuild -tb ${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${RELEASE}.tar.gz )

add_dependencies( rpm dist )




