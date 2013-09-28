# petrisrvn 3.10
# petrisrvn -p  case-02-01.in
V y
C 1.8822e-16
I 30
PP 3
NP 2
#!User:  0:00:00.25
#!Sys:   0:00:00.48
#!Real:  0:00:01.04

W 3
client : client server 0.000000 0.000000 -1
     client acquire 0.017435 0.000000 -1
  -1
server : server release 0.073321 0.000000 -1
  -1
-1


DP 1
server : server release 0.000000 0.000000 -1
  -1
-1


X 4
client : client 0.218493 1.000000 -1
  -1
server : server 0.200000 0.000000 -1
  -1
lock : acquire 0.001058 0.300000 -1
     release 0.073321 0.000000 -1
  -1
-1

FQ 3
client 1 client 0.820686 0.179314 0.820686 -1 1.000000 -1
server 1 server 0.820686 0.164137 0.000000 -1 0.164137 -1
lock 2 acquire 0.820686 0.000869 0.246206 -1 0.247074
  release 0.820686 0.060174 0.000000 -1 0.060174 -1
    1.641372 0.061042 0.246206 -1 0.307248
-1

P client 1
client 1 0 1 client  0.820686 0.000000 0.000000 -1 -1
-1

P server 1
server 1 0 1 server  0.164137 0.000000 0.000000 -1 -1
-1

P lock 1
lock 2 0 2 acquire  0.247026 0.000058 0.000000 -1
         release  0.000821 0.072321 0.000000 -1 -1
      0.247847
-1
-1

