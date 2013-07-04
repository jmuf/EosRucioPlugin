# ----------------------------------------------------------------------
# File: CMakeLists.txt
# Author: Elvin-Alin Sindrilaru - CERN
# ----------------------------------------------------------------------

# ************************************************************************
# * EOS - the CERN Disk Storage System                                   *
# * Copyright (C) 2013 CERN/Switzerland                                  *
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

include_directories( ./  
		     ${CURL_INCLUDE_DIRS}
		     ${XROOTD_INCLUDE_DIR} 
		     ${XROOTD_PRIVATE_INCLUDE_DIR} )

add_library(EosRucioCms MODULE
	    EosRucioCms.cc         EosRucioCms.hh
	    )		 

add_library(EosRucioOfs MODULE
	    EosRucioOfs.cc         EosRucioOfs.hh
	    )

target_link_libraries(EosRucioCms XrdCl ${CURL_LIBRARIES} crypto)
target_link_libraries(EosRucioOfs XrdOfs XrdServer XrdCl)

if (Linux)
  set_target_properties (EosRucioCms EosRucioOfs PROPERTIES
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    SOVERSION ${VERSION_MAJOR}
    CLEAN_DIRECT_OUTPUT 1
  )
endif(Linux)

install( TARGETS EosRucioCms EosRucioOfs
         LIBRARY DESTINATION ${LIB_INSTALL_DIR}
         ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)

