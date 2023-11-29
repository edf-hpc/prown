Name:     prown
Version:  4.1
Release:  1%{?dist}.edf
Summary:  Prown is a simple tool to give users the possibility to own projects. 

License:  GPL-3.0+
Source0:  %{name}-%{version}.tar.gz

BuildRequires: libbsd-devel libacl-devel pandoc gettext

%description
Prown is a simple tool developed to give users the possibility to own projects. 
It uses the configuration file in /etc/prown.conf to specify the projects directory. 
When a user specify ane or multiple directories, Prown verify the user permissions 
and chage recursively the owner of the directory to that user.

%global debug_package %{nil}

%prep
%setup -q

%build
%set_build_flags
make all

%install
make install DESTDIR=%{buildroot} prefix=/usr
install -D -m644 -p conf/prown.conf %{buildroot}%{_sysconfdir}/prown.conf

%clean
rm -rf %{buildroot}

%files
%doc GPL-3.txt
%attr(0644,root,root) %config(noreplace) %{_sysconfdir}/prown.conf
%attr(0755,root,root) %caps(cap_chown=ep) %{_bindir}/prown
%{_datadir}

%changelog
* Fri Nov 24 2023 Kwame Amedodji <kwame-externe.amedodji@edf.fr> - 4.1-1.el8.edf
- New upstream release 4.1
* Tue Dec 22 2021 Rémi Palancher <remi-externe.palancher@edf.fr> - 4.0-1.el8.edf
- New upstream release 4.0
* Tue Jun 22 2021 Thomas HAMEL <thomas-t.hamel@edf.fr> - 3.8-2.el8.edf
- Set config file as noreplace
* Thu May 06 2021 Thomas <thomas-t.hamel@edf.fr> - 3.8-1.el8.edf
- Bump to 3.8
* Thu May 06 2021 Mouad Bahi <mouad-externe.bahi@edf.fr> - 3.7-2
- add config macro
* Thu May 06 2021 Mouad Bahi <mouad-externe.bahi@edf.fr> - 3.7-1
- fix bug in symbolic link handling
- align code lines
* Mon Mar 15 2021 Tàzio Gennuso <tazio-externe.gennuso@edf.fr> - 3.6-2
- remove suid from binary
- add CAP_CHOWN capability to binary
* Fri Feb 26 2021 Tàzio Gennuso <tazio-externe.gennuso@edf.fr> - 3.6-1
- Initial el8 release
