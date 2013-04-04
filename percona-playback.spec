# Copyright (C) 2011-2012 Percona Inc.
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2, as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranties of
# MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
# PURPOSE.  See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http://www.gnu.org/licenses/>.
 
# boost 1.41 requirement for rhel5
%define rhelver %(rpm -qf --qf '%%{version}\\n' /etc/redhat-release | sed -e 's/^\\([0-9]*\\).*/\\1/g')
%if "%rhelver" == "5"
 %define boost_req boost141-devel
 %define boost_runreq boost141-program-options
%else
 %define boost_req boost-devel
 %define boost_runreq boost-program-options
%endif

Name:           percona-playback
Version:        0.6
Release:        2%{?dist}
Summary:        A tool for replaying captured database server load

License:        GPL
URL:            http://www.percona.com/
Source:         percona-playback-%{version}.tar.gz
Group:          Applications/Databases
BuildRequires:  autoconf automake libtool libdrizzle-devel gettext-devel libpcap-devel tbb-devel mysql mysql-devel intltool %{boost_req} pkgconfig
Requires:       libdrizzle libpcap tbb mysql %{boost_runreq}
BuildRoot:	%{_tmppath}/%{name}-%{version}-build

%description
Percona Playback is a tool for replaying the captured load of one database
server against another in the most realistic way possible. Captured load can
come in the form of MySQL slow query logs or tcpdump capture.
It's multithreaded, modular and configurable to allow for flexibility and
future extension.

%package        devel
Summary:        Development files for %{name}
Group:          Applications/Databases
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description    devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.


%prep
%setup -q


%build
autoreconf -i
%configure --disable-static
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
find $RPM_BUILD_ROOT -name '*.la' -exec rm -f {} ';'


%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%doc
%{_bindir}/percona-playback
%{_libdir}/*.so.*

%files devel
%doc
%{_includedir}/*
%{_libdir}/*.so


%changelog
