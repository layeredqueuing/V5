# petrisrvn 3.10
# petrisrvn -p  case-03-01.in
V y
C 2.105e-16
I 57
PP 3
NP 2
#!User:  0:00:00.30
#!Sys:   0:00:00.42
#!Real:  0:00:01.20

W 3
client : client server 0.000000 0.000000 -1
     client acquire 0.000000 0.000000 -1
  -1
server : server release 0.030630 0.000000 -1
  -1
-1


DP 1
server : server release 0.000000 0.000000 -1
  -1
-1


X 4
client : client 0.205412 1.000000 -1
  -1
server : server 0.200000 0.000000 -1
  -1
lock : acquire 0.005412 0.200000 -1
     release 0.030630 0.000000 -1
  -1
-1

FQ 3
client 1 client 0.829592 0.170408 0.829592 -1 1.000000 -1
server 1 server 0.829592 0.165918 0.000000 -1 0.165918 -1
lock 2 acquire 0.829593 0.004489 0.165918 -1 0.170408
  release 0.829593 0.025410 0.000000 -1 0.025410 -1
    1.659186 0.029900 0.165918 -1 0.195818
-1

P client 1
client 1 0 1 client  0.829592 0.000000 0.000000 -1 -1
-1

P server 1
server 1 0 1 server  0.165918 0.000000 0.000000 -1 -1
-1

P lock 1
lock 2 0 3 acquire  0.166748 0.004412 0.000000 -1
         release  0.000830 0.029630 0.000000 -1 -1
      0.167578
-1
-1

