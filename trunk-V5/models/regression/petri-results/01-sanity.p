# petrisrvn 4.3
# petrisrvn -M -p -o01-sanity.out  01-sanity.in
V y
C 0
I 2
PP 2
NP 1
#!User:  0:00:00.20
#!Sys:   0:00:00.48
#!Real:  0:00:01.31

W 1
client : client server 0.000000 -1
  -1
-1


X 2
client : client 2.000000 -1
  -1
server : server 1.000000 -1
  -1
-1

FQ 2
client : client 0.500000 1.000000 -1 1.000000
-1
server : server 0.500000 0.500000 -1 0.500000
-1
-1

P client 1
client 1 0 1 client 0.500000 0.000000 -1 -1
-1

P server 1
server 1 0 1 server 0.500000 0.000000 -1 -1
-1
-1

