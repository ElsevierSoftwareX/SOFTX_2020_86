@Echo Off

FOR /D %%G in ("*") DO (
    CD "%%G"
    IF "%1"=="clean" (
        make.exe clean
    ) ELSE ( 
        IF NOT "%1"=="test" (
            make.exe
            ) 
    )
    CD ..
)

IF "%1"=="clean" (
    CD vkcomp
    ECHO "Erasing object files in vkcomp"
    rm *.o 
    CD ..
)

IF "%1"=="test" (
	clear
	cd 2DCONV
	echo "Performing 2DConvolution"
	2DConvolution.exe
	cd ..
	cd 2MM
	echo "Performing 2MM. Warning: CPU implementation will take a while..."
	2mm.exe
	cd ..
	cd 3DCONV
	echo "Performing 3DConvolution"
	3DConvolution.exe
	cd ..
    cd 3MM
	echo "Performing 3MM" 
	3mm.exe
	cd ..
	cd ATAX
	echo "Performing ATAX"
	atax.exe
	cd ..
	cd BICG
	echo "Performing BICG"
	bicg.exe
	cd ..
	cd CORR
	echo "Performing CORR"
	correlation.exe 
	cd ..
	cd COVAR
	echo "Performing covariance"
	covariance.exe
	cd ..
	cd FDTD-2D
	echo "Performing FDTD-2D"
	fdtd2d.exe
	cd ..
	cd GEMM
	echo "Performing GEMM"
	gemm.exe
	cd ..
	cd GESUMMV
	echo "Performing GESUMMV"
	gesummv.exe
	cd ..
	cd GRAMSCHM
	echo "Performing GRAMSCHM"
	gramschmidt.exe
	cd ..
	cd MVT
	echo "Performing MVT"
	mvt.exe 
	cd ..
	cd SYR2K
	echo "Performing SYR2K"
	syr2k.exe
	cd ..
	cd SYRK 
	echo "Performing SYRK"
	syrk.exe
	cd ..
)




