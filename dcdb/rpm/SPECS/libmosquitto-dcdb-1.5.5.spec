Name: libmosquitto-dcdb
Summary: Mosquitto library patched for DCDB
Group: System Environment/Libraries
Version: 1.5.5
Release: 1

License: BSD
URL: http://mosquitto.org/
Source: http://mosquitto.org/files/source/mosquitto-%{version}.tar.gz
Patch: mosquitto-%{version}.patch
Packager: DCDB project <info@dcdb.it>

Buildroot: %{_tmppath}/%{name}-%{version}-root
BuildRequires: gcc-c++
BuildRequires: openssl-devel
Conflicts: mosquitto
Conflicts: mosquitto-devel

%description
Mosquitto is an open source implementation of the MQ Telemetry Transport 
protocol version 3.1 and 3.1.1. MQTT provides a lightweight method of carrying 
out messaging using a publish/subscribe model. This package provides 
libmosquitto.so but patched the original sources for better integration with
DataCenter DataBase (DCDB).

The original sources can be found at 
 http://mosquitto.org/
For more information on the DCDB project visit 
 https://dcdb.it

%prep
%setup -q -n mosquitto-%{version}
# Set the install prefix to $RPM_BUILD_ROOT
sed -i "s|prefix?=/usr/local|prefix?=/usr|" config.mk
%patch -p1

%build
cd lib/
%if 0%{?rhel} == 7
export CFLAGS="%{optflags} -std=gnu99"
%else
export CFLAGS="%{optflags}"
%endif
export LDFLAGS="%{optflags} %{__global_ldflags} -Wl,--as-needed"
make all %{?_smp_mflags} WITH_SHARED_LIBRARIES=yes WITH_STATIC_LIBRARIES=no WITH_STRIP=no

%install
rm -rf $RPM_BUILD_ROOT
%if "%{_lib}" == "lib64"
export LIB_SUFFIX=64
%endif
cd lib/
%make_install

%post -p /sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
%{_includedir}/*.h
%{_libdir}/*.so
%{_libdir}/*.so.*
%{_libdir}/pkgconfig/*.pc

%changelog
* Thu Jul 04 2019 Micha Mueller <micha.mueller@lrz.de> 1.5.5-1
- Initial release
