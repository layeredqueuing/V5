# petrisrvn 3.10
# petrisrvn -p  case-01-03.in
V y
C 6.7993e-06
I 568
PP 3
NP 2
#!User:  0:00:00.37
#!Sys:   0:00:00.46
#!Real:  0:00:01.13

W 3
client : client server 0.000000 0.037550 -1
     client acquire 0.808459 0.000000 -1
  -1
server : server release 0.866522 0.000000 -1
  -1
-1


DP 1
server : server release 0.000000 0.000000 -1
  -1
-1


X 4
client : client 1.009457 1.037550 -1
  -1
server : server 0.200000 0.000000 -1
  -1
lock : acquire 0.001000 0.599999 -1
     release 0.001000 0.000000 -1
  -1
-1

FQ 3
client 1 client 1.465554 1.479414 1.520586 -1 3.000000 -1
server 1 server 1.465555 0.293111 0.000000 -1 0.293111 -1
lock 2 acquire 1.465554 0.001466 0.879331 -1 0.880796
  release 1.465558 0.001466 0.000000 -1 0.001466 -1
    2.931112 0.002931 0.879331 -1 0.882262
-1

P client 1
client 1 0 3 client  1.465554 0.000000 0.000000 -1 -1
-1

P server 1
server 1 0 1 server  0.293111 0.000000 0.000000 -1 -1
-1

P lock 1
lock 2 0 1 acquire  0.880796 0.000000 0.000000 -1
         release  0.001466 0.000000 0.000000 -1 -1
      0.882262
-1
-1

