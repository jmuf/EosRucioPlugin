// ----------------------------------------------------------------------
// File: EosRucioOfs.hh
// Author: Elvin-Alin Sindrilaru - CERN
// ----------------------------------------------------------------------

/************************************************************************
 * EOS - the CERN Disk Storage System                                   *
 * Copyright (C) 2013 CERN/Switzerland                                  *
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
  //! Configure routine 
  //----------------------------------------------------------------------------
  virtual int Configure(XrdSysError& error);

 
  //----------------------------------------------------------------------------
  //! Stat function
  //----------------------------------------------------------------------------
  int stat( const char*             path,
            struct stat*            buf,
            XrdOucErrInfo&          out_error,
            const XrdSecEntity*     client,
            const char*             opaque = 0 );

 private:

  std::string mUplinkInstance; ///< Uplink instance host:port
  std::string mUplinkHost; ///< uplink host address where failed req are redirected
  unsigned int mUplinkPort; ///< uplink host port where failed req are redirected
};


//------------------------------------------------------------------------------
//! Global OFS object
//------------------------------------------------------------------------------
extern EosRucioOfs* gOFS;


#endif //__EOS_EOSRUCIOOFS_HH__







