# petrisrvn 3.10
# petrisrvn -p  case-01-04.in
V y
C 7.2777e-06
I 2261
PP 3
NP 2
#!User:  0:00:00.90
#!Sys:   0:00:00.46
#!Real:  0:00:01.71

W 3
client : client server 0.000000 0.052551 -1
     client acquire 1.273413 0.000000 -1
  -1
server : server release 1.309380 0.000000 -1
  -1
-1


DP 1
server : server release 0.000000 0.000000 -1
  -1
-1


X 4
client : client 1.474412 1.052551 -1
  -1
server : server 0.200000 0.000000 -1
  -1
lock : acquire 0.001000 0.600000 -1
     release 0.001000 0.000000 -1
  -1
-1

FQ 3
client 1 client 1.582928 2.333888 1.666112 -1 4.000000 -1
server 1 server 1.582928 0.316586 0.000000 -1 0.316586 -1
lock 2 acquire 1.582928 0.001583 0.949757 -1 0.951340
  release 1.582926 0.001583 0.000000 -1 0.001583 -1
    3.165854 0.003166 0.949757 -1 0.952923
-1

P client 1
client 1 0 4 client  1.582928 0.000000 0.000000 -1 -1
-1

P server 1
server 1 0 1 server  0.316586 0.000000 0.000000 -1 -1
-1

P lock 1
lock 2 0 1 acquire  0.951340 0.000000 0.000000 -1
         release  0.001583 0.000000 0.000000 -1 -1
      0.952923
-1
-1

