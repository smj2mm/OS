#!/bin/bash

config_dir=Student-FTP-Server-Config
test_data_dir=Test-Data
server_port=`cat $config_dir/port.txt | cut -d '=' -f2`
input_file=ftp.input
log_file=ftp.log

# generate an input file to run the ftp-client
echo "open localhost $server_port" > $input_file
echo "binary" >> $input_file
echo "put $test_data_dir/ftp-min-spec.txt documents/Text/ftp-min-spec.txt" >> $input_file
echo "close" >> $input_file
echo "quit" >> $input_file

# run the ftp-client
ftp -n -v < $input_file > $log_file

