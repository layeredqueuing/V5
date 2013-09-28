# petrisrvn 3.10
# petrisrvn -p  case-03-02.in
V y
C 5.9306e-06
I 1091
PP 3
NP 2
#!User:  0:00:00.86
#!Sys:   0:00:00.66
#!Real:  0:00:01.72

W 3
client : client server 0.000000 0.023856 -1
     client acquire 0.005318 0.000000 -1
  -1
server : server release 0.063387 0.000000 -1
  -1
-1


DP 1
server : server release 0.000000 0.000000 -1
  -1
-1


X 4
client : client 0.241750 1.023856 -1
  -1
server : server 0.200000 0.000000 -1
  -1
lock : acquire 0.036432 0.211761 -1
     release 0.061908 0.000000 -1
  -1
-1

FQ 3
client 1 client 1.580270 0.382031 1.617969 -1 2.000000 -1
server 1 server 1.580270 0.316054 0.000000 -1 0.316054 -1
lock 2 acquire 1.580271 0.057573 0.334640 -1 0.392213
  release 1.580271 0.097831 0.000000 -1 0.097831 -1
    3.160542 0.155404 0.334640 -1 0.490044
-1

P client 1
client 1 0 2 client  1.580270 0.000000 0.000000 -1 -1
-1

P server 1
server 1 0 1 server  0.316054 0.000000 0.000000 -1 -1
-1

P lock 1
lock 2 0 3 acquire  0.331383 0.035432 0.003061 -1
         release  0.001580 0.060908 0.000000 -1 -1
      0.332963
-1
-1

