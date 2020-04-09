# doesn't seem to work on sles 12.3: %%{!?make_build:%%define make_build %%{__make} %%{?_smp_mflags}}
# so...
%if 0%{?suse_version} <= 1320
%define make_build  %{__make} %{?_smp_mflags}
%endif

%bcond_with use_release

Name:		raft
Version:	0.5.0
Release:	3%{?relval}%{?dist}

Summary:	C implementation of the Raft Consensus protocol, BSD licensed

License:	BSD-3-Clause
URL:		https://github.com/daos-stack/%{name}
Source0:	https://github.com/daos-stack/%{name}/releases/download/%{shortcommit0}/%{name}-%{version}.tar.gz

%if 0%{?suse_version} >= 1315
Group:		Development/Libraries/C and C++
%endif

%description
Raft is a consensus algorithm that is designed to be easy to understand.
It's equivalent to Paxos in fault-tolerance and performance. The difference
is that it's decomposed into relatively independent subproblems, and it
cleanly addresses all major pieces needed for practical systems.

%package devel
Summary:	Development libs

%description devel
Development libs for Raft consensus protocol

%prep
%setup -q

%build
# only build the static lib
%make_build static

%install
mkdir -p %{buildroot}/%{_libdir}
cp -a libraft.a %{buildroot}/%{_libdir}
mkdir -p %{buildroot}/%{_includedir}
cp -a include/* %{buildroot}/%{_includedir}

%files
%defattr(-,root,root,-)
%doc README.rst
%license LICENSE

%files devel
%defattr(-,root,root,-)
%{_libdir}/*
%{_includedir}/*


%changelog
* Thu Apr 09 2020 Brian J. Murrell <brian.murrell@intel> -0.5.0-3
- Only build the static library

* Fri Oct 04 2019 John E. Malmberg <john.e.malmberg@intel> -0.5.0-2
- SUSE rpmlint fixups

* Mon Apr 08 2019 Brian J. Murrell <brian.murrell@intel> -0.5.0-1
- initial package