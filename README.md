EosRucioPlugin
==============

The project contains two plugins for the XRootD server used to translate logical file names to physical
ones using the [**Rucio**](https://twiki.cern.ch/twiki/bin/view/Atlas/MovingToRucio#n2n) algorithm from the **ATLAS**
experiement. The plugins also enable federating the **ATLAS** data from **EOS** in FAX.

The project translates lfns to pfns by using the Rucio algorithm and then checks if the file is in EOS. If the check
is successful then the client is redirected to the EOS instace with the pfn added as opaque information using the 
tag **"eos.lfn="** (the confusion between lfn and pfn here is due to the fact that for ATLAS the file name in EOS is a 
pfn but in reality in EOS this is just an lfn).

Instalation
-----------

For the translation to work and the server to join the federation one needs the following configuration of daemons:
* XRootD+cmsd fed which connects to a regional redirector and forwards request to the Rucio XRootD

* XRootD running in Rucio mode doing the actual translations

Configuration
-------------

The project provides in the **config** directory configuration files examples for all three daemons that need 
to run on the box. Besides the ususal xrootd and cms configuration directives, we introduce a new tag: **eosrucio**
used to configure various parameter values relating to the **Rucio** scheme. Below one can find a brief description
of them.

* overwriteSE - list of space tokens to be used at the current site in order to construct the physical file name. 
                This one can be used to hide some parts of the namespace from FAX.
* site - name of the current site in Rucio's world. This value is used to download the proper configuration of 
         space tokens for the current site from the [**AGIS**](https://twiki.cern.ch/twiki/bin/view/Atlas/MovingToRucio#n2n) 
         web page.
* agis - web page address for the **AGIS** configuration file which is a [JSON file](http://atlas-agis-api.cern.ch/request/service/query/get_se_services/?json&flavour=XROOTD).
* jsonfile - local JSON file containing the space tokens configuration. This one is tried if none of the previous
         alternatives is available.

There are also a couple of configuration values that refer to the possbile redirection points. 

* eoshost - host name of the EOS instance where we check for file existance
* eosport - port value for the EOS instance 


* uphost - host name of a higher up in the hierarchy redirector to which requests of files not in EOS are redirected
* upport - port value for the redirector instance 


