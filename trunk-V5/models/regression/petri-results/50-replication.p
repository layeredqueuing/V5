# petrisrvn 4.3
# petrisrvn -M -p -o50-replication.out  50-replication.in
V y
C 3.4288e-06
I 25
PP 6
NP 1
#!User:  0:00:00.11
#!Sys:   0:00:00.09
#!Real:  0:00:01.06

W 4
client_1 : client_1 server_1 0.500001 -1
  -1
client_2 : client_2 server_2 0.500001 -1
  -1
client_3 : client_3 server_1 0.500001 -1
  -1
client_4 : client_4 server_2 0.500001 -1
  -1
-1


X 6
client_1 : client_1 2.500000 -1
  -1
client_2 : client_2 2.500000 -1
  -1
client_3 : client_3 2.500000 -1
  -1
client_4 : client_4 2.500000 -1
  -1
server_1 : server_1 1.000001 -1
  -1
server_2 : server_2 1.000001 -1
  -1
-1

FQ 6
client_1 : client_1 0.400000 1.000000 -1 1.000000
-1
client_2 : client_2 0.400000 1.000000 -1 1.000000
-1
client_3 : client_3 0.400000 1.000000 -1 1.000000
-1
client_4 : client_4 0.400000 1.000000 -1 1.000000
-1
server_1 : server_1 0.800000 0.800000 -1 0.800000
-1
server_2 : server_2 0.800000 0.800000 -1 0.800000
-1
-1

P client_1 1
client_1 1 0 1 client_1 0.400000 0.000000 -1 -1
-1

P client_2 1
client_2 1 0 1 client_2 0.400000 0.000000 -1 -1
-1

P client_3 1
client_3 1 0 1 client_3 0.400000 0.000000 -1 -1
-1

P client_4 1
client_4 1 0 1 client_4 0.400000 0.000000 -1 -1
-1

P server_1 1
server_1 1 0 1 server_1 0.800000 0.000000 -1 -1
-1

P server_2 1
server_2 1 0 1 server_2 0.800000 0.000000 -1 -1
-1
-1

