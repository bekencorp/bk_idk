set(OVERRIDE_COMPILE_OPTIONS 
    "-mcpu=cortex-m33+nodsp"
    "-mfpu=fpv5-sp-d16"
    "-mfloat-abi=hard"
    "-mcmse"
    "-fstack-protector"
    "--specs=nano.specs"
    "--specs=nosys.specs"
    "-ffunction-sections"
    "-fdata-sections"
)

set(OVERRIDE_LINK_OPTIONS
    "-Os"
    "--specs=nano.specs"
    "--specs=nosys.specs"

)
