# petrisrvn 3.10
# petrisrvn -p  case-01-02.in
V y
C 7.5942e-06
I 121
PP 3
NP 2
#!User:  0:00:00.29
#!Sys:   0:00:00.42
#!Real:  0:00:00.99

W 3
client : client server 0.000000 0.019225 -1
     client acquire 0.420761 0.000000 -1
  -1
server : server release 0.516789 0.000000 -1
  -1
-1


DP 1
server : server release 0.000000 0.000000 -1
  -1
-1


X 4
client : client 0.621762 1.019225 -1
  -1
server : server 0.200000 0.000000 -1
  -1
lock : acquire 0.001000 0.600001 -1
     release 0.001000 0.000000 -1
  -1
-1

FQ 3
client 1 client 1.218779 0.757790 1.242210 -1 2.000000 -1
server 1 server 1.218779 0.243756 0.000000 -1 0.243756 -1
lock 2 acquire 1.218778 0.001219 0.731267 -1 0.732486
  release 1.218778 0.001219 0.000000 -1 0.001219 -1
    2.437556 0.002438 0.731267 -1 0.733705
-1

P client 1
client 1 0 2 client  1.218779 0.000000 0.000000 -1 -1
-1

P server 1
server 1 0 1 server  0.243756 0.000000 0.000000 -1 -1
-1

P lock 1
lock 2 0 1 acquire  0.732486 0.000000 0.000000 -1
         release  0.001219 0.000000 0.000000 -1 -1
      0.733705
-1
-1

