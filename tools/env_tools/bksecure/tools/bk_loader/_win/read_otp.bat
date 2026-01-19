@echo off

::address / length is hex
bk_loader.exe readinfo -p 35 --read-otp 4b010000-16 --reboot 0

@echo on
pause