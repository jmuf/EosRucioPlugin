// -----------------------------------------------------------------------------
// File: EosRucioOfs.hh
// Author: Elvin-Alin Sindrilaru - CERN
// -----------------------------------------------------------------------------

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

/*----------------------------------------------------------------------------*/
#include <cstdio>
#include <fcntl.h>
/*----------------------------------------------------------------------------*/
#include "EosRucioOfs.hh"
/*----------------------------------------------------------------------------*/
#include "XrdOuc/XrdOucTrace.hh"
#include "XrdOuc/XrdOucString.hh"
#include "XrdOss/XrdOssApi.hh"
#include "XrdCl/XrdClFileSystem.hh"
/*----------------------------------------------------------------------------*/

// The global OFS handle
EosRucioOfs* gOFS;

extern XrdSysError OfsEroute;
extern XrdOfs* XrdOfsFS;

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
extern "C"
{
  XrdSfsFileSystem* XrdSfsGetFileSystem(XrdSfsFileSystem* native_fs,
                                        XrdSysLogger* lp,
                                        const char* configfn)
  {
    // Do the herald thing
    //
    OfsEroute.SetPrefix("RucioOfs_");
    OfsEroute.logger(lp);
    XrdOucString version = "RuciOfs (Object Storage File System) ";
    version += VERSION;
    OfsEroute.Say("++++++ (c) 2013 CERN/IT-DSS ", version.c_str());
    // Initialize the subsystems
    gOFS = new EosRucioOfs();
    gOFS->ConfigFN = (configfn && *configfn ? strdup(configfn) : 0);

    if (gOFS->Configure(OfsEroute)) return 0;

    XrdOfsFS = gOFS;
    return gOFS;
  }
}

//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------
EosRucioOfs::EosRucioOfs():
  XrdOfs(),
  mUplinkInstance(""),
  mUplinkHost(""),
  mUplinkPort(0)
{
  // empty
}


//------------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------------
EosRucioOfs::~EosRucioOfs()
{
  //empty
}


//------------------------------------------------------------------------------
// Configure routine which just needs to read the extra tag eosrucio.uphost
//------------------------------------------------------------------------------
int
EosRucioOfs::Configure(XrdSysError& error)
{
  int NoGo = 0;
  int cfgFD;
  char* var;
  const char* val;
  std::string space_tkn;
  error.Emsg("EosRucioOfs::Configure", "Calling function");
  // Configure the basix XrdOfs and exit if not successful
  NoGo = XrdOfs::Configure(error);

  if (NoGo)
    return NoGo;

  // extract the manager from the config file
  XrdOucStream Config(&error, getenv("XRDINSTANCE"));

  // Read in the rucio configuration from the xrd.cf.rucio file
  if (!ConfigFN || !*ConfigFN)
  {
    NoGo = 1;
    error.Emsg("Configure", "no configure file");
  }
  else
  {
    // Try to open the configuration file.
    if ((cfgFD = open(ConfigFN, O_RDONLY, 0)) < 0)
      return error.Emsg("Configure", errno, "open config file fn=", ConfigFN);

    Config.Attach(cfgFD);
    std::string rucio_tag = "eosrucio.";

    while ((var = Config.GetMyFirstWord()))
    {
      if (!strncmp(var, rucio_tag.c_str(), rucio_tag.length()))
      {
        var += rucio_tag.length();
        // Get Rucio N2N uplink host address
        std::string option_tag = "uphost";

        if (!strncmp(var, option_tag.c_str(), option_tag.length()))
        {
          if (!(val = Config.GetWord()))
            error.Emsg("Configure ", "No uplink redirect instance specified");
          else
            mUplinkHost = val;
        }

        // Get Rucio N2N uplink port number
        option_tag = "upport";

        if (!strncmp(var, option_tag.c_str(), option_tag.length()))
        {
          if (!(val = Config.GetWord()))
            error.Emsg("Configure ", "No EOS redirect instance specified");
          else
          {
            char* endptr;
            mUplinkPort = static_cast<unsigned int>(strtol(val, &endptr, 10));

            //Check for various possible errors
            if ((errno == ERANGE && (mUplinkPort == LONG_MAX || mUplinkPort == LONG_MIN))
                || (errno != 0 && mUplinkPort == 0))
            {
              error.Emsg("Configure", "strtol error");
              mUplinkPort = 0;
            }
            else
            {
              if (endptr == val)
              {
                error.Emsg("Configure", "No digits were found when parsing upport");
                mUplinkPort = 0;
              }
            }
          }
        }
      }
    }
  }

  // Check that the uplink redirect is valid
  if (mUplinkHost.empty() || (mUplinkPort == 0))
  {
    error.Emsg("Configure", "Uplink instance value missing/invalid",
               "Example \"eosrucio.uphost atlas-xrd-eu.cern.ch\"",
               "        \"eosrucio.upport 1094\"");
    NoGo = 1;
    return NoGo;
  }
  else
  {
    std::stringstream sstr;
    sstr << mUplinkHost << ":" << mUplinkPort;
    mUplinkInstance = sstr.str();
    XrdCl::URL url(mUplinkInstance);

    if (!url.IsValid())
    {
      error.Emsg("Configure ", "Uplink redirect url is not valid");
      mUplinkInstance = "";
      NoGo = 1;
    }
  }

  return NoGo;
}


//------------------------------------------------------------------------------
//! Rewrite the stat method so that it actually returns OK if the LFC transaltion
//! results in a redirection. This is because the old client can not handle the
//! redirection on stat nad therefore we it fake and tell it we have the file.
//! Once once it requests it, we actually go to the EOS instance with the name
//! translation already done.
//------------------------------------------------------------------------------
int
EosRucioOfs::stat(const char*             path,
                  struct stat*            buf,
                  XrdOucErrInfo&          out_error,
                  const XrdSecEntity*     client,
                  const char*             opaque)
{
  int retc = XrdOfs::stat(path, buf, out_error, client, opaque);

  if (retc == SFS_REDIRECT)
  {
    std::string err_data = out_error.getErrData();

    if (err_data == mUplinkHost)
    {
      //........................................................................
      // If the file is not in EOS we redirect up to a meta manager and we
      // signal that we don't have the file by replying SFS_ERROR
      //........................................................................
      OfsEroute.Emsg("stat", "Not found int EOS, Rucio file: ", path);
      return SFS_ERROR;
    }
    else
    {
      // We know that the file is in EOS and accessible since the EosRucioCms::
      // Locate methos stat-ed the file. Therefore, here we just to a dummy
      // stat to populate the stat buffer passed to this functon.
      lstat("/etc/passwd", buf);
      OfsEroute.Emsg("stat", "Found in EOS Rucio file: ", path);
      return SFS_OK;
    }
  }

  return retc;
}


