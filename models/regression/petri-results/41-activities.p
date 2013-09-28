# petrisrvn 4.3
# petrisrvn -M -p -o41-activities.out  41-activities.in
V y
C 2.7756e-17
I 5
PP 2
NP 1
#!User:  0:00:00.23
#!Sys:   0:00:00.40
#!Real:  0:00:00.82

W 1
client : client server 0.000000 -1
  -1
-1


X 2
client : client 2.049999 -1
  -1
server : server 1.050000 -1
  -1

:
     server 0.250000 -1
     fork1 0.400001 -1
     fork2 0.599999 -1
     join 0.250000 -1
  -1
-1

FQ 2
client : client 0.487805 1.000000 -1 1.000000
-1
server : server 0.487805 0.512195 -1 0.512195
-1

:
    server 0.487805 0.121951 -1
    fork1 0.121951 0.048780 -1
    fork2 0.365854 0.219512 -1
    join 0.487805 0.121951 -1
  -1
-1

P client 1
client 1 0 1 client 0.487805 0.000000 -1 -1
-1

P server 1
server 1 0 1 server 0.512195 0.000000 -1 -1

:
         server 0.121951 0 -1
         fork1 0.0487805 0 -1
         fork2 0.219512 0 -1
         join 0.121951 0 -1
 -1
-1
-1

