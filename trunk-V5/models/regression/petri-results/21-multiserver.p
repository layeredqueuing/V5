# petrisrvn 4.1
# petrisrvn -M -p -o21-multiserver.out  21-multiserver.in
V y
C 4.4615e-06
I 39
PP 2
NP 2
#!User:  0:00:00.65
#!Sys:   0:00:00.16
#!Real:  0:00:00.82

W 1
client : client server 0.062423 0.000000 -1
  -1
-1


X 2
client : client 1.562423 0.000000 -1
  -1
server : server 0.500000 0.500001 -1
  -1
-1

FQ 2
client : client 1.920094 3.000000 0.000000 -1 3.000000
-1
server : server 1.920096 0.960049 0.960050 -1 1.920098
-1
-1

P client 1
client 1 0 3 client 1.920094 0.000000 0.000000 -1 -1
-1

P server 1
server 1 0 3 server 1.920099 0.000000 0.000000 -1 -1
-1
-1

