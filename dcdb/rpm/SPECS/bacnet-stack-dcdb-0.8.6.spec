Name: bacnet-stack-devel-dcdb
Summary: BACnet stack development files patched for DCDB
Group: Development/Libraries
Version: 0.8.6
Release: 1

License: GPL with exception
URL: http://bacnet.sourceforge.net/
Source: https://downloads.sourceforge.net/bacnet/bacnet-stack/bacnet-stack-%{version}/bacnet-stack-%{version}.tgz
Patch: bacnet-stack-%{version}.patch
Packager: DCDB project <info@dcdb.it>

Buildroot: %{_tmppath}/%{name}-%{version}-root
BuildRequires: gcc
#TODO icon, vendor and preReq?

#ExclusiveArch: x86_64
ExclusiveOS: linux

%description
The BACnet stack is an open source implementation of the BACnet protocol for embedded systems. This package provides include files and a static library of the originial BACnet stack but patched the original sources for better integration with DataCenter DataBase (DCDB). 

The original sources can be found at 
 http://bacnet.sourceforge.net/
For more information on the DCDB project visit 
 https://dcdb.it

%global debug_package %{nil}

%prep
%setup -q -n bacnet-stack-%{version}
%patch -p1

%build
BACNET_PORT=linux MAKE_DEFINE=-fpic make library

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_libdir}
install -m644 include/*.h -D -t $RPM_BUILD_ROOT%{_includedir}/bacnet
install -m755 lib/libbacnet.a -D -t $RPM_BUILD_ROOT%{_libdir}

%post -p /sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
%{_includedir}/bacnet/*.h
%{_libdir}/libbacnet.a

%changelog
* Wed Jul 03 2019 Micha Mueller <micha.mueller@lrz.de> 0.8.6-1
- Initial release
