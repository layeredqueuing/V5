# petrisrvn 4.3
# petrisrvn -M -p -o43-activities.out  43-activities.in
V y
C 2.3324e-06
I 77
PP 2
NP 2
#!User:  0:00:00.26
#!Sys:   0:00:00.51
#!Real:  0:00:01.01

W 1
client : client server 0.175278 0.000000 -1
  -1
-1


J 1
server : fork1 fork2 0.854374 0.000000
  -1
-1


X 2
client : client 1.954954 0.000000 -1
  -1
server : server 0.779676 0.649950 -1
  -1

:
     server 0.276425 -1
     fork1 0.503251 -1
     fork2 0.688456 -1
     join 0.298826 -1
  -1
-1

FQ 2
client : client 1.023042 2.000000 0.000000 -1 2.000000
-1
server : server 1.023042 0.797641 0.664926 -1 1.462567
-1

:
    server 1.023042 0.282795 -1
    fork1 1.023042 0.514847 -1
    fork2 1.023042 0.704320 -1
    join 1.023042 0.305712 -1
  -1
-1

P client 1
client 1 0 2 client 1.023042 0.000000 0.000000 -1 -1
-1

P server 1
server 1 0 2 server 1.534563 0.000000 0.000000 -1 -1

:
         server 0.255761 0.0264251 -1
         fork1 0.409217 0.103251 -1
         fork2 0.613825 0.0884562 -1
         join 0.255761 0.0488261 -1
 -1
-1
-1

