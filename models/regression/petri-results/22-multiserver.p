# petrisrvn 4.1
# petrisrvn -M -p -o22-multiserver.out  22-multiserver.in
V y
C 5.5511e-17
I 8
PP 2
NP 1
#!User:  0:00:00.55
#!Sys:   0:00:00.54
#!Real:  0:00:01.15

W 1
client : client server 0.000000 -1
  -1
-1


X 2
client : client 2.000000 -1
  -1
server : server 1.000000 -1
  -1
-1

FQ 2
client : client 1.500000 3.000000 -1 3.000000
-1
server : server 1.500000 1.500000 -1 1.500000
-1
-1

P client 1
client 1 0 3 client 1.500000 0.000000 -1 -1
-1

P server 1
server 1 0 3 server 1.500000 0.000000 -1 -1
-1
-1

