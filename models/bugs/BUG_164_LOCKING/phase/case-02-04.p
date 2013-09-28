# petrisrvn 3.10
# petrisrvn -p  case-02-04.in
V y
C 6.0563e-06
I 8443
PP 3
NP 2
#!User:  0:00:03.24
#!Sys:   0:00:00.77
#!Real:  0:00:04.34

W 3
client : client server 0.000000 0.078722 -1
     client acquire 0.280454 0.000000 -1
  -1
server : server release 0.398942 0.000000 -1
  -1
-1


DP 1
server : server release 0.000000 0.000000 -1
  -1
-1


X 4
client : client 0.578416 1.078722 -1
  -1
server : server 0.200000 0.000000 -1
  -1
lock : acquire 0.097962 0.385562 -1
     release 0.122758 0.000000 -1
  -1
-1

FQ 3
client 1 client 2.413801 1.396180 2.603820 -1 4.000000 -1
server 1 server 2.413800 0.482760 0.000000 -1 0.482760 -1
lock 2 acquire 2.413800 0.236461 0.930669 -1 1.167131
  release 2.413802 0.296313 0.000000 -1 0.296313 -1
    4.827602 0.532774 0.930669 -1 1.463443
-1

P client 1
client 1 0 4 client  2.413801 0.000000 0.000000 -1 -1
-1

P server 1
server 1 0 1 server  0.482760 0.000000 0.000000 -1 -1
-1

P lock 1
lock 2 0 2 acquire  0.799450 0.096962 0.055362 -1
         release  0.002414 0.121758 0.000000 -1 -1
      0.801864
-1
-1

