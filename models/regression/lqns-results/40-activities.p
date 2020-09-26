# lqns 5.17
# lqns --pragma=variance=mol,threads=mak --parseable 40-activities.in
# $Id: 40-activities.p 13878 2020-09-26 02:30:34Z greg $
V y
C 6.25398e-06
I 7
PP 2
NP 1

#!Comment: Activities with AND fork/join.
#!User:  0:00:00.002
#!Sys:   0:00:00.000
#!Real:  0:00:00.002
#!Solver: 3 21 132 1572 8841 1.2101e+07 0

B 2
client         :client          0.47619     
server         :server          0.779221    
-1

W 1
client         :-1
               :
                client          server          0           -1 
                -1 
-1

J 0
server         :fork1           fork2           0.836245    0.301691   
-1 
-1

X 2
client         :client          2.33624    -1 
                -1
:
                client          2.33624     -1 
                -1 
server         :server          1.33624    -1 
                -1
:
                fork1           0.466325    -1 
                fork2           0.640971    -1 
                join            0.25        -1 
                server          0.25        -1 
                -1 
-1

VAR 2
client         :client          6.5275     -1 
                -1
:
                client          6.5275      -1 
                -1 
server         :server          0.426691   -1 
                -1
:
                fork1           0.164399    -1 
                fork2           0.361679    -1 
                join            0.0625      -1 
                server          0.0625      -1 
                -1 
-1

FQ 2
client         :client          0.428037    0.999999   -1 0.999999
                -1
:
                client          0.428037    0.999999    -1 
                -1 
server         :server          0.428037    0.571962   -1 0.571962
                -1
:
                fork1           0.428037    0.199604    -1 
                fork2           0.428037    0.274359    -1 
                join            0.428037    0.107009    -1 
                server          0.428037    0.107009    -1 
                -1 
-1

P client 1
client          1  0 1  client          0.428037    0          -1 
-1
                       :
                        client          0.428037    0           -1 
                        -1 
                                        0.428037    
-1 

P server 1
server          1  0 1  server          0.642055    0          -1 
-1
                       :
                        fork1           0.171215    0.0663253   -1 
                        fork2           0.256822    0.0409707   -1 
                        join            0.107009    0           -1 
                        server          0.107009    0           -1 
                        -1 
                                        0.642055    
-1 

-1

