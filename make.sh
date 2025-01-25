# General idea: Remove ./queue to not accidently run the previous version,
#               as if the compilation failed the previous version is still available.

rm ./server
rm ./client

gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 pcc_server.c -o server
gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 pcc_client.c -o client
