# petrisrvn 3.10
# petrisrvn -p  case-02-03.in
V y
C 4.2646e-06
I 1960
PP 3
NP 2
#!User:  0:00:00.98
#!Sys:   0:00:00.51
#!Real:  0:00:01.76

W 3
client : client server 0.000000 0.049735 -1
     client acquire 0.135240 0.000000 -1
  -1
server : server release 0.249378 0.000000 -1
  -1
-1


DP 1
server : server release 0.000000 0.000000 -1
  -1
-1


X 4
client : client 0.417494 1.049735 -1
  -1
server : server 0.200000 0.000000 -1
  -1
lock : acquire 0.082255 0.356374 -1
     release 0.114066 0.000000 -1
  -1
-1

FQ 3
client 1 client 2.044670 0.853638 2.146362 -1 3.000000 -1
server 1 server 2.044669 0.408934 0.000000 -1 0.408934 -1
lock 2 acquire 2.044670 0.168184 0.728668 -1 0.896851
  release 2.044668 0.233226 0.000000 -1 0.233226 -1
    4.089338 0.401410 0.728668 -1 1.130077
-1

P client 1
client 1 0 3 client  2.044670 0.000000 0.000000 -1 -1
-1

P server 1
server 1 0 1 server  0.408934 0.000000 0.000000 -1 -1
-1

P lock 1
lock 2 0 2 acquire  0.663495 0.081255 0.032874 -1
         release  0.002045 0.113065 0.000000 -1 -1
      0.66554
-1
-1

