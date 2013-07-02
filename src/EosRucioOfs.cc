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
#include <memory>
#include <list>
#include <fstream>
#include <sstream>
#include <fcntl.h>
#include <curl/curl.h>
#include <openssl/md5.h>
#include "rapidjson/document.h"
/*----------------------------------------------------------------------------*/
#include "EosRucioOfs.hh"
/*----------------------------------------------------------------------------*/
#include "XrdOuc/XrdOucTrace.hh"
#include "XrdOuc/XrdOucString.hh"
#include "XrdOss/XrdOssApi.hh"
#include "XrdOuc/XrdOucTokenizer.hh"
#include "XrdCl/XrdClFile.hh"
/*----------------------------------------------------------------------------*/

// The global OFS handle
EosRucioOfs* gOFS;

extern XrdSysError OfsEroute;
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
    OfsEroute.SetPrefix("EosRucioOfs_");
    OfsEroute.logger(lp);
    XrdOucString version = "EosRucioOfs (Object Storage File System) ";
    version += VERSION;
    OfsEroute.Say("++++++ (c) 2013 CERN/IT-DSS ",
                  version.c_str());

    //static XrdVERSIONINFODEF( info, XrdOss, XrdVNUMBER, XrdVERSION);

    // Initialize the subsystems
    gOFS = new EosRucioOfs();
    
    gOFS->ConfigFN = (configfn && *configfn ? strdup(configfn) : 0);

    if (gOFS->Configure(OfsEroute))
      return 0;
    
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
EosRucioOfs::EosRucioOfs():
  XrdOfs(),
  mSiteName(""),
  mJsonFile(""),
  mAgisSite(""),
  mEosInstance(""),
  mEosHost(""),
  mEosPort(0),
  mUplinkInstance(""),
  mUplinkHost(""),
  mUplinkPort(0)
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


//------------------------------------------------------------------------------
//! Configure routine 
//------------------------------------------------------------------------------
int
EosRucioOfs::Configure(XrdSysError& eroute)
{
  int NoGo = 0;
  int cfgFD;
  char* var;
  const char* val;
  std::string space_tkn;
  eroute.Emsg("EosRucioOfs::Configure", "Calling function");

  // Configure the basix XrdOfs and exit if not successful
  NoGo = XrdOfs::Configure(eroute);
  
  if (NoGo) 
    return NoGo;

  // extract the manager from the config file
  XrdOucStream Config(&eroute, getenv("XRDINSTANCE"));

  // Read in the rucio configuration from the xrd.cf.rucio file
  if (!ConfigFN || !*ConfigFN)
  {
    NoGo = 1;
    eroute.Emsg("Configure", "no configure file");
  }
  else
  {
    // Try to open the configuration file.
    if ((cfgFD = open(ConfigFN, O_RDONLY, 0)) < 0)
      return eroute.Emsg("Config", errno, "open config file fn=", ConfigFN);

    Config.Attach(cfgFD);
    std::string rucio_tag = "eosrucio.";
    
    while ((var = Config.GetMyFirstWord()))
    {
      if (!strncmp(var, rucio_tag.c_str(), rucio_tag.length()))
      {
        var += rucio_tag.length();

        // Get overwrite space tokens for local instance 
        std::string option_tag  = "overwriteSE";

        if (!strncmp(var, option_tag.c_str(), option_tag.length()))
        {
          while ((val = Config.GetToken()))
          {
            // Add a slash at the end if there is none already
            space_tkn = val;
                        
            if (space_tkn.at(space_tkn.length() - 1) != '/')
              space_tkn += '/';
            
            mMapSpace.insert(std::make_pair(space_tkn, 0));            
          }
        }

        // Get name for local SITE as specified in the Rucio scheme
        option_tag = "site";
        
        if (!strncmp(var, option_tag.c_str(), option_tag.length()))
        {
          if (!(val = Config.GetWord()))
          {
            eroute.Emsg("Configuration",
                        "Site name is mandatory, set it in /etc/xrd.cf.rucio.",
                        "Example: \"eosrucio.site CERN-EOS\"");
            NoGo = 1;
            break;
          }
          else
          {
            mSiteName = val;
          }
        }

        // Get path to local JSON file
        option_tag = "jsonfile";

        if (!strncmp(var, option_tag.c_str(), option_tag.length()))
        {
          if (!(val = Config.GetWord()))
          {
            eroute.Emsg("Configure", "JSON file not specified");
          }
          else
          {
            mJsonFile = val;
          }
        }

        // Get AGIS site address
        option_tag = "agis";
        
        if (!strncmp(var, option_tag.c_str(), option_tag.length()))
        {
          if (!(val = Config.GetWord()))
          {
            eroute.Emsg("Configure ", "AGIS site not specified");
          }
          else
          {
            mAgisSite = val;
          }
        }

        // Get EOS instance host address
        option_tag = "eoshost";
                  
        if (!strncmp(var, option_tag.c_str(), option_tag.length()))
        {
          if (!(val = Config.GetWord()))
            eroute.Emsg("Configure ", "No EOS redirect instance specified");
          else
            mEosHost = val;
        }

        // Get EOS instance port number
        option_tag = "eosport";
        
        if (!strncmp(var, option_tag.c_str(), option_tag.length()))
        {
          if (!(val = Config.GetWord()))
            eroute.Emsg("Configure ", "No EOS redirect instance specified");
          else
          {
            char* endptr;
            mEosPort = static_cast<unsigned int>(strtol(val, &endptr, 10));
            
            //Check for various possible errors 
            if ((errno == ERANGE && (mEosPort == LONG_MAX || mEosPort == LONG_MIN))
                || (errno != 0 && mEosPort == 0))
            {
              eroute.Emsg("Configure", "strtol error");
              mEosPort = 0;
            }
            else
            {
              if (endptr == val) {
                eroute.Emsg("Configure", "No digits were found when parsing eosport");
                mEosPort = 0;
              }
            }
          }
        }

        // Get Rucio N2N uplink host address
        option_tag = "uphost";
        
        if (!strncmp(var, option_tag.c_str(), option_tag.length()))
        {
          if (!(val = Config.GetWord()))
            eroute.Emsg("Configure ", "No uplink redirect instance specified");
          else
            mUplinkHost = val;
        }

        // Get Rucio N2N uplink port number
        option_tag = "upport";
        
        if (!strncmp(var, option_tag.c_str(), option_tag.length()))
        {
          if (!(val = Config.GetWord()))
            eroute.Emsg("Configure ", "No EOS redirect instance specified");
          else
          {
            char* endptr;
            mUplinkPort = static_cast<unsigned int>(strtol(val, &endptr, 10));
            
            //Check for various possible errors 
            if ((errno == ERANGE && (mUplinkPort == LONG_MAX || mUplinkPort == LONG_MIN))
                || (errno != 0 && mUplinkPort == 0))
            {
              eroute.Emsg("Configure", "strtol error");
              mUplinkPort = 0;
            }
            else
            {
              if (endptr == val) {
                eroute.Emsg("Configure", "No digits were found when parsing upport");
                mUplinkPort = 0;
              }
            }
          }
        }
      }
    }
  }

  // Check that the EOS instance is valid 
  if (mEosHost.empty() || (mEosPort == 0))
  {
    eroute.Emsg("Configure", "EOS redirect instance value missing/invalid",
                "Example \"eosrucio.eoshost eosatlas.cern.ch\"",
                "        \"eosrucio.eosport 1094\"");
    NoGo = 1;
    return NoGo;
  }
  else
  {
    std::stringstream sstr;
    sstr << mEosHost << ":" << mEosPort;
    mEosInstance = sstr.str();
    XrdCl::URL url(mEosInstance);
    
    if (!url.IsValid())
    {
      eroute.Emsg("Configure ", "EOS redirect url is not valid");
      mEosInstance = "";
      NoGo = 1;
    }
  }

  // Check that the uplink redirect is valid 
  if (mUplinkHost.empty() || (mUplinkPort == 0))
  {
    eroute.Emsg("Configure", "Uplink instance value missing/invalid",
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
      eroute.Emsg("Configure ", "Uplink redirect url is not valid");
      mUplinkInstance = "";
      NoGo = 1;
    }
  }

  // Check that the Rucio site name was specified 
  if (mSiteName.empty())
  {
    eroute.Emsg("Configure", "Mandatory site name value missing.",
                "Example \"eosrucio.site CERN-EOS\"");
    NoGo = 1;
    return NoGo;
  }

  eroute.Say("EosRucioOfs::Configure ", "Json file: ", mJsonFile.c_str());
  eroute.Say("EosRucioOfs::Configure ", "AGIS site: ", mAgisSite.c_str());
  eroute.Say("EosRucioOfs::Configure ", "Site: ", mSiteName.c_str());

  if (mMapSpace.empty())
  {
    bool done_agis = false;
    bool done_json = false;
    // This means that the overwriteSE tag did not specify any tokens, therefore
    // we need first to check the AGIS site and then the local JSON file
    
    if (!mAgisSite.empty())
    {
      // Try to read configuration of space tokens from the AGIS site
      eroute.Say("Configure: ", "Trying to read config from AGIS site");
      done_agis = ReadAgisConfig();
    }

    if (!done_agis && !mJsonFile.empty())
    {
      // Try to read configuration of space tokens from the json file
      eroute.Say("Configure: ", "Trying to read config from JSON file");
      done_json = ReadLocalJson(mJsonFile);   
    }

    if (!done_agis && !done_json)
    {
      eroute.Emsg("Configure",
                  "No resource (overwriteSE, AGIS site or local (JSON file)) from",
                  "which to read the space tokend is available" );
      NoGo = 1;
    }   
  }

  // Check that we have some mappings defined
  if (mMapSpace.empty())
  {
    OfsEroute.Emsg("Configure", "The Rucio space token map is empty");
    NoGo = 1;
  }
  
  eroute.Say("Contents of the space map is:");
  std::stringstream ss;
  
  for (auto entry = mMapSpace.begin(); entry != mMapSpace.end(); entry++)
  {
    ss << entry->second;
    eroute.Say(entry->first.c_str(), " " , ss.str().c_str());
    ss.str("");
  }

  return NoGo;
}


//----------------------------------------------------------------------------
//! New directory
//----------------------------------------------------------------------------
XrdSfsDirectory*
EosRucioOfs::newDir(char *user, int MonID)
{
  return 0;
}


//----------------------------------------------------------------------------
//! New file
//----------------------------------------------------------------------------
XrdSfsFile*
EosRucioOfs::newFile(char *user, int MonID)
{
  return (XrdSfsFile *)new EosRucioOfsFile(user, MonID);
}


//------------------------------------------------------------------------------
// Stat
//------------------------------------------------------------------------------
int
EosRucioOfs::stat(const char* path,
                  struct stat* buf,
                  XrdOucErrInfo& outError,
                  const XrdSecEntity* client,
                  const char* opaque)
{
  OfsEroute.Emsg("EosRucioOfs::stat", "Calling function");
  return XrdOfs::stat(path, buf, outError, client, opaque);  
}


//------------------------------------------------------------------------------
// Translate logical file name to physical file name using the Rucio alg.
//------------------------------------------------------------------------------
std::string
EosRucioOfs::Translate(std::string lfn)
{
  std::string pfn = "";
  std::string file_name = "";
  std::string scope = "";
  std::string prefix = "/atlas/rucio/";

  // We only translate file names that begin with /atlas
  if (lfn.compare(0, prefix.length(), prefix) == 0)
  {
    std::string tmp = lfn.substr(prefix.length(), lfn.length() - prefix.length());
    size_t last_colon = tmp.find_last_of(':');

    if (last_colon != std::string::npos)
    {
      file_name = tmp.substr(last_colon + 1);
      scope = tmp.substr(0, last_colon);
    }
    else
    {
      size_t last_slash = tmp.find_last_of('/');
      file_name = tmp.substr(last_slash + 1);
      scope = tmp.substr(0, last_slash);
    }

    if (file_name.empty() || scope.empty())
    {
      OfsEroute.Emsg("Translate", "Error extracting scope and/or file name");
      return pfn;
    }

    OfsEroute.Say("File name: ", file_name.c_str(), " scope: ", scope.c_str());
    std::string scope_file = scope + ":" + file_name;
    
    // Compute the MD5 hash 
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5((unsigned char*) scope_file.c_str(), scope_file.length(), (unsigned char*)&digest);

    // Now we need to 0 pad it to get the full 32 chars
    char zero_pad[3];
    zero_pad[2] = '\0';
    std::string md5_string;
    md5_string.reserve(32);
    
    for (int i = 0; i < 16; i++)
    {
      sprintf(&zero_pad[0], "%02x", (unsigned int)digest[i]);
      md5_string.append(zero_pad);
    }
     
    OfsEroute.Say("MD5 string is: ", md5_string.c_str());   

    std::stringstream sstr;
    sstr << "rucio/" << scope << "/" << md5_string.substr(0, 2) << "/" <<
      md5_string.substr(2, 2) << "/" << file_name;
    pfn = sstr.str();    
  }

  OfsEroute.Say("Pfn is: ", pfn.c_str());
  return pfn;
}


//------------------------------------------------------------------------------
// Compare method used to sort the list of space tokens by priority
//------------------------------------------------------------------------------
bool 
EosRucioOfs::CompareByPriority(std::pair<std::string, uint64_t>& first,
                               std::pair<std::string, uint64_t>& second)
{

  if (first.second > second.second) return true;
  else return false;
}


//------------------------------------------------------------------------------
// Generate the full pfn path by concatenating the space tokens at the current
// site with the translated lfn
//------------------------------------------------------------------------------
std::string 
EosRucioOfs::GetValidPfn(std::string lfn)
{
  bool found_file = false;
  std::string pfn_full = "";
  std::string pfn_partial = Translate(lfn);
  std::list< std::pair<std::string, uint64_t> > ordered_list;
  mMutexMap.Lock();   // -->

  for (auto it = mMapSpace.begin(); it != mMapSpace.end(); ++it)
  {
    ordered_list.push_back(*it);
  }
  mMutexMap.UnLock(); // <--

  ordered_list.sort(EosRucioOfs::CompareByPriority);
  std::stringstream sstr;
  XrdCl::URL url(mEosInstance);
  XrdCl::FileSystem fs(url);
  XrdCl::StatInfo *response = 0;
  XrdCl::Status status;

  for (auto it = ordered_list.begin(); it != ordered_list.end(); ++it)
  {
    sstr << "Path: " << it->first << " with priority: " << it->second;
    OfsEroute.Say(sstr.str().c_str());
    sstr.str("");

    sstr << it->first << pfn_partial;
    pfn_full = sstr.str();

    // Do stat
    OfsEroute.Say("Trying to stat file: ", pfn_full.c_str());
    status = fs.Stat(pfn_full, response);

    if (status.IsOK() && response)
    {
      OfsEroute.Say("Stat successful");
      if (response->TestFlags(XrdCl::StatInfo::IsReadable |
                              XrdCl::StatInfo::IsWritable))
      {
        // Update the map value
        mMutexMap.Lock();    // -->
        mMapSpace[it->first] = it->second + 1;
        mMutexMap.UnLock();  // <--
        found_file = true;
        break;
      }

      delete response;
      response = 0;
    }
  }

  if (!found_file)
    pfn_full.clear();
  
  OfsEroute.Say("Full pfn is: ", pfn_full.c_str());
  return pfn_full;
}


//------------------------------------------------------------------------------
// Handle the data read from the remote URL address i.e. JSON file
//------------------------------------------------------------------------------
size_t
EosRucioOfs::HandleData(void* ptr, size_t size, size_t nmemb, void* user_specific) 
{
  std::string* contents = static_cast<std::string*>(user_specific);

  // The data is not null-terminated, so replace the last character with '\0'
  int numbytes = size * nmemb;
  char lastchar = *((char*) ptr + numbytes - 1);
  *((char *) ptr + numbytes - 1) = '\0';
  contents->append((char*) ptr);
  contents->append(1, lastchar);   
  return numbytes;
}


//------------------------------------------------------------------------------
// Read file from URL
//------------------------------------------------------------------------------
std::string
EosRucioOfs::ReadFileFromUrl(std::string url)
{
  std::unique_ptr<std::string> contents = 
    std::unique_ptr<std::string>(new std::string(""));

  CURL* curl_handle = curl_easy_init();
  
  if (curl_handle)
  {
    curl_easy_setopt(curl_handle, CURLOPT_URL, mAgisSite.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, EosRucioOfs::HandleData);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, static_cast<void*>(contents.get()));
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);

    CURLcode res = curl_easy_perform(curl_handle);
    curl_easy_cleanup(curl_handle);

    if (res != 0)
    {
      OfsEroute.Emsg("error: ", curl_easy_strerror(res));
      contents->clear();
    }
  }

  return *contents;
}


//------------------------------------------------------------------------------
// Read space tokens for current site from AGIS 
//------------------------------------------------------------------------------
bool
EosRucioOfs::ReadAgisConfig()
{
  // We must use the site parameter to get the corresponsing configuration from
  // the AGIS site. We know that the site parameter is not empty.
  bool found = false;
  std::string site_tag = "rc_site";
  std::string aproto_tag = "aprotocols";
  std::string read_tag = "r";
  std::string json_content = ReadFileFromUrl(mAgisSite);
  std::string space_tkn;

  // Parse the contents using rapidjson
  rapidjson::Document d;
  d.Parse<0>(json_content.c_str());
  
  if (d.HasParseError())
  {
    OfsEroute.Emsg("ReadAgisConfig", "Error while parsing the JSON file");
    return found;
  }
  else 
  {
    for (rapidjson::Value::ConstValueIterator it_obj = d.Begin();
         it_obj != d.End(); ++it_obj)
    {
      if (it_obj->IsObject())
      {
        for (rapidjson::Value::ConstMemberIterator it_site = it_obj->MemberBegin();
             it_site != it_obj->MemberEnd(); ++it_site)
        {
          // Find our local site in the AGIS file
          if ((it_site->name.GetString() ==  site_tag) &&
              (mSiteName == it_site->value.GetString()))
          {
            found = true;

            // Iterate through the memeber of the SITE (CERN-PROD) object to find
            // the aprotocols object
            for (rapidjson::Value::ConstMemberIterator it_proto = it_obj->MemberBegin();
                 it_proto != it_obj->MemberEnd(); ++it_proto)
            {
              if (it_proto->name.GetString() == aproto_tag)
              {
                // Iterate through the aprotocols object and get the "r" object
                // and then add all the mappings to the global map
                for (rapidjson::Value::ConstMemberIterator it_read = it_proto->value.MemberBegin();
                 it_read != it_proto->value.MemberEnd(); ++it_read)
                {

                  // Find read tag "r" for current SITE
                  if (it_read->name.GetString() == read_tag)
                  {
                    for(rapidjson::Value::ConstValueIterator it_map = it_read->value.Begin();
                        it_map != it_read->value.End(); ++it_map)
                    {
                      const rapidjson::Value& entries = *it_map;

                      if (entries.Size() != 3)
                      {
                        OfsEroute.Emsg("ReadAgisConfig", "Warning: Incomplete record in AGIS");
                        continue;
                      }
                      else
                      {
                        // Add a slash at the end if there is none already
                        space_tkn = entries[2].GetString();
                        
                        if (space_tkn.at(space_tkn.length() - 1) != '/')
                          space_tkn += '/';

                        auto res_pair = mMapSpace.insert(std::make_pair(space_tkn, 0));

                        if (!res_pair.second)
                        {
                          OfsEroute.Say("Entry: ", space_tkn.c_str(), " already added!");
                        }
                      }
                    }
                  }
                }
              }
            }
          }

          if (found) break;
        }       
      }

      if (found) break;
    }
  }

  return found;
}


//------------------------------------------------------------------------------
// Read local JSON file and populate the amp
//------------------------------------------------------------------------------
bool
EosRucioOfs::ReadLocalJson(std::string path)
{
  bool done = false;
  std::string site_tag = "rc_site";
  std::string aproto_tag = "aprotocols";
  std::string space_tkn;
  std::ifstream in(path, std::ios::in);

  if (in)
  {
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();

    // Parse the contents using rapidjson
    rapidjson::Document d;
    d.Parse<0>(contents.c_str());
    
    if (d.HasParseError())
    {
      OfsEroute.Emsg("ReadLocalJson", "Error while parsing the JSON file");
    }
    else 
    {
      for (rapidjson::Value::ConstValueIterator it_obj = d.Begin();
           it_obj != d.End(); ++it_obj)
      {
        if (it_obj->IsObject())
        {
          for (rapidjson::Value::ConstMemberIterator it_site = it_obj->MemberBegin();
               it_site != it_obj->MemberEnd(); ++it_site)
          {
            // Find our local site in the AGIS file
            if ( it_site->name.IsString() && it_site->value.IsString() &&
                 (it_site->name.GetString() ==  site_tag) &&
                 (mSiteName == it_site->value.GetString()))
            {
              for (rapidjson::Value::ConstMemberIterator it_proto = it_obj->MemberBegin();
                   it_proto != it_obj->MemberEnd(); ++it_proto)
              {
                if (it_proto->name.IsString() && (it_proto->name.GetString() == aproto_tag))
                {
                  done = true;
                  
                  for (rapidjson::Value::ConstValueIterator it_values = it_proto->value.Begin();
                       it_values != it_proto->value.End(); ++it_values)
                  {
                    // Add a slash at the end if there is none already
                    space_tkn = it_values->GetString();
                    
                    if (space_tkn.at(space_tkn.length() - 1) != '/')
                      space_tkn += '/';

                    auto res_pair = mMapSpace.insert(std::make_pair(space_tkn, 0));
                    
                    if (!res_pair.second)
                    {
                      OfsEroute.Say("Entry: ", space_tkn.c_str(), " already added!");
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  return done;
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

  // TODO: compute the Rucio pfn using the algorithm and redirect to the correct
  // location. Also need to do a stat.
  std::string pfn = gOFS->GetValidPfn(fileName);
  std::string ret_string;

  if (pfn.empty())
  {
    OfsEroute.Emsg("open", "Error, file not found in EOS, redirect to uplink host");
    ret_string = gOFS->GetUplinkHost();
    error.setErrData(ret_string.c_str());
    error.setErrCode(gOFS->GetUplinkPort());
    return gOFS->fsError(error, SFS_REDIRECT);
  }
  else
  {
    // Add pfn as opaque information with the tag eos.lfn
    OfsEroute.Say("Redirect to EOS instance");
    ret_string = gOFS->GetEosHost();
    ret_string += "?eos.lfn=";
    ret_string += pfn;
    ret_string += "&eos.app=rucio";
    error.setErrData(ret_string.c_str());
    error.setErrCode(gOFS->GetEosPort());
    
    return gOFS->fsError(error, SFS_REDIRECT);
  }
}


