Name:           twincam
Version:        0.2
Release:        3%{?dist}
Summary:        A lightweight camera application

License:        GPLv2
URL:            https://github.com/ericcurtin/twincam
Source0:        %{url}/archive/%{version}/%{name}-%{version}.tar.gz

BuildRequires:  meson
BuildRequires:  gcc-g++
BuildRequires:  ninja-build
BuildRequires:  pkgconfig(libevent_pthreads)
BuildRequires:  pkgconfig(libcamera)
BuildRequires:  pkgconfig(libdrm)

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

