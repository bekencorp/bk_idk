@echo off

set key=73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18
set inputf=./nvs_key_aes_out_crc.bin
set input=fed195e120ada2f13875058d8f22a7881df025f29f1b9cb9ea79ab659476d4dac959c15436c317c8d2d85423447f2b0e34cd0bfdb35846ea52afa234ce771a8c812e63b62c9131ebc49039e24c4956defc8d155b79eaaba9ad61d65fe26813a22e1dead323cce23d30176f14bbfaf8d78f1e7ed603cf194e38516f29f938d5433ab85993c2f782a1efc3d8dbeebda7817303e46968099bf619c61ebc955cb7b8b6be5872577eb9d3cd089e8167609eafa03bdabb8caae754f995550059e5e9929579c42c36864e54a579da3bd497c83e845e3e6ae6f64d54b552875f44cc060904425e8e47112e93cf1b7d4b6e2ba146efe72012634a95dfcd3d1c47e06c

::beken_aes.exe decrypt -inwords %input% -outfile ./out.bin -startaddress 0 -keywords %key% 
beken_aes.exe decrypt -infile %inputf% -outfile ./out.bin -startaddress 0 -keywords %key% 
::beken_aes.exe encrypt -infile ./nvs_key_raw.bin -outfile ./nvs_key_aes_out.bin -startaddress 0 -keywords %key% 

::echo %errorlevel%

pause
