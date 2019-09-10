Overview
========

This data recovery method uses a host machine running Linux to assemble the
md raid and a guest VM running the QNAP firmware to access the LVM volumes.

A running QNAP device is not necessary.

Preparing the disk array
========================

* the disk array should be visible on the device-mapper level, either
  automatically after boot, or after mdadm -A -R --scan

* test the md block device is accessible and set it read-only.
  We are only interested in the data device, which should be `/dev/md1`.

``` bash
mdadm --detail /dev/md1
mdadm --readonly /dev/md1
```

Preparing a VM with an up-to-date firmware
==========================================

* Download x86 FW from
  `https://wiki.qnap.com/wiki/NAS_Firmware_Update_When_No_HDD(s)_Installed`,
  e.g. `F_TS-X53S_20150106-1.2.8.img`

* Download FW update from qnap.com (e.g. `TS-X53S_20190704-4.3.6.0993.img`)
  and unzip

* decrypt the update

``` bash
make TS-X53S_20190704-4.3.6.0993.tgz
```

* create a patched firmware

``` bash
./patch-firmware F_TS-X53S_20150106-1.2.8.img TS-X53S_20190704-4.3.6.0993.tgz
```

* copy the firmware to /var/lib/libvirt/images

* update the apparmor profile to allow access, e.g.:

``` bash
echo '/dev/md1 r,' >> /etc/apparmor.d/local/usr.lib.libvirt.virt-aa-helper
systemctl reload apparmor
```

* create new VM:

  * add FW image as disk
  * add `/dev/md1` as VirtIO disk device to the VM, then set it as read-only
  * display: VGA
  * remove the Spice GPU

* start the VM

* display output will stop after 'Booting the kernel...', instead we have to
  use the serial console
  (either with virt-manager -> View -> Text Consoles -> Serial, or
   with `virsh console <vm name>`)

* login with admin / admin

* run the patched `/etc/init.d/init_check.sh` script

* mount the device

``` bash
# link to /dev/md1, so that pvscan recognizes it
ln /dev/vda /dev/md1

pvscan --cache /dev/md1

# pvs should now show the volume group, and lvs the volumes
pvs
lvs

# activate the thin pool and the volume
lvchange -a y vg1/lv2

# check the content, if it contains a LUKS header -> encrypted
dd if=/dev/mapper/vg1-lv2 bs=1 count=256|hexdump -C
cryptsetup isLuks /dev/mapper/vg1-lv2 -v

# open luks container with web password
/bin/cryptsetup-qnap vg1-lv2 <password>

# mount the unencrypted device
# because we are read-only, a dirty ext4 can only be mounted with '-o ro,noload'
# for a complete journal replay, switch to read-write mode, or to be safe
# copy the raw block device to the host and replay it on the copy
mount -t ext4 /dev/mapper/vg1-lv4 /mnt/ext/

# get an IP address for file transfer
dhclient -i eth0
```

Copying newer files
===================

The qnap userland is quite simple, so we have to be creative:

``` bash
# create a file with a suitable mtime
touch -t 201904100000 /tmp/mark

# find newer files
find . -type t -newer /tmp/mark > /tmp/filelist

# copy them with rsync
rsync -avx --progress --files-from=/tmp/filelist . mkke@1.2.3.4:/my-data
```

Firmware Update Decryption
==========================

The original links for the decryption tool source code and presentation
(pc1.c, pc2.c, insomnihack.pdf) by Paul Rascagneres <rootbsd@r00ted.com>
has disappeared, I got my copy from
<https://sites.google.com/site/nliaudat/nas/test2/qnap401t-decryptencryptfirmware>.

If you would rather decrypt from inside the QNAP firmware:

``` bash
# create new sparse disk image file
dd if=/dev/zero of=TS-X53S-2.img bs=1 count=0 seek=16G

# partition it
/sbin/sfdisk -q TS-X53S-2.img <<EOF
,,b,*
EOF

# make partition visible as device
kpartx -a TS-X53S-2.img

# format it
/sbin/mkdosfs -F 32 /dev/mapper/loop0p1

# mount it
mount /dev/mapper/loop0p1 /mnt

# copy update
cp TS-X53S_20190704-4.3.6.0993.img /mnt/

# unmount and remove loop device
umount /mnt
kpartx -d TS-X53S-2.img

# convert to qcow2
qemu-img convert -O qcow2 TS-X53S-2.img TS-X53S-2.qcow
```

* copy original FW and update image to /var/lib/libvirt

* create new VM:

  * add FW image as disk
  * add FW update image as second (USB) disk
  * display: VGA

* start the VM

* wait after booting a few seconds to a minute, the login prompt will come...

* login with admin / admin

``` bash
mkdir /mnt/HDA_ROOT/update
ln -sf /mnt/HDA_ROOT/update /mnt/update
mount /dev/sdb1 /mnt/ext
cd /mnt/ext
/sbin/PC1 d QNAPNASVERSION4 TS-X53S_20190704-4.3.6.0993.img TS-X53S_20190704-4.3.6.0993.tgz
tar xzf TS-X53S_20190704-4.3.6.0993.tgz -C /mnt/update
```

* shut the VM down, mount the image on the host and copy the tgz


