# lqns 5.15
# lqns --pragma=variance=mol,threads=mak --parseable 40-activities.in
# $Id: 40-activities.p 13705 2020-07-20 21:46:53Z greg $
V y
C 3.97164e-06
I 12
PP 2
NP 1

#!Comment: Activities with AND fork/join.
#!User:  0:00:00.003
#!Sys:   0:00:00.000
#!Real:  0:00:00.003
#!Solver: 3 36 274 4832 27927 1.11365e+08 0

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
server         :fork1           fork2           1.10947     0.446985   
-1 
-1

X 2
client         :client          2.60948     -1 
                -1
:
                client          2.60948     -1 
                -1 
server         :server          1.60947     -1 
                -1
:
                fork1           0.473104    -1 
                fork2           0.996654    -1 
                join            0.25        -1 
                server          0.25        -1 
                -1 
-1

VAR 2
client         :client          6.5275      -1 
                -1
:
                client          6.5275      -1 
                -1 
server         :server          0.571985    -1 
                -1
:
                fork1           0.165344    -1 
                fork2           0.517335    -1 
                join            0.0625      -1 
                server          0.0625      -1 
                -1 
-1

FQ 2
client         :client          0.383219    1           -1 1
                -1
:
                client          0.383219    1           -1 
                -1 
server         :server          0.383219    0.616778    -1 0.616778
                -1
:
                fork1           0.383219    0.181302    -1 
                fork2           0.383219    0.381937    -1 
                join            0.383219    0.0958047   -1 
                server          0.383219    0.0958047   -1 
                -1 
-1

P client 1
client          1  0 1  client          0.383219    0           -1 
-1
                       :
                        client          0.383219    0           -1 
                        -1 
                                        0.383219    
-1 

P server 1
server          1  0 1  server          0.574828    0           -1 
-1
                       :
                        fork1           0.153287    0.0731013   -1 
                        fork2           0.229931    0.396654    -1 
                        join            0.0958047   0           -1 
                        server          0.0958047   0           -1 
                        -1 
                                        0.574828    
-1 

-1

