Summary:	Financial Data Analyser
Name:		auric
Version:	002
Release:	1%{?dist}
Group:		Applications/Productivity
License:	GPLv3
URL:		https://github.com/ac000/auric
Packager:	Andrew Clayton <andrew@digital-domain.net>
Source0:	auric-%{version}.tar
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root

BuildRequires:	gtk3-devel cairo-devel tokyocabinet-devel
Requires:	gtk3 cairo tokyocabinet

%description


%prep
%setup -q

%build
make -C src
make -C docs/guide

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
%doc README COPYING docs/tmpl.tab docs/guide/guide.pdf

%changelog
* Fri Jan 24 2014 Andrew Clayton <andrew@digital-domain.net> - 002-1
- Version 002

* Thu Oct 18 2012 Andrew Clayton <andrew@opentechlabs.co.uk> - 001-1.otl
- Initial release
