#!/bin/bash

config_dir=Student-FTP-Server-Config
server_port=`cat $config_dir/port.txt | cut -d '=' -f2`
input_file=ftp.input
log_file=ftp.log
output_file=ftp.out

# generate an input file to run the ftp-client
echo "open localhost $server_port" > $input_file
echo "binary" >> $input_file
echo "ls documents/Text $output_file" >> $input_file
echo "yes" >> $input_file
echo "close" >> $input_file
echo "quit" >> $input_file

# run the ftp-client
ftp -n -v < $input_file > $log_file

