set(OVERRIDE_COMPILE_OPTIONS
	"-mcpu=cortex-m33+nodsp"
	"-mfloat-abi=soft"
	"-mcmse"
	"-fstack-protector"
	"--specs=nano.specs"
)

set(OVERRIDE_LINK_OPTIONS
    "-fno-builtin-printf"
    "-Os"
    "--specs=nano.specs"
)
