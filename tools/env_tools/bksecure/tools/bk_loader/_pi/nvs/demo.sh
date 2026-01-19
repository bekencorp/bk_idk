#decrypt nvs_key -- infile can use -inwords instead
./beken_aes decrypt -infile ./nvs_key_aes_crc.bin -outfile ./nvskey_out.bin -startaddress 0 -keywords 73c7bf397f2ad6bf4e7403a7b965dc5ce0645df039c2d69c814ffb403183fb18

#decrypt nvs_partition
python ./nvs_partition_gen.py decrypt ./nvs_aes.bin ./nvskey_out.bin ./pi_nvs.csv
