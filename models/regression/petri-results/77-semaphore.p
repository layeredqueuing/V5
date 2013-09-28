# petrisrvn 4.4
# petrisrvn -M -p -o77-semaphore.out  77-semaphore.in
V y
C 2.4822e-06
I 16
PP 2
NP 1
#!User:  0:00:00.62
#!Sys:   0:00:00.30
#!Real:  0:00:01.16

W 2
client1 : client1 server1 0.500001 -1
  -1
client2 : client2 server2 0.500000 -1
  -1
-1


X 4
client1 : client1 3.000003 -1
  -1
client2 : client2 3.000003 -1
  -1
server : server2 1.000001 -1
     server1 1.000001 -1
  -1
-1


H 1
server : server1 server2 1.000001 1.500002 0.000000 -1
-1

FQ 3
client1 : client1 0.333333 1.000000 -1 1.000000
-1
client2 : client2 0.333333 1.000000 -1 1.000000
-1
server : server2 0.333333 0.416667 -1 0.416667
  server1 0.333333 0.416667 -1 0.416667
-1
    0.333333 0.833333 -1 0.833333
-1

P client 2
client1 1 0 1 client1 0.333333 0.500001 -1 -1
client2 1 0 1 client2 0.333333 0.500000 -1 -1
-1
	0.666667
-1

P server 1
server 2 0 1 server2 0.333333 0.000000 -1
         server1 0.333333 0.000000 -1 -1
      0.666667
-1
-1

