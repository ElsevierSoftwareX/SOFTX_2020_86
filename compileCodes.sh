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

		if [$currDir == "vkcomp"]
			continue
		fi

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

if [ "$1" == "test" ]; then
	clear;
	cd 2DCONV
	echo "Performing 2DConvolution"
	./2DConvolution ;
	cd ..
	cd 2MM
	echo "Performing 2MM. Warning: CPU implementation will take a while..."
	./2mm ;
	cd ..
	cd 3DCONV
	echo "Performing 3DConvolution"
	./3DConvolution ;
	cd ..
	cd 3MM
	echo "Performing 3MM" 
	./3mm ;
	cd ..
	cd ATAX
	echo "Performing ATAX"
	./atax ;
	cd ..
	cd BICG
	echo "Performing BICG"
	./bicg ;
	cd ..
	cd CORR
	echo "Performing CORR"
	./correlation ; 
	cd ..
	cd COVAR
	echo "Performing covariance"
	./covariance ;
	cd ..
	cd FDTD-2D
	echo "Performing FDTD-2D"
	./fdtd2d ;
	cd ..
	cd GEMM
	echo "Performing GEMM"
	./gemm ;
	cd ..
	cd GESUMMV
	echo "Performing GESUMMV"
	./gesummv ;
	cd ..
	cd GRAMSCHM
	echo "Performing GRAMSCHM"
	./gramschmidt ;
	cd ..
	cd MVT
	echo "Performing MVT"
	./mvt ; 
	cd ..
	cd SYR2K
	echo "Performing SYR2K"
	./syr2k ;
	cd ..
	cd SYRK 
	echo "Performing SYRK"
	./syrk ;
	cd ..
fi
