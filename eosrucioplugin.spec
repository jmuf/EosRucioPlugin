%define _unpackaged_files_terminate_build 0
Name:		eosrucioplugin
Version:	0.1.0
Release:	0
Summary:	Rucio name plugin for EOS
Prefix:         /usr
Group:		Applications/File
License:	GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Source:        %{name}-%{version}-%{release}.tar.gz
BuildRoot:     %{_tmppath}/%{name}-root

BuildRequires: cmake >= 2.6
BuildRequires: curl
BuildRequires: openssl, openssl-devel
#BuildRequires: xrootd-server-devel >= 3.3.0
#BuildRequires: xrootd-private-devel >= 3.3.0

%description
Rucio plugin for EOS for doing name translations

%prep
%setup -n %{name}-%{version}-%{release}

%build
test -e $RPM_BUILD_ROOT && rm -r $RPM_BUILD_ROOT
%if 0%{?rhel} < 6
export CC=/usr/bin/gcc44 CXX=/usr/bin/g++44 
%endif

mkdir -p build
cd build
cmake ../ -DRELEASE=%{release} -DCMAKE_BUILD_TYPE=RelWithDebInfo
%{__make} %{_smp_mflags} 

%install
cd build
%{__make} install DESTDIR=$RPM_BUILD_ROOT
echo "Installed!"

%post 
/sbin/ldconfig

%postun
/sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
/usr/lib64/libEosRucioOfs.so
/usr/lib64/libEosRucioCms.so
%config(noreplace) /etc/xrd.cf.rucio.example
%config(noreplace) /etc/xrd.cf.fed.example

