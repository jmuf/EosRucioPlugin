// -----------------------------------------------------------------------------
// File: EosRucioCms.hh
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

#ifndef __EOS_EOSRUCIOCMS_HH__
#define __EOS_EOSRUCIOCMS_HH__

/*----------------------------------------------------------------------------*/
#include "XrdCms/XrdCmsClient.hh"
#include "XrdSys/XrdSysPthread.hh"
/*----------------------------------------------------------------------------*/
#include <string>
#include <map>
/*----------------------------------------------------------------------------*/

//------------------------------------------------------------------------------
//! Class EosRucioCms used to the Rucio translation for the files in EOS
//------------------------------------------------------------------------------
class EosRucioCms: public XrdCmsClient
{
  public:

    //--------------------------------------------------------------------------
    //! Constuctor
    //--------------------------------------------------------------------------
    EosRucioCms(XrdSysLogger* logger);


    //--------------------------------------------------------------------------
    //! Destructor
    //--------------------------------------------------------------------------
    virtual ~EosRucioCms();


    //--------------------------------------------------------------------------
    //! Configue() is called to configure the client. If the client is obtained
    //!            via a plug-in then Parms are the  parameters specified after
    //!            cmslib path. It is zero if no parameters exist.
    //!
    //! @param cfn configuration file name
    //! @param Parms any parameters specified in the cmsd lib directive. If none,
    //!        the pointer may be null.
    //! @param EnvInfo environment information of the caller
    //!
    //! @return:   0 if failed, otherwise !0
    //!
    //--------------------------------------------------------------------------
    virtual int Configure(const char* cfn, char* Parms, XrdOucEnv* EnvInfo);


    //--------------------------------------------------------------------------
    //! Locate() is called to retrieve file location information. It is only used
    //!        on a manager node. This can be the list of servers that have a
    //!        file or the single server that the client should be sent to. The
    //!        "flags" indicate what is required and how to process the request.
    //!
    //!        SFS_O_LOCATE  - return the list of servers that have the file.
    //!                        Otherwise, redirect to the best server for the file.
    //!        SFS_O_NOWAIT  - w/ SFS_O_LOCATE return readily available info.
    //!                        Otherwise, select online files only.
    //!        SFS_O_CREAT   - file will be created.
    //!        SFS_O_NOWAIT  - select server if file is online.
    //!        SFS_O_REPLICA - a replica of the file will be made.
    //!        SFS_O_STAT    - only stat() information wanted.
    //!        SFS_O_TRUNC   - file will be truncated.
    //!
    //!        For any the the above, additional flags are passed:
    //!        SFS_O_META    - data will not change (inode operation only)
    //!        SFS_O_RESET   - reset cached info and recaculate the location(s).
    //!        SFS_O_WRONLY  - file will be only written    (o/w RDWR   or RDONLY).
    //!        SFS_O_RDWR    - file may be read and written (o/w WRONLY or RDONLY).
    //!
    //! @return:  As explained above
    //!
    //--------------------------------------------------------------------------
    virtual int Locate(XrdOucErrInfo& Resp,
                       const char* path,
                       int flags,
                       XrdOucEnv* Info = 0);

    //--------------------------------------------------------------------------
    //! Space() is called to obtain the overall space usage of a cluster. It is
    //!         only called on manager nodes.
    //!
    //! @return: Space information as defined by the response to kYR_statfs.
    //!          For a typical implementation see XrdCmsNode::do_StatFS().
    //!
    //--------------------------------------------------------------------------
    virtual int Space(XrdOucErrInfo& Resp,
                      const char* path,
                      XrdOucEnv* Info = 0)
    {
      return 0;
    };


  private:

    XrdSysRWLock mLockMap; ///< rw lock used to sync access to the map

    ///! map between space tokend and requests successfully statisfied
    ///! i.e. files for which the stat was succcessful in the corresponding
    ///! space token.
    std::map<std::string, uint64_t> mMapSpace;

    std::string mSiteName; ///< site parameter for the Rucio translation
    std::string mJsonFile; ///< local json file for the Rucio translation
    std::string mAgisSite; ///< AGIS site for Rucio translation
    std::string mEosInstance; ///< EOS instance host:port
    std::string mEosHost; ///< EOS host where requests are forwarded
    unsigned int mEosPort; ///< EOS port where requestts are forwarded
    std::string mUplinkInstance; ///< Uplink instance host:port
    std::string mUplinkHost; ///< uplink host address where failed req are redirected
    unsigned int mUplinkPort; ///< uplink host port where failed req are redirected

    //--------------------------------------------------------------------------
    //! Generate the full pfn path by concatenating the space tokens at the
    //! current site with the translated lfn.
    //!
    //! @param pfn_name parital pfn name obtained using the Translate method
    //!
    //! @return full pfn name
    //!
    //--------------------------------------------------------------------------
    std::string GetValidPfn(std::string pfn_partial);


    //--------------------------------------------------------------------------
    //! Translate logical file name to physical file name using the Rucio alg.
    //!
    //! @param lfn logical file name
    //!
    //! @return translated physical file name
    //!
    //--------------------------------------------------------------------------
    std::string Translate(std::string lfn);


    //--------------------------------------------------------------------------
    //! Compare method used to sort the list of space tokens by priority
    //!
    //! @param first first element
    //! @param second second element
    //!
    //! @return true if the first argument goes before the second one argument
    //!         in the strict weak ordering it defines, and falso otherwise.
    //!
    //--------------------------------------------------------------------------
    static bool CompareByPriority(std::pair<std::string, uint64_t>& first,
                                  std::pair<std::string, uint64_t>& second);


    //--------------------------------------------------------------------------
    //! Read space tokens configuration from the AGIS site. If this is successful
    //! then the map containing the space tokens will be populated in this step.
    //!
    //! @return true if reading and parsing were successful, otherwise false
    //!
    //--------------------------------------------------------------------------
    bool ReadAgisConfig();


    //----------------------------------------------------------------------------
    //! Read local JSON file and populate the amp
    //!
    //! @param path local path to JSON file
    //!
    //! @return true if JSON file found and parsed successfully, otherwise false
    //!
    //----------------------------------------------------------------------------
    bool ReadLocalJson(std::string path);


    //----------------------------------------------------------------------------
    //! Read file from URL. In particular this will be a JSON file.
    //!
    //! @param url url address
    //!
    //! @return contents for the file as a string
    //!
    //----------------------------------------------------------------------------
    std::string ReadFileFromUrl(std::string url);


    //----------------------------------------------------------------------------
    //! Handle data read from a remote URL location. According to CURL doc. this
    //! function needs to have this signature.
    //!
    //! @param ptr pointer to the data just read
    //! @param size size of elements
    //! @param nmemb number of elements (char)
    //! @param user_specific this is an agrument which can be used by the user
    //!        to pass local data between the application and the function that
    //!        gets invoked by libcurl. In our case this will be the address of
    //!        a string in which we read the whole file from the URL.
    //!
    //! @return number of bytes read
    //!
    //--------------------------------------------------------------------------
    static size_t HandleData(void* ptr, size_t size, size_t nmemb,
                             void* user_specific);

};

#endif //__EOS_EOSRUCIOCMS_HH__

