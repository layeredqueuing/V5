# petrisrvn 3.10
# petrisrvn -p  case-02-02.in
V y
C 9.7843e-06
I 348
PP 3
NP 2
#!User:  0:00:00.43
#!Sys:   0:00:00.41
#!Real:  0:00:01.21

W 3
client : client server 0.000000 0.022749 -1
     client acquire 0.044782 0.000000 -1
  -1
server : server release 0.138314 0.000000 -1
  -1
-1


DP 1
server : server release 0.000000 0.000000 -1
  -1
-1


X 4
client : client 0.303640 1.022748 -1
  -1
server : server 0.200000 0.000000 -1
  -1
lock : acquire 0.058857 0.320542 -1
     release 0.092613 0.000000 -1
  -1
-1

FQ 3
client 1 client 1.507854 0.457845 1.542155 -1 2.000000 -1
server 1 server 1.507853 0.301571 0.000000 -1 0.301571 -1
lock 2 acquire 1.507854 0.088749 0.483330 -1 0.572079
  release 1.507854 0.139646 0.000000 -1 0.139646 -1
    3.015708 0.228395 0.483330 -1 0.711725
-1

P client 1
client 1 0 2 client  1.507853 0.000000 0.000000 -1 -1
-1

P server 1
server 1 0 1 server  0.301571 0.000000 0.000000 -1 -1
-1

P lock 1
lock 2 0 2 acquire  0.473466 0.057858 0.007542 -1
         release  0.001508 0.091613 0.000000 -1 -1
      0.474974
-1
-1

