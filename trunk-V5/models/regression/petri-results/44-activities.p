# petrisrvn 4.3
# petrisrvn -M -p -o44-activities.out  44-activities.in
V y
C 5.977e-06
I 4
PP 2
NP 1
#!User:  0:00:00.20
#!Sys:   0:00:00.47
#!Real:  0:00:01.72

W 1
client : client server 0.000000 -1
  -1
-1


X 2
client : client 2.000000 -1
  -1
server : server 0.999999 -1
  -1

:
     server 0.250000 -1
     loop 0.999999 -1
     done 0.250000 -1
  -1
-1

FQ 2
client : client 0.500000 1.000000 -1 1.000000
-1
server : server 0.500000 0.500000 -1 0.500000
-1

:
    server 0.500000 0.125000 -1
    loop 0.250000 0.250000 -1
    done 0.499999 0.125000 -1
  -1
-1

P client 1
client 1 0 1 client 0.500000 0.000000 -1 -1
-1

P server 1
server 1 0 1 server 0.500000 0.000000 -1 -1

:
         server 0.125 0 -1
         loop 0.25 0 -1
         done 0.125 0 -1
 -1
-1
-1

