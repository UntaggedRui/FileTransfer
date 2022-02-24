server端运行
./bin/server  -a 0.0.0.0 -p 8888 -s 1024
client端运行
./bin/client -a 192.168.2.204 -p 8888 -s 1024 -t 3 -f test.txt
就会将test.txt以三个线程发送到server端。