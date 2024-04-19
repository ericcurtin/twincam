#!/bin/bash

set -ex

REL="$(git tag | tail -1)"
mkdir -p "$HOME/rpmbuild/SOURCES/"
git archive -o "$HOME/rpmbuild/SOURCES/twincam-$REL.tar.gz" --prefix "twincam-$REL/" HEAD
rpmbuild -bs twincam.spec

#copr-cli build @centos-automotive-sig/next $HOME/rpmbuild/SRPMS/twincam-*.src.rpm

