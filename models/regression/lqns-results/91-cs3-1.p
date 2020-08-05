# lqns 5.16
# lqns --pragma=variance=mol,threads=hyper --parseable 91-cs3-1.in
# $Id: 91-cs3-1.p 13729 2020-08-04 20:20:16Z greg $
V y
C 5.64541e-07
I 4
PP 4
NP 2

#!Comment: TDA test #1
#!User:  0:00:00.003
#!Sys:   0:00:00.000
#!Real:  0:00:00.003
#!Solver: 2 8 106 2134 42705 6.99835e+08 0

B 6
client1        :cl1             0.333333    
client2        :cl2             1           
client3        :cl3             0.666667    
server1        :s1              0.166667    
                s2              1           
                s3              0.666667    
-1

W 3
client1        :cl1             s1              5.31873     0           -1 
                -1 
client2        :cl2             s2              8.27744     0           -1 
                -1 
client3        :cl3             s3              8.03351     0           -1 
                -1 
-1

X 6
client1        :cl1             8.31874     0           -1 
                -1 
client2        :cl2             9.27744     0           -1 
                -1 
client3        :cl3             9.53351     0           -1 
                -1 
server1        :s1              1           5           -1 
                s2              0.5         0.5         -1 
                s3              0.5         1           -1 
                -1 
-1

VAR 6
client1        :cl1             169.017     0           -1 
                -1 
client2        :cl2             278.679     0           -1 
                -1 
client3        :cl3             272.94      0           -1 
                -1 
server1        :s1              1           25          -1 
                s2              0.25        0.25        -1 
                s3              0.25        1           -1 
                -1 
-1

FQ 4
client1        :cl1             0.120211    1           0           -1 1
                -1 
client2        :cl2             0.107788    1           0           -1 1
                -1 
client3        :cl3             0.104893    1           0           -1 1
                -1 
server1        :s1              0.120211    0.120211    0.601053    -1 0.721264
                s2              0.107788    0.0538942   0.0538942   -1 0.107788
                s3              0.104893    0.0524466   0.104893    -1 0.15734
                -1 
                                0.332892    0.226551    0.75984     -1 0.986392
-1

P client1Proc 1
client1         1  0 1  cl1             0.240421    0           0           -1 
                        -1 
-1 

P client2Proc 1
client2         1  0 1  cl2             0.0538942   0           0           -1 
                        -1 
-1 

P client3Proc 1
client3         1  0 1  cl3             0.104893    0           0           -1 
                        -1 
-1 

P server1Proc 1
server1         3  0 1  s1              0.721264    0           0           -1 
                        s2              0.107788    0           0           -1 
                        s3              0.15734     0           0           -1 
                        -1 
                                        0.986392    
-1 

-1

