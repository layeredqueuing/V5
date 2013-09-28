# petrisrvn 3.10
# petrisrvn -p  case-01-05.in
V y
C 4.9983e-06
I 8320
PP 3
NP 2
#!User:  0:00:02.66
#!Sys:   0:00:00.58
#!Real:  0:00:03.57

W 3
client : client server 0.000000 0.062958 -1
     client acquire 1.798528 0.000000 -1
  -1
server : server release 1.824296 0.000000 -1
  -1
-1


DP 1
server : server release 0.000000 0.000000 -1
  -1
-1


X 4
client : client 1.999533 1.062958 -1
  -1
server : server 0.200000 0.000000 -1
  -1
lock : acquire 0.001000 0.600001 -1
     release 0.001000 0.000000 -1
  -1
-1

FQ 3
client 1 client 1.632658 3.264553 1.735447 -1 5.000000 -1
server 1 server 1.632658 0.326532 0.000000 -1 0.326532 -1
lock 2 acquire 1.632658 0.001633 0.979596 -1 0.981229
  release 1.632651 0.001633 0.000000 -1 0.001633 -1
    3.265309 0.003265 0.979596 -1 0.982861
-1

P client 1
client 1 0 5 client  1.632659 0.000000 0.000000 -1 -1
-1

P server 1
server 1 0 1 server  0.326532 0.000000 0.000000 -1 -1
-1

P lock 1
lock 2 0 1 acquire  0.981229 0.000000 0.000000 -1
         release  0.001633 0.000000 0.000000 -1 -1
      0.982861
-1
-1

