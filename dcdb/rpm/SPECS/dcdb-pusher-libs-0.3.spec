Name: dcdb-pusher-libs
Summary: Required runtime libraries for dcdb-pusher
Version: 0.3
Release: 1

License: Various
URL: https://dcdb.it/
Source: https://gitlab.lrz.de/dcdb/dcdb/tree/master/dcdbpusher
Packager: DCDB project <info@dcdb.it>

%description
This is a supplimentary package for dcdb-pusher. Pusher relies on a set of
runtime libraries that are either not (yet) available on the end user system or
require custom patches. This package bundles all runtime libraries required
for Pusher to run.

More information, source code, and the full DCDB project can be found at
 https://dcdb.it

%prep
#nothing to prepare

%build
#nothing to build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_libdir}/dcdb
cd ../../../install/lib
install -p -m755 libboost_*.so.1.70.0 %{buildroot}%{_libdir}/dcdb/
install -p -m755 libcrypto.so.1.1 %{buildroot}%{_libdir}/dcdb/
install -p -m755 libfreeipmi.so.17 %{buildroot}%{_libdir}/dcdb/
#install -p -m755 libgcrypt.so* %{buildroot}%{_libdir}/dcdb/
#install -p -m755 libgpg-error.so* %{buildroot}%{_libdir}/dcdb/
install -p -m755 libmosquitto*.so.1 %{buildroot}%{_libdir}/dcdb/
install -p -m755 libnetsnmp*.so.35 %{buildroot}%{_libdir}/dcdb/
#install -p -m755 libopamgt.so* %{buildroot}%{_libdir}/dcdb/
install -p -m755 libssl.so.1.1 %{buildroot}%{_libdir}/dcdb/
#install -p -m755 libuv.so* %{buildroot}%{_libdir}/dcdb/

#-p /sbin/ldconfig deprecated since ca. Fedora28
%post -p /sbin/ldconfig

%clean
rm -rf %{buildroot}

%preun
#nothing to do pre-uninstallation

#-p /sbin/ldconfig deprecated since ca. Fedora28
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
%{_libdir}/dcdb/lib*.so*

%changelog
* Wed Dec 11 2019 Micha Mueller <micha.mueller@lrz.de> 0.3-1
- Initial release
