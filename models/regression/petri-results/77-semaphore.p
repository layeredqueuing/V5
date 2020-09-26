# petrisrvn 5.17
# petrisrvn --parseable --output=77-semaphore.out 77-semaphore.in
# $Id: 77-semaphore.p 13875 2020-09-25 20:02:38Z greg $
V y
C 1.90242e-06
I 0
PP 2
NP 1

#!Comment: Semaphore test case.
#!User:  0:00:00.060
#!Sys:   0:00:00.049
#!Real:  0:00:00.881

W 2
client1        :client1         server1         0.677596    -1 
                -1 
client2        :client2         server2         0.677596    -1 
                -1 
-1

X 4
client1        :client1         2.6776      -1 
                -1 
client2        :client2         2.6776      -1 
                -1 
server         :server1         1           -1 
                server2         1           -1 
                -1 
-1

H 0
server         :server1         server2         0           0           0          
-1

FQ 3
client1        :client1         0.373469    1           -1 1
                -1 
client2        :client2         0.373469    1           -1 1
                -1 
server         :server1         0.373469    0.373469    -1 0.436735
                server2         0.373469    0.373469    -1 0.436735
                -1 
                                0.373469    0.873469    -1 0.873469
-1

P client 2
client1         1  0 1  client1         0.373469    0           -1 
                        -1 
client2         1  0 1  client2         0.373469    0           -1 
                        -1 
                -1 
                                        0.746939
-1 

P server 1
server          2  0 1  server1         0.373469    0           -1 
                        server2         0.373469    0           -1 
                        -1 
                                        0.746939    
-1 

-1

