@echo off

set nvskey=./out.bin
set inputf=./nvs_aes.bin
set out=./_nvs.csv

python.exe ./nvs_partition_gen.py decrypt %inputf% %nvskey% %out%

::echo %errorlevel%

pause
