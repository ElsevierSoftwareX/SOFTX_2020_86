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
done

if [ "$1" == "clean" ]; then
	echo "Cleaning vkcomp obj files..."
	cd ./vkcomp
	rm -f *.o
	cd ..
fi

DEVICE_SELECTION="0"

if [ "$1" == "test" ]; then
	clear;
	cd 2DCONV
	echo "Performing 2DConvolution"
	./2DConvolution $DEVICE_SELECTION ;
	cd ..
	cd 2MM
	echo "Performing 2MM. Warning: CPU implementation will take a while..."
	./2mm $DEVICE_SELECTION ;
	cd ..
	cd 3DCONV
	echo "Performing 3DConvolution"
	./3DConvolution $DEVICE_SELECTION ;
	cd ..
	cd 3MM
	echo "Performing 3MM" 
	./3mm $DEVICE_SELECTION ;
	cd ..
	cd ATAX
	echo "Performing ATAX"
	./atax $DEVICE_SELECTION ;
	cd ..
	cd BICG
	echo "Performing BICG"
	./bicg $DEVICE_SELECTION ;
	cd ..
	cd CORR
	echo "Performing CORR"
	./correlation $DEVICE_SELECTION ; 
	cd ..
	cd COVAR
	echo "Performing covariance"
	./covariance $DEVICE_SELECTION ;
	cd ..
	cd FDTD-2D
	echo "Performing FDTD-2D"
	./fdtd2d $DEVICE_SELECTION ;
	cd ..
	cd GEMM
	echo "Performing GEMM"
	./gemm $DEVICE_SELECTION ;
	cd ..
	cd GESUMMV
	echo "Performing GESUMMV"
	./gesummv $DEVICE_SELECTION ;
	cd ..
	cd GRAMSCHM
	echo "Performing GRAMSCHM"
	./gramschmidt $DEVICE_SELECTION ;
	cd ..
	cd MVT
	echo "Performing MVT"
	./mvt $DEVICE_SELECTION ; 
	cd ..
	cd SYR2K
	echo "Performing SYR2K"
	./syr2k $DEVICE_SELECTION ;
	cd ..
	cd SYRK 
	echo "Performing SYRK"
	./syrk $DEVICE_SELECTION ;
	cd ..
fi
