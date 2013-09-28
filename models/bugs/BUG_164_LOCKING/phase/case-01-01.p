# petrisrvn 3.10
# petrisrvn -p  case-01-01.in
V y
C 1.9429e-16
I 18
PP 3
NP 2
#!User:  0:00:00.19
#!Sys:   0:00:00.50
#!Real:  0:00:01.62

W 3
client : client server 0.000000 0.000000 -1
     client acquire 0.116786 0.000000 -1
  -1
server : server release 0.265490 0.000000 -1
  -1
-1


DP 1
server : server release 0.000000 0.000000 -1
  -1
-1


X 4
client : client 0.317786 0.999999 -1
  -1
server : server 0.200000 0.000000 -1
  -1
lock : acquire 0.001000 0.599999 -1
     release 0.001000 0.000000 -1
  -1
-1

FQ 3
client 1 client 0.758849 0.241151 0.758849 -1 1.000000 -1
server 1 server 0.758849 0.151770 0.000000 -1 0.151770 -1
lock 2 acquire 0.758849 0.000759 0.455309 -1 0.456068
  release 0.758849 0.000759 0.000000 -1 0.000759 -1
    1.517698 0.001518 0.455309 -1 0.456827
-1

P client 1
client 1 0 1 client  0.758849 0.000000 0.000000 -1 -1
-1

P server 1
server 1 0 1 server  0.151770 0.000000 0.000000 -1 -1
-1

P lock 1
lock 2 0 1 acquire  0.456068 0.000000 0.000000 -1
         release  0.000759 0.000000 0.000000 -1 -1
      0.456827
-1
-1

