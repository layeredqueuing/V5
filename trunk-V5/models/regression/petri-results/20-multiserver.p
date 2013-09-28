# petrisrvn 4.1
# petrisrvn -M -p -o20-multiserver.out  20-multiserver.in
V y
C 4.8406e-06
I 6
PP 2
NP 1
#!User:  0:00:00.62
#!Sys:   0:00:00.54
#!Real:  0:00:02.80

W 1
client : client server 0.352942 -1
  -1
-1


X 2
client : client 2.352945 -1
  -1
server : server 1.000000 -1
  -1
-1

FQ 2
client : client 1.699997 4.000000 -1 4.000000
-1
server : server 1.700002 1.700002 -1 1.700002
-1
-1

P client 1
client 1 0 4 client 1.699997 0.000000 -1 -1
-1

P server 1
server 1 0 2 server 1.700002 0.000000 -1 -1
-1
-1

