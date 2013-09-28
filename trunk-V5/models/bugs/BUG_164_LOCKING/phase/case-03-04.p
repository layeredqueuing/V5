# petrisrvn 3.10
# petrisrvn -p  case-03-04.in
V y
C 7.6827e-06
I 44757
PP 3
NP 2
#!User:  0:00:19.23
#!Sys:   0:00:02.21
#!Real:  0:00:21.95

W 3
client : client server 0.000000 0.089022 -1
     client acquire 0.057160 0.000000 -1
  -1
server : server release 0.175467 0.000000 -1
  -1
-1


DP 1
server : server release 0.000000 0.000000 -1
  -1
-1


X 4
client : client 0.357464 1.089021 -1
  -1
server : server 0.200000 0.000000 -1
  -1
lock : acquire 0.100304 0.273198 -1
     release 0.122940 0.000000 -1
  -1
-1

FQ 3
client 1 client 2.765325 0.988503 3.011497 -1 4.000000 -1
server 1 server 2.765324 0.553065 0.000000 -1 0.553065 -1
lock 2 acquire 2.765325 0.277373 0.755482 -1 1.032855
  release 2.765325 0.339970 0.000000 -1 0.339970 -1
    5.530650 0.617343 0.755482 -1 1.372825
-1

P client 1
client 1 0 4 client  2.765322 0.000000 0.000000 -1 -1
-1

P server 1
server 1 0 1 server  0.553065 0.000000 0.000000 -1 -1
-1

P lock 1
lock 2 0 3 acquire  0.643215 0.099304 0.041598 -1
         release  0.002765 0.121940 0.000000 -1 -1
      0.64598
-1
-1

