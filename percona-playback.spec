Name:           percona-playback
Version:        0.4
Release:        1%{?dist}
Summary:        A tool for replaying captured database server load

License:        GPL
URL:            http://www.percona.com/
Source:         percona-playback-%{version}.tar.gz

BuildRequires:  autoconf automake libtool libdrizzle-devel gettext-devel libpcap-devel tbb-devel mysql-devel intltool boost-devel pkgconfig
Requires:       libdrizzle libpcap tbb mysql boost-program-options

%description
Percona Playback is a tool for replaying the captured load of one database
server against another in the most realistic way possible. Captured load can
come in the form of MySQL slow query logs or tcpdump capture.
It's multithreaded, modular and configurable to allow for flexibility and
future extension.

%package        devel
Summary:        Development files for %{name}
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
