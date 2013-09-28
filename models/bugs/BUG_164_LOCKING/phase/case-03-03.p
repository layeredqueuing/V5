# petrisrvn 3.10
# petrisrvn -p  case-03-03.in
V y
C 8.7886e-06
I 8993
PP 3
NP 2
#!User:  0:00:04.73
#!Sys:   0:00:00.86
#!Real:  0:00:05.95

W 3
client : client server 0.000000 0.053445 -1
     client acquire 0.019087 0.000000 -1
  -1
server : server release 0.107732 0.000000 -1
  -1
-1


DP 1
server : server release 0.000000 0.000000 -1
  -1
-1


X 4
client : client 0.288378 1.053445 -1
  -1
server : server 0.200000 0.000000 -1
  -1
lock : acquire 0.069291 0.236878 -1
     release 0.092598 0.000000 -1
  -1
-1

FQ 3
client 1 client 2.235765 0.644745 2.355255 -1 3.000000 -1
server 1 server 2.235765 0.447153 0.000000 -1 0.447153 -1
lock 2 acquire 2.235765 0.154919 0.529604 -1 0.684522
  release 2.235765 0.207027 0.000000 -1 0.207027 -1
    4.471530 0.361946 0.529604 -1 0.891549
-1

P client 1
client 1 0 3 client  2.235764 0.000000 0.000000 -1 -1
-1

P server 1
server 1 0 1 server  0.447153 0.000000 0.000000 -1 -1
-1

P lock 1
lock 2 0 3 acquire  0.493434 0.068291 0.017178 -1
         release  0.002236 0.091598 0.000000 -1 -1
      0.49567
-1
-1

