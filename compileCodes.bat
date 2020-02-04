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

SET DEVICE_SELECTION=0

IF "%1"=="test" (
	clear
	cd 2DCONV
	echo "Performing 2DConvolution"
	2DConvolution.exe %DEVICE_SELECTION%
	cd ..
	cd 2MM
	echo "Performing 2MM. Warning: CPU implementation will take a while..."
	2mm.exe %DEVICE_SELECTION%
	cd ..
	cd 3DCONV
	echo "Performing 3DConvolution"
	3DConvolution.exe %DEVICE_SELECTION%
	cd ..
    cd 3MM
	echo "Performing 3MM" 
	3mm.exe %DEVICE_SELECTION%
	cd ..
	cd ATAX
	echo "Performing ATAX"
	atax.exe %DEVICE_SELECTION%
	cd ..
	cd BICG
	echo "Performing BICG"
	bicg.exe %DEVICE_SELECTION%
	cd ..
	cd CORR
	echo "Performing CORR"
	correlation.exe  %DEVICE_SELECTION%
	cd ..
	cd COVAR
	echo "Performing covariance"
	covariance.exe %DEVICE_SELECTION%
	cd ..
	cd FDTD-2D
	echo "Performing FDTD-2D"
	fdtd2d.exe %DEVICE_SELECTION%
	cd ..
	cd GEMM
	echo "Performing GEMM"
	gemm.exe %DEVICE_SELECTION%
	cd ..
	cd GESUMMV
	echo "Performing GESUMMV"
	gesummv.exe %DEVICE_SELECTION%
	cd ..
	cd GRAMSCHM
	echo "Performing GRAMSCHM"
	gramschmidt.exe %DEVICE_SELECTION%
	cd ..
	cd MVT
	echo "Performing MVT"
	mvt.exe %DEVICE_SELECTION%
	cd ..
	cd SYR2K
	echo "Performing SYR2K"
	syr2k.exe %DEVICE_SELECTION%
	cd ..
	cd SYRK 
	echo "Performing SYRK"
	syrk.exe %DEVICE_SELECTION%
	cd ..
)




