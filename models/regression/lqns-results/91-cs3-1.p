# lqns 5.15
# lqns --pragma=variance=mol,threads=mak --parseable 91-cs3-1.in
# $Id: 91-cs3-1.p 13705 2020-07-20 21:46:53Z greg $
V y
C 8.62923e-07
I 4
PP 4
NP 2

#!Comment: #pragma tda=fcfs #pragma variance=none TDA test #1
#!User:  0:00:00.002
#!Sys:   0:00:00.000
#!Real:  0:00:00.002
#!Solver: 2 8 97 1663 32814 3.5778e+08 0

B 6
client1        :cl1             0.333333    
client2        :cl2             1           
client3        :cl3             0.666667    
server1        :s1              0.166667    
                s2              1           
                s3              0.666667    
-1

W 3
client1        :cl1             s1              5.45046     0           -1 
                -1 
client2        :cl2             s2              8.96983     0           -1 
                -1 
client3        :cl3             s3              8.67178     0           -1 
                -1 
-1

X 6
client1        :cl1             8.45046     0           -1 
                -1 
client2        :cl2             9.96983     0           -1 
                -1 
client3        :cl3             10.1718     0           -1 
                -1 
server1        :s1              1           5           -1 
                s2              0.5         0.5         -1 
                s3              0.5         1           -1 
                -1 
-1

VAR 6
client1        :cl1             71.4103     0           -1 
                -1 
client2        :cl2             99.3975     0           -1 
                -1 
client3        :cl3             103.465     0           -1 
                -1 
server1        :s1              1           25          -1 
                s2              0.25        0.25        -1 
                s3              0.25        1           -1 
                -1 
-1

FQ 4
client1        :cl1             0.118337    1           0           -1 1
                -1 
client2        :cl2             0.100303    1           0           -1 1
                -1 
client3        :cl3             0.0983112   1           0           -1 1
                -1 
server1        :s1              0.118337    0.118337    0.591684    -1 0.710021
                s2              0.100303    0.0501513   0.0501513   -1 0.100303
                s3              0.0983112   0.0491556   0.0983112   -1 0.147467
                -1 
                                0.316951    0.217644    0.740146    -1 0.95779
-1

P client1Proc 1
client1         1  0 1  cl1             0.236674    0           0           -1 
                        -1 
-1 

P client2Proc 1
client2         1  0 1  cl2             0.0501513   0           0           -1 
                        -1 
-1 

P client3Proc 1
client3         1  0 1  cl3             0.0983112   0           0           -1 
                        -1 
-1 

P server1Proc 1
server1         3  0 1  s1              0.710021    0           0           -1 
                        s2              0.100303    0           0           -1 
                        s3              0.147467    0           0           -1 
                        -1 
                                        0.95779     
-1 

-1

