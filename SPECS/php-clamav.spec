%global php_apiver  %((echo 0; php -i 2>/dev/null | sed -n 's/^PHP API => //p') | tail -1)
%global php_extdir  %(php-config --extension-dir 2>/dev/null || echo "undefined")
%global php_version %(php-config --version 2>/dev/null || echo 0)

%define        php_libname clamav
%define	       php_confdir %{_sysconfdir}/php.d 

Name:          php-clamav
Version:       0.15.8
Release:       1%{?dist}
Summary:       ClamAV Interface for PHP scripts
Group:         Development/Libraries
License:       GPLv2+
URL:           http://sourceforge.net/projects/php-clamav/
Source0:       http://sourceforge.net/projects/php-clamav/files/php-clamav_%{version}.tar.gz
BuildRoot:     %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires: clamav-devel >= 0.95
BuildRequires: php-devel >= 5
Requires:      openssl
Requires:      php >= 5
Requires:      clamav >= 0.95
Conflicts:     php5-clamav
Conflicts:     php5-clamavlib

%description
PHP-ClamAV is a PHP extension that allow to incorporate virus scanning
features on your PHP scripts. It uses the ClamAV API for virus scanning.
All functions provides by this package are documented.

%prep
%setup -q php-%{php_libname}-%{version}

%build
%{_bindir}/phpize --clean
%{_bindir}/phpize
%configure --with-clamav --with-php-config=%{_bindir}/php-config
%{__make} %{?_smp_mflags}

%install
%{__rm} -rf %{buildroot}
%{__make} install INSTALL_ROOT=%{buildroot}
%{__mkdir_p} %{buildroot}%{php_confdir}
%{__cat} > %{buildroot}%{php_confdir}/%{php_libname}.ini << 'EOF'
extension=clamav.so
[clamav]
clamav.dbpath="/var/lib/clamav"
clamav.maxreclevel=16
clamav.maxfiles=10000
clamav.maxfilesize=26214400
clamav.maxscansize=104857600
clamav.keeptmp=0
clamav.load_db_on_startup=0
clamav.tmpdir="/tmp"
EOF

%check
%{_bindir}/php --no-php-ini \
    --define extension_dir=modules \
    --define extension=%{php_libname}.so \
    --modules | grep %{php_libname}

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-,root,root,-)
%doc README CREDITS phpclamav_test.php
%config(noreplace) %{php_confdir}/%{php_libname}.ini
%{php_extdir}/%{php_libname}.so

%changelog
* Thu Aug 05 2014 Argos <argos66@gmail.com> - 0.15.8-1
- Improve database reload during cl_scanfile.
- Check compatibility with php 5.5..
