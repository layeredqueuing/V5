# petrisrvn 4.1
# petrisrvn -M -p -o23-multiserver.out  23-multiserver.in
V y
C 4.5696e-06
I 4
PP 2
NP 1
#!User:  0:00:00.61
#!Sys:   0:00:00.17
#!Real:  0:00:00.81

W 1
client : client server 0.000000 -1
  -1
-1


X 2
client : client 3.200004 -1
  -1
server : server 2.200001 -1
  -1
-1

FQ 2
client : client 0.937499 3.000000 -1 3.000000
-1
server : server 0.937500 2.062501 -1 2.062501
-1
-1

P client 1
client 1 0 3 client 0.937499 0.000000 -1 -1
-1

P server 1
server 1 0 1 server 0.937500 1.200002 -1 -1
-1
-1

