# petrisrvn 3.10
# petrisrvn -p  case-01-06.in
V y
C 6.5975e-06
I 29583
PP 3
NP 2
#!User:  0:00:09.26
#!Sys:   0:00:01.30
#!Real:  0:00:11.00

W 3
client : client server 0.000000 0.069062 -1
     client acquire 2.362784 0.000000 -1
  -1
server : server release 2.384921 0.000000 -1
  -1
-1


DP 1
server : server release 0.000000 0.000000 -1
  -1
-1


X 4
client : client 2.563786 1.069062 -1
  -1
server : server 0.200000 0.000000 -1
  -1
lock : acquire 0.001000 0.600000 -1
     release 0.001000 0.000000 -1
  -1
-1

FQ 3
client 1 client 1.651597 4.234341 1.765659 -1 6.000000 -1
server 1 server 1.651598 0.330320 0.000000 -1 0.330320 -1
lock 2 acquire 1.651597 0.001652 0.990958 -1 0.992609
  release 1.651601 0.001652 0.000000 -1 0.001652 -1
    3.303198 0.003303 0.990958 -1 0.994261
-1

P client 1
client 1 0 6 client  1.651598 0.000000 0.000000 -1 -1
-1

P server 1
server 1 0 1 server  0.330320 0.000000 0.000000 -1 -1
-1

P lock 1
lock 2 0 1 acquire  0.992609 0.000000 0.000000 -1
         release  0.001652 0.000000 0.000000 -1 -1
      0.994261
-1
-1

