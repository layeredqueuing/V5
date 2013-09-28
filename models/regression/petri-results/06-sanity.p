# petrisrvn 4.3
# petrisrvn -M -p -o06-sanity.out  06-sanity.in
V y
C 0
I 3
PP 2
NP 1
#!User:  0:00:00.12
#!Sys:   0:00:00.10
#!Real:  0:00:00.37

W 1
client : client server 0.000000 -1
  -1
-1


X 2
client : client 1.500000 -1
  -1
server : server 1.000000 -1
  -1
-1

FQ 2
client : client 0.500000 0.750000 -1 0.750000
-1
server : server 0.500000 0.500000 -1 0.500000
-1
-1

P client 1
client 1 0 1 client 0.250000 0.000000 -1 -1
-1

P server 1
server 1 0 1 server 0.500000 0.000000 -1 -1
-1
-1

