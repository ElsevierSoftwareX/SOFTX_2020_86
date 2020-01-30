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
		else
			make
		fi
		cd ..
    fi
done
