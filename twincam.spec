%global dracutdir %(pkg-config --variable=dracutdir dracut)

Name:          twincam
Version:       0.3
Release:       3%{?dist}
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
BuildRequires: systemd

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

mkdir -p $RPM_BUILD_ROOT%{dracutdir}/modules.d/81twincam
mkdir -p $RPM_BUILD_ROOT%{_unitdir}/sysinit.target.wants
mkdir -p $RPM_BUILD_ROOT%{_unitdir}/multi-user.target.wants
install -m644 lib/dracut/modules.d/81twincam/module-setup.sh $RPM_BUILD_ROOT%{dracutdir}/modules.d/81twincam/
install -m644 lib/systemd/system/*.service $RPM_BUILD_ROOT%{_unitdir}/
ln -fs ../twincam.service $RPM_BUILD_ROOT%{_unitdir}/sysinit.target.wants/
ln -fs ../twincam-quit.service $RPM_BUILD_ROOT%{_unitdir}/multi-user.target.wants/

%post
dracut -f

%files
%license COPYING
%doc README.md
%{_bindir}/twincam
%{dracutdir}/modules.d/81twincam/module-setup.sh
%{_unitdir}/twincam.service
%{_unitdir}/twincam-quit.service
%{_unitdir}/sysinit.target.wants/twincam.service
%{_unitdir}/multi-user.target.wants/twincam-quit.service

%changelog
* Wed Jun 29 2022 Eric Curtin <ecurtin@redhat.com> - 0.3-3
- Fix days
- Add systemd

* Wed Jun 29 2022 Eric Curtin <ecurtin@redhat.com> - 0.3-2
- Add some more files
- Run dracut after the files are in place

* Tue Jun 28 2022 Eric Curtin <ecurtin@redhat.com> - 0.3-1
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

