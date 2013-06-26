// ----------------------------------------------------------------------
// File: EosRucioOfs.hh
// Author: Elvin-Alin Sindrilaru - CERN
// ----------------------------------------------------------------------

/************************************************************************
 * EOS - the CERN Disk Storage System                                   *
 * Copyright (C) 2011 CERN/Switzerland                                  *
 *                                                                      *
 * This program is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.*
 ************************************************************************/

#ifndef __EOS_EOSRUCIOOFS_HH__
#define __EOS_EOSRUCIOOFS_HH__

/*----------------------------------------------------------------------------*/
#include "XrdOfs/XrdOfs.hh"
#include <string>
/*----------------------------------------------------------------------------*/

//------------------------------------------------------------------------------
//! Class EosRucioOfs built on top of XrdOfs
//------------------------------------------------------------------------------
class EosRucioOfs: public XrdOfs
{
 public:

  //----------------------------------------------------------------------------
  //! Constuctor
  //----------------------------------------------------------------------------
  EosRucioOfs();


  //----------------------------------------------------------------------------
  //! Destructor
  //----------------------------------------------------------------------------
  virtual ~EosRucioOfs();

 
  //----------------------------------------------------------------------------
  //! Stat function
  //----------------------------------------------------------------------------
  int stat( const char*             path,
            struct stat*            buf,
            XrdOucErrInfo&          outError,
            const XrdSecEntity*     client,
            const char*             opaque = 0 );

  

 private:
  
};


//------------------------------------------------------------------------------
//! Class EosRucioOfsFile built on top of XrdOfsFile
//------------------------------------------------------------------------------
class EosRucioOfsFile: public XrdOfsFile
{
 public:

  //----------------------------------------------------------------------------
  //! Constuctor
  //!
  //! @param user
  //! @param MonID
  //!
  //----------------------------------------------------------------------------
  EosRucioOfsFile(const char *user, int MonID);;


  //----------------------------------------------------------------------------
  //! Destructor
  //----------------------------------------------------------------------------
  virtual ~EosRucioOfsFile();

 
  //----------------------------------------------------------------------------
  //! Open file
  //!
  //! @param fileName file name
  //! @param openMode flags for the open operation
  //! @param createMode mode of the open operation
  //! @param client security entity
  //! @param opaque opaque information
  //!
  //! @return SFS_REDIRECT either to the EOS instance with the corresponding
  //!         opaque information containing the physical file name obtainded
  //!         after the translation or a meta manager higher in the hierarchy
  //!         if the file is not in EOS
  //!
  //----------------------------------------------------------------------------
  int open(const char* fileName,
           XrdSfsFileOpenMode openMode,
           mode_t createMode,
           const XrdSecEntity* client,
           const char* opaque = 0);

 private:
  
};


//------------------------------------------------------------------------------
//! Global OFS object
//------------------------------------------------------------------------------
extern EosRucioOfs* gOFS;


#endif //__EOS_EOSRUCIOOFS_HH__


