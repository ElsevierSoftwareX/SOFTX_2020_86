#!/bin/bash

# set PATH and LD_LIBRARY_PATH for CUDA/OpenCL installation (may need to be adjusted)
#export PATH=$PATH:/usr/local/cuda/bin
#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/cuda/lib:/usr/local/cuda/lib64

for currDir in *
do
    echo $currDir
    if [ -d $currDir ]
	then
		cd $currDir
		pwd
		if [ "$1" == "clean" ]; then
			make clean
		elif [ "$1" != "test" ]; then
			make
		fi
		cd ..
    fi
	if [ "$1" == "test" ]; then
		clear;
		cd 2DCONV
		./2DConvolution ;
		cd ..
		cd 2MM
		./2mm ;
		cd ..
		cd 3DCONV
		./3DConvolution ;
		cd ..
		cd 3MM
		./3mm ;
		cd ..
		cd ATAX
		./atax ;
		cd ..
		cd BICG
		./bicg ;
		cd ..
		cd CORR
		./correlation ; 
		cd ..
		cd COVAR
		./covariance ;
		cd ..
		cd FDTD-2D
		./fdtd2d ;
		cd ..
		cd GEMM
		./gemm ;
		cd ..
		cd GESUMMV
		./gesummv ;
		cd ..
		cd GRAMSCHM
		./gramschm ;
		cd ..
		cd MVT
		./mvt ; 
		cd ..
		cd SYR2K
		./syr2k ;
		cd ..
		cd SYRK 
		./syrk ;
		cd ..
	fi
done
