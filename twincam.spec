Name:           twincam
Version:        0.2
Release:        1%{?dist}
Summary:        A lightweight camera application

License:        GPLv2
URL:            https://github.com/ericcurtin/twincam
Source0:        %{url}/archive/%{version}/%{name}-%{version}.tar.gz

BuildRequires:  meson
BuildRequires:  gcc-g++
BuildRequires:  ninja-build
BuildRequires:  libevent-devel
BuildRequires:  libcamera-devel
BuildRequires:  libdrm-devel

%description
A lightweight camera application

%prep
%autosetup

%build
%meson
%meson_build

%check
%meson_test

%install
%meson_install

%pre
exit 0

%files
%license COPYING
%doc README.md
%{_bindir}/twincam

%changelog
* Wed Feb  9 2022 Eric Curtin <ecurtin@redhat.com> - 0.2-1
- Update to new upstream release
- Fixes for building on 32 bit platforms

* Thu Jan 27 2022 Eric Curtin <ecurtin@redhat.com> - 0.1-1
- Initial version

