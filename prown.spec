Name:     prown
Version:  3.7
Release:  1%{?dist}.edf
Summary:  Prown is a simple tool to give users the possibility to own projects. 

License:  GPL-3.0+
Source0:  %{name}-%{version}.tar.gz

BuildRequires: libbsd-devel

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
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_sysconfdir}

install -cp src/prown       %{buildroot}%{_bindir}
install -cp conf/prown.conf %{buildroot}%{_sysconfdir}

%clean
rm -rf %{buildroot}

%files
%doc GPL-3.txt
%attr(0644,root,root) %config %{_sysconfdir}/prown.conf
%attr(0755,root,root) %caps(cap_chown=ep) %{_bindir}/prown

%changelog
* Thu May 06 2021 Mouad Bahi <mouad-externe.bahi@edf.fr> - 3.7-1
- fix bug in symbolic link handling
- align code lines
* Mon Mar 15 2021 Tàzio Gennuso <tazio-externe.gennuso@edf.fr> - 3.6-2
- remove suid from binary
- add CAP_CHOWN capability to binary
* Fri Feb 26 2021 Tàzio Gennuso <tazio-externe.gennuso@edf.fr> - 3.6-1
- Initial el8 release
