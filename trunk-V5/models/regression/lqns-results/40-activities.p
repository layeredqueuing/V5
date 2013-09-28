# lqns 5.4
# lqns --pragma=variance=mol,threads=mak --parseable 40-activities.in
V y
C 5.64197e-06
I 12
PP 2
NP 1
#!User:  0:00:00.00
#!Sys:   0:00:00.00
#!Real:  0:00:00.00
#!Solver: 3 36 274 4832 27927 1.11365e+08 0

B 2
client         :client          0.47619     
server         :server          0.779221    
-1

W 0
client         :-1
               :
                client          server          8.91325e-06 -1 
  -1 
-1

J 0
server         :fork1           fork2           1.10947     0.446985   
-1 
-1

X 2
client         :client          2.60948     0           0           -1 
                -1
:
                client          2.60948     -1 
  -1 
server         :server          1.60947     0           0           -1 
                -1
:
                fork1           0.473104    -1 
                fork2           0.996654    -1 
                join            0.25        -1 
                server          0.25        -1 
  -1 
-1

VAR 2
client         :client          0           0           0           -1 
                -1
:
                client          6.5275      -1 
  -1 
server         :server          0           0           0           -1 
                -1
:
                fork1           0.165344    -1 
                fork2           0.517335    -1 
                join            0.0625      -1 
                server          0.0625      -1 
  -1 
-1

FQ 2
client         :client          0.383219    0           0           0           -1 1          
                -1
:
                client          0.383219    1           -1 
  -1 
server         :server          0.383219    0           0           0           -1 0.616778   
                -1
:
                fork1           0.383219    0.181302    -1 
                fork2           0.383219    0.381937    -1 
                join            0.383219    0.0958047   -1 
                server          0.383219    0.0958047   -1 
  -1 
-1

P client 1
client          1  0 1  client          0.383219    0           0           0           -1 
-1
                       :
                        client          0.383219    0           -1 
                        -1 
                                        0.383219    
  -1 

P server 1
server          1  0 1  server          0.574828    0           0           0           -1 
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

