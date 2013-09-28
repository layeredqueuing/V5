# petrisrvn 4.3
# petrisrvn -M -p -o42-activities.out  42-activities.in
V y
C 0
I 9
PP 2
NP 2
#!User:  0:00:00.17
#!Sys:   0:00:00.41
#!Real:  0:00:00.99

W 1
client : client server 0.200000 0.000000 -1
  -1
-1


J 1
server : fork1 fork2 1.000001 0.000000
  -1
-1


X 2
client : client 2.150001 0.000000 -1
  -1
server : server 0.950001 0.550000 -1
  -1

:
     server 0.250000 -1
     fork1 0.700000 -1
     fork2 0.800000 -1
     join 0.250000 -1
  -1
-1

FQ 2
client : client 0.465116 1.000000 0.000000 -1 1.000000
-1
server : server 0.465116 0.441860 0.255814 -1 0.697674
-1

:
    server 0.465116 0.116279 -1
    fork1 0.465116 0.325581 -1
    fork2 0.465116 0.372093 -1
    join 0.465116 0.116279 -1
  -1
-1

P client 1
client 1 0 1 client 0.465116 0.000000 0.000000 -1 -1
-1

P server 1
server 1 0 1 server 0.697674 0.000000 0.000000 -1 -1

:
         server 0.116279 0 -1
         fork1 0.186047 0.3 -1
         fork2 0.27907 0.2 -1
         join 0.116279 0 -1
 -1
-1
-1

