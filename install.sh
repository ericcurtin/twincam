#!/bin/bash

set -e

for i in $(find lib -type f | xargs dirname); do
  mkdir -p /usr/$i
done

cp lib/dracut/modules.d/81twincam/module-setup.sh /usr/lib/dracut/modules.d/81twincam/
cp lib/systemd/system/twincam.service /usr/lib/systemd/system/
ln -fs ../twincam.service /usr/lib/systemd/system/sysinit.target.wants/
dracut -f

