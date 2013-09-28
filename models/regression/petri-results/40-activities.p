# petrisrvn 4.3
# petrisrvn -p  40-activities.in
V y
C 0
I 7
PP 2
NP 1
#!User:  0:00:00.09
#!Sys:   0:00:00.09
#!Real:  0:00:00.31

W 1
  client : -1

:
     client server 0.000000 -1
  -1
-1


J 1
server : fork1 fork2 1.000000 0.000000
  -1
-1


X 2
client : client 2.500000 -1
  -1

:
     client 2.500000 -1
  -1
server : server 1.500000 -1
  -1

:
     server 0.250000 -1
     fork1 0.700000 -1
     fork2 0.800000 -1
     join 0.250000 -1
  -1
-1

FQ 2
client : client 0.400000 1.000000 -1 1.000000
-1

:
    client 0.400000 1.000000 -1
  -1
server : server 0.400000 0.600000 -1 0.600000
-1

:
    server 0.400000 0.100000 -1
    fork1 0.400000 0.280000 -1
    fork2 0.400000 0.320000 -1
    join 0.400000 0.100000 -1
  -1
-1

P client 1
client 1 0 1 client 0.400000 0.000000 -1 -1

:
         client 0.4 0 -1
 -1
-1

P server 1
server 1 0 1 server 0.600000 0.000000 -1 -1

:
         server 0.1 0 -1
         fork1 0.16 0.3 -1
         fork2 0.24 0.2 -1
         join 0.1 0 -1
 -1
-1
-1

