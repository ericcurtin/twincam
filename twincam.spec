Name:          twincam
Version:       0.3
Release:       1%{?dist}
Summary:       A lightweight camera application

License:       GPLv2
URL:           https://github.com/ericcurtin/twincam
Source0:       %{url}/archive/%{version}/%{name}-%{version}.tar.gz

BuildRequires: meson
BuildRequires: gcc-c++
BuildRequires: ninja-build
BuildRequires: pkgconfig(libevent_pthreads)
BuildRequires: pkgconfig(libcamera)
BuildRequires: pkgconfig(libdrm)
BuildRequires: SDL2-devel
BuildRequires: SDL2_image-devel

Conflicts: plymouth

%description
%{summary}.

%prep
%autosetup

%build
%meson
%meson_build

%check
%meson_test

%install
%meson_install

%files
%license COPYING
%doc README.md
%{_bindir}/twincam

%changelog
* Thu Feb 10 2022 Eric Curtin <ecurtin@redhat.com> - 0.3-1
- Add SDL dependancies to ensure we will work on wide variety of hardware
- Add more systemd files to ensure we start and stop correctly on boot

* Thu Feb 10 2022 Eric Curtin <ecurtin@redhat.com> - 0.2-4
- Changes after review
- Change gcc-g++ to gcc-c++

* Wed Feb  9 2022 Eric Curtin <ecurtin@redhat.com> - 0.2-3
- Changes after review
- Remove section that does nothing

* Wed Feb  9 2022 Eric Curtin <ecurtin@redhat.com> - 0.2-2
- Changes after review
- Switch to pkgconfig names
- Summary and description are indentical, so simplify

* Wed Feb  9 2022 Eric Curtin <ecurtin@redhat.com> - 0.2-1
- Update to new upstream release
- Fixes for building on 32 bit platforms

* Thu Jan 27 2022 Eric Curtin <ecurtin@redhat.com> - 0.1-1
- Initial version

