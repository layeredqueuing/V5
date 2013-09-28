# petrisrvn 3.10
# petrisrvn -p  case-02-05.in
V y
C 4.9739e-06
I 32240
PP 3
NP 2
#!User:  0:00:11.58
#!Sys:   0:00:01.57
#!Real:  0:00:13.95

W 3
client : client server 0.000000 0.108271 -1
     client acquire 0.474863 0.000000 -1
  -1
server : server release 0.591124 0.000000 -1
  -1
-1


DP 1
server : server release 0.000000 0.000000 -1
  -1
-1


X 4
client : client 0.779526 1.108270 -1
  -1
server : server 0.200000 0.000000 -1
  -1
lock : acquire 0.104664 0.406705 -1
     release 0.126390 0.000000 -1
  -1
-1

FQ 3
client 1 client 2.648592 2.064646 2.935354 -1 5.000000 -1
server 1 server 2.648589 0.529718 0.000000 -1 0.529718 -1
lock 2 acquire 2.648592 0.277212 1.077195 -1 1.354407
  release 2.648590 0.334755 0.000000 -1 0.334755 -1
    5.297182 0.611967 1.077195 -1 1.689162
-1

P client 1
client 1 0 5 client  2.648590 0.000000 0.000000 -1 -1
-1

P server 1
server 1 0 1 server  0.529718 0.000000 0.000000 -1 -1
-1

P lock 1
lock 2 0 2 acquire  0.885424 0.103664 0.073405 -1
         release  0.002649 0.125390 0.000000 -1 -1
      0.888073
-1
-1

