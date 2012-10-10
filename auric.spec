Summary:	Financial Data Analyser
Name:		auric
Version:	000
Release:	1.otl%{?dist}
Group:		Applications/Productivity
License:	GPLv3
Vendor:		OpenTech Labs
Packager:	Andrew Clayton <andrew@opentechlabs.co.uk>
Source0:	auric-%{version}.tar
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root

BuildRequires:	gtk3-devel cairo-devel tokyocabinet-devel
Requires:	gtk3 cairo tokyocabinet

%description


%prep
%setup -q

%build
make -C src

%install
rm -rf $RPM_BUILD_ROOT
install -Dp -m0755 src/auric $RPM_BUILD_ROOT/%{_bindir}/auric
install -Dp -m0644 auric.desktop $RPM_BUILD_ROOT/%{_datadir}/applications/auric.desktop
install -Dp -m0644 auric.png $RPM_BUILD_ROOT/%{_datadir}/pixmaps/auric.png
install -Dp -m0644 src/auric.glade $RPM_BUILD_ROOT/%{_datadir}/auric/auric.glade
install -Dp -m0644 src/auric_vid.glade $RPM_BUILD_ROOT/%{_datadir}/auric/auric_vid.glade

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_bindir}/auric
%{_datadir}/applications/auric.desktop
%{_datadir}/pixmaps/auric.png
%{_datadir}/auric/auric.glade
%{_datadir}/auric/auric_vid.glade
%doc README COPYING

%changelog
* Mon Oct 08 2012 Andrew Clayton <andrew@opentechlabs.co.uk> - 000-1.otl
- Initial release
