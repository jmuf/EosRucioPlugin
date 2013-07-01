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
#include <map>
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
  //! New directory
  //----------------------------------------------------------------------------
  XrdSfsDirectory *newDir(char *user = 0, int MonID = 0);


  //----------------------------------------------------------------------------
  //! New file
  //----------------------------------------------------------------------------
  XrdSfsFile *newFile(char *user = 0,int MonID = 0);


  //----------------------------------------------------------------------------
  //! Configure routine 
  //----------------------------------------------------------------------------
  virtual int Configure(XrdSysError& error);
  
  
  //----------------------------------------------------------------------------
  //! Stat function
  //----------------------------------------------------------------------------
  int stat( const char*             path,
            struct stat*            buf,
            XrdOucErrInfo&          outError,
            const XrdSecEntity*     client,
            const char*             opaque = 0 );

  //!!!!!!!!!!! XXX !!!!!!!!!!!!!!!!!
  //TODO: move this in the private section or create a new class 
  //----------------------------------------------------------------------------
  //! Translate logical file name to physical file name using the Rucio alg.
  //!
  //! @param lfn logical file name
  //!
  //! @return translated physical file name
  //!
  //----------------------------------------------------------------------------
  std::string Translate(std::string lfn);


 private:

  ///! map between space tokend and their priorities
  std::map<std::string, int> mMapSpace;

  std::string mSiteName; ///! site parameter for the Rucio translation
  std::string mJsonFile; ///! local json file for the Rucio translation
  std::string mAgisSite; ///! AGIS site for Rucio translation
  

  //----------------------------------------------------------------------------
  //! Read space tokens configuration from the AGIS site. If this is successful
  //! then the map containing the space tokens will be populated in this step.
  //!
  //! @return true if reading and parsing were successful, otherwise false 
  //!
  //----------------------------------------------------------------------------
  bool ReadAgisConfig();


  //----------------------------------------------------------------------------
  //! Read local JSON file and populate the amp
  //!
  //! @param path local path to JSON file
  //!
  //! @return true if JSON file found and parsed successfully, otherwise false
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
  //----------------------------------------------------------------------------
  static size_t HandleData(void* ptr, size_t size, size_t nmemb, void* user_specific); 
  
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


