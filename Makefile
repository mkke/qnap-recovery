all:

%.tgz: %.img pc1
	./pc1 d QNAPNASVERSION4 $< $@

build-dep:
	apt install libjson-pp-perl kpartx

