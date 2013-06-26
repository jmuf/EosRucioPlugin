// ----------------------------------------------------------------------
// File: EosRucioOfs.cc
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

/*----------------------------------------------------------------------------*/
#include <cstdio>
/*----------------------------------------------------------------------------*/
#include "EosRucioOfs.hh"
/*----------------------------------------------------------------------------*/
#include "XrdOuc/XrdOucTrace.hh"
#include "XrdOuc/XrdOucString.hh"
#include "XrdOss/XrdOssApi.hh"
/*----------------------------------------------------------------------------*/

// The global OFS handle
EosRucioOfs* gOFS;

extern XrdSysError OfsEroute;
extern XrdOssSys  *XrdOfsOss;
extern XrdOfs     *XrdOfsFS;
extern XrdOucTrace OfsTrace;

extern XrdOss* XrdOssGetSS( XrdSysLogger*, 
                            const char*, 
                            const char* );


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
extern "C"
{
  XrdSfsFileSystem *XrdSfsGetFileSystem(XrdSfsFileSystem *native_fs, 
                                        XrdSysLogger     *lp,
                                        const char       *configfn)
  {
    // Do the herald thing
    //
    OfsEroute.SetPrefix("RucioOfs_");
    OfsEroute.logger(lp);
    XrdOucString version = "RucioOfs (Object Storage File System) ";
    version += VERSION;
    OfsEroute.Say("++++++ (c) 2013 CERN/IT-DSS ",
                  version.c_str());

    //static XrdVERSIONINFODEF( info, XrdOss, XrdVNUMBER, XrdVERSION);

    // Initialize the subsystems
    gOFS = new EosRucioOfs();
    
    gOFS->ConfigFN = (configfn && *configfn ? strdup(configfn) : 0);

    if ( gOFS->Configure(OfsEroute) ) return 0;
    
    XrdOfsFS = gOFS;
    return gOFS;
  }
}

//******************************************************************************
//                           E o s R u c i o O f s
//******************************************************************************

//------------------------------------------------------------------------------
// Constructor 
//------------------------------------------------------------------------------
EosRucioOfs::EosRucioOfs(): XrdOfs()
{
  // emtpy
}


//------------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------------
EosRucioOfs::~EosRucioOfs()
{
  //empty
}


//******************************************************************************
//                        E o s R u c i o O f s F i l e
//******************************************************************************


//------------------------------------------------------------------------------
// Constructor 
//------------------------------------------------------------------------------
EosRucioOfsFile::EosRucioOfsFile(const char *user, int MonID):
    XrdOfsFile(user, MonID)
{
  OfsEroute.Emsg("EosRucioOfsFile", "Calling constructor");
}


//------------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------------
EosRucioOfsFile::~EosRucioOfsFile()
{
  //empty
}


//------------------------------------------------------------------------------
// Open file
//------------------------------------------------------------------------------
int
EosRucioOfsFile::open(const char* fileName,
                      XrdSfsFileOpenMode openMode,
                      mode_t createMode,
                      const XrdSecEntity* client,
                      const char* opaque)
{
  OfsEroute.Emsg("EosRucioOfsFile::open", "Calling function");
  return XrdOfsFile::open(fileName, openMode, createMode, client, opaque);  
}








