# SPDX-License-Identifier: MIT
# Copyright (c) 2023 University of Washington

define gdrive
$(eval $1-url = $2)
$(eval get-from-gdrive += $1.mtx)
endef

define uflorida
$(eval $1-url = $2)
$(eval get-from-uflorida += $1.mtx)
endef

define cswashington
$(eval $1-url = $2)
$(eval get-from-cswashington += $1.mtx)
endef

###################################################
# list of mtx files to download from google drive #
###################################################
get-from-gdrive :=

.PHONY: gdrive
gdrive: $(get-from-gdrive)
$(get-from-gdrive): %.mtx:
	$(eval tarfile=$@.tar.gz)
	$(eval tarurl=$($*-url))
	gdown --fuzzy $(tarurl)
	tar zxf $(tarfile)
	rm -f $(tarfile)

all: $(get-from-gdrive)

#######################################################
# list of mtx files to download from Univ. of Florida #
#######################################################
get-from-uflorida :=
# misc
$(call uflorida,bcsstk15,https://suitesparse-collection-website.herokuapp.com/MM/HB/bcsstk15.tar.gz)
$(call uflorida,offshore,https://suitesparse-collection-website.herokuapp.com/MM/Um/offshore.tar.gz)
# networks
$(call uflorida,wiki-Vote,https://suitesparse-collection-website.herokuapp.com/MM/SNAP/wiki-Vote.tar.gz)
$(call uflorida,email-Enron,https://suitesparse-collection-website.herokuapp.com/MM/SNAP/email-Enron.tar.gz)
$(call uflorida,web-NotreDame,https://suitesparse-collection-website.herokuapp.com/MM/SNAP/web-NotreDame.tar.gz)
$(call uflorida,web-Stanford,https://suitesparse-collection-website.herokuapp.com/MM/SNAP/web-Stanford.tar.gz)
$(call uflorida,web-Google,https://suitesparse-collection-website.herokuapp.com/MM/SNAP/web-Google.tar.gz)
$(call uflorida,web-BerkStan,https://suitesparse-collection-website.herokuapp.com/MM/SNAP/web-BerkStan.tar.gz)
# roads
$(call uflorida,minnesota,https://suitesparse-collection-website.herokuapp.com/MM/Gleich/minnesota.tar.gz)
$(call uflorida,roadNet-CA,https://suitesparse-collection-website.herokuapp.com/MM/SNAP/roadNet-CA.tar.gz)
$(call uflorida,roadNet-PA,https://suitesparse-collection-website.herokuapp.com/MM/SNAP/roadNet-PA.tar.gz)
$(call uflorida,roadNet-TX,https://suitesparse-collection-website.herokuapp.com/MM/SNAP/roadNet-TX.tar.gz)
$(call uflorida,road_central,https://suitesparse-collection-website.herokuapp.com/MM/DIMACS10/road_central.tar.gz)
$(call uflorida,road_usa,https://suitesparse-collection-website.herokuapp.com/MM/DIMACS10/road_usa.tar.gz)
$(call uflorida,europe_osm,https://suitesparse-collection-website.herokuapp.com/MM/DIMACS10/europe_osm.tar.gz)

.PHONY: uflorida
uflorida: $(get-from-uflorida)
$(get-from-uflorida): %.mtx:
	$(eval tarurl=$($*-url))
	$(eval tarfile=$*.tar.gz)
	wget $(tarurl)
	tar zxf $(tarfile)
	ln -s $*/$@ $@
	rm -f $(tarfile)

all: $(get-from-uflorida)

####################################################
# list of mtx files to download from CS Washington #
####################################################
get-from-cswashington :=
$(call cswashington,g7k1,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g7k1.mtx.tar.gz)
$(call cswashington,g10k4,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g10k4.mtx.tar.gz)
$(call cswashington,g10k8,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g10k8.mtx.tar.gz)
$(call cswashington,g10k16,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g10k16.mtx.tar.gz)
$(call cswashington,g11k4,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g11k4.mtx.tar.gz)
$(call cswashington,g11k8,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g11k8.mtx.tar.gz)
$(call cswashington,g11k16,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g11k16.mtx.tar.gz)
$(call cswashington,g12k4,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g12k4.mtx.tar.gz)
$(call cswashington,g12k8,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g12k8.mtx.tar.gz)
$(call cswashington,g12k16,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g12k16.mtx.tar.gz)
$(call cswashington,g13k4,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g13k4.mtx.tar.gz)
$(call cswashington,g13k8,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g13k8.mtx.tar.gz)
$(call cswashington,g13k16,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g13k16.mtx.tar.gz)
$(call cswashington,g14k4,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g14k4.mtx.tar.gz)
$(call cswashington,g14k8,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g14k8.mtx.tar.gz)
$(call cswashington,g14k16,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g14k16.mtx.tar.gz)
$(call cswashington,g15k4,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g15k4.mtx.tar.gz)
$(call cswashington,g15k8,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g15k8.mtx.tar.gz)
$(call cswashington,g15k16,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g15k16.mtx.tar.gz)
$(call cswashington,g16k4,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g16k4.mtx.tar.gz)
$(call cswashington,g16k8,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g16k8.mtx.tar.gz)
$(call cswashington,g16k16,https://homes.cs.washington.edu/~mrutt/sparse_inputs/g16k16.mtx.tar.gz)
$(call cswashington,u7k1,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u7k1.mtx.tar.gz)
$(call cswashington,u10k4,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u10k4.mtx.tar.gz)
$(call cswashington,u10k8,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u10k8.mtx.tar.gz)
$(call cswashington,u10k16,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u10k16.mtx.tar.gz)
$(call cswashington,u11k4,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u11k4.mtx.tar.gz)
$(call cswashington,u11k8,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u11k8.mtx.tar.gz)
$(call cswashington,u11k16,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u11k16.mtx.tar.gz)
$(call cswashington,u12k4,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u12k4.mtx.tar.gz)
$(call cswashington,u12k8,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u12k8.mtx.tar.gz)
$(call cswashington,u12k16,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u12k16.mtx.tar.gz)
$(call cswashington,u13k4,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u13k4.mtx.tar.gz)
$(call cswashington,u13k8,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u13k8.mtx.tar.gz)
$(call cswashington,u13k16,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u13k16.mtx.tar.gz)
$(call cswashington,u14k4,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u14k4.mtx.tar.gz)
$(call cswashington,u14k8,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u14k8.mtx.tar.gz)
$(call cswashington,u14k16,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u14k16.mtx.tar.gz)
$(call cswashington,u15k4,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u15k4.mtx.tar.gz)
$(call cswashington,u15k8,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u15k8.mtx.tar.gz)
$(call cswashington,u15k16,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u15k16.mtx.tar.gz)
$(call cswashington,u16k4,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u16k4.mtx.tar.gz)
$(call cswashington,u16k8,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u16k8.mtx.tar.gz)
$(call cswashington,u16k16,https://homes.cs.washington.edu/~mrutt/sparse_inputs/u16k16.mtx.tar.gz)

.PHONY: cswashington
cswashington: $(get-from-cswashington)
$(get-from-cswashington): %.mtx:
	$(eval tarurl=$($*-url))
	$(eval tarfile=$*.mtx.tar.gz)
	wget $(tarurl)
	tar zxf $(tarfile)
	rm -f $(tarfile)

all: $(get-from-cswashington)

##################################
# cleanup downloaded input files #
##################################
clean:
	rm -rf */ *.mtx *.tar.gz
