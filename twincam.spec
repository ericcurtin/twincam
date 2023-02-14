%global dracutdir %(pkg-config --variable=dracutdir dracut)

Name:          twincam
Version:       0.6
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
BuildRequires: libjpeg-devel
BuildRequires: systemd

Conflicts: plymouth

# twincam is not expected to be used in these architectures
ExcludeArch: s390x ppc64le %ix86

%description
%{summary}.

%prep
%autosetup -p1

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
install -m644 lib/systemd/system/twincam-quit.service $RPM_BUILD_ROOT%{_unitdir}/
install -m644 lib/systemd/system/twincam.service $RPM_BUILD_ROOT%{_unitdir}/
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
* Tue Feb 14 2023 Eric Curtin <ecurtin@redhat.com> - 0.6-3
- Exclude more CPU architectures

* Tue Feb 14 2023 Eric Curtin <ecurtin@redhat.com> - 0.6-2
- .spec file changes

* Tue Feb 14 2023 Eric Curtin <ecurtin@redhat.com> - 0.6-1
- Some libcamera and gcc compatibility fixes

* Sat Jan 21 2023 Fedora Release Engineering <releng@fedoraproject.org> - 0.5.4-7
- Rebuilt for https://fedoraproject.org/wiki/Fedora_38_Mass_Rebuild

* Sun Jan 01 2023 Mamoru TASAKA <mtasaka@fedoraproject.org> - 0.5.4-6
- Rebuild for new libcamera again
- Patch for libcamera 0.0.3 API change

* Wed Dec 07 2022 Mamoru TASAKA <mtasaka@fedoraproject.org> - 0.5.4-5
- Rebuild for new libcamera

* Tue Aug 23 2022 Eric Curtin <ecurtin@redhat.com> - 0.5-4
- Remove dri dependancies

* Mon Aug 22 2022 Eric Curtin <ecurtin@redhat.com> - 0.5-3
- Exclude architecture ix86

* Thu Aug 18 2022 Eric Curtin <ecurtin@redhat.com> - 0.5-2
- Remove wildcard from spec file

* Thu Aug 18 2022 Eric Curtin <ecurtin@redhat.com> - 0.5-1
- Don't show cursor

* Fri Jul 29 2022 Ian Mullins <imullins@redhat.com> - 0.4-1
- Use libjpeg over SDL2_image 
- Use MJPEG by default if compiled with libjpeg
- Add install for no initrd

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

