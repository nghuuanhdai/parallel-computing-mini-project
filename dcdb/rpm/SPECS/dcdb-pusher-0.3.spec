Name: dcdb-pusher
Summary: Pusher component of the DCDB project
Version: 0.3
Release: 1

License: GPLv2+
URL: https://dcdb.it/
Source: https://gitlab.lrz.de/dcdb/dcdb/tree/master/dcdbpusher
Packager: DCDB project <info@dcdb.it>

Requires: dcdb-pusher-libs

BuildRequires: systemd-rpm-macros
BuildRequires: sed

%description
Pusher is the data acquisition component for the DataCenter Data Base (DCDB)
project. DCDB is a holistic monitoring solution for HPC environments. This
package bundles the Pusher framework binary as well as the following plugin
libraries: sysfs, perfevent, ipmi, pdu, bacnet, snmp, procfs, tester, gpfsmon,
opa, msr.

More information, source code, and the full DCDB project can be found at
 https://dcdb.it

%prep
#nothing to prepare

%build
#nothing to build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_libdir}/dcdb
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_unitdir}
mkdir -p %{buildroot}%{_sysconfdir}/dcdb
cd ../../dcdbpusher
#does not install to correct paths. Install manually instead
#make DCDBDEPLOYPATH=%{buildroot} install
#make DCDBDEPLOYPATH=%{buildroot} install_conf
install -p -m 755 dcdbpusher %{buildroot}%{_bindir}
install -p -m 755 libdcdbplugin_*.so %{buildroot}%{_libdir}/dcdb/
install -p -m 644 config/* %{buildroot}%{_sysconfdir}/dcdb/
#custom systemd service file and pusher config as the ones provided in the repo
#had to be adapted for package install paths
install -p -m 644 %{_sourcedir}/dcdbpusher.conf %{buildroot}%{_sysconfdir}/dcdb/
install -p -m 644 %{_sourcedir}/pusher.service %{buildroot}%{_unitdir}

%post
%systemd_post pusher.service

%clean
rm -rf %{buildroot}

%preun
%systemd_preun pusher.service

%postun
%systemd_postun_with_restart pusher.service

%files
%defattr(-,root,root)
%{_libdir}/dcdb/*.so
%{_bindir}/dcdbpusher
%{_unitdir}/pusher.service
%{_sysconfdir}/dcdb/*.conf

%changelog
* Tue Dec 10 2019 Micha Mueller <micha.mueller@lrz.de> 0.3-1
- Initial release
