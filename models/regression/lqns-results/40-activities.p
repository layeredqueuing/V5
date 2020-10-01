# lqns 5.18
# lqns --pragma=variance=mol,threads=hyper --parseable 40-activities.in
# $Id: 40-activities.p 13905 2020-10-01 11:32:09Z greg $
V y
C 6.59826e-06
I 11
PP 2
NP 1

#!Comment: Activities with AND fork/join.
#!User:  0:00:00.005
#!Sys:   0:00:00.000
#!Real:  0:00:00.005
#!Solver: 3 33 234 3646 20919 6.6688e+07 0

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
server         :fork1           fork2           1.10947     0.446984   
-1 
-1

X 2
client         :client          2.60949    -1 
                -1
:
                client          2.60949     -1 
                -1 
server         :server          1.60947    -1 
                -1
:
                fork1           0.473114    -1 
                fork2           0.996654    -1 
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
server         :server          0.571984   -1 
                -1
:
                fork1           0.165345    -1 
                fork2           0.517335    -1 
                join            0.0625      -1 
                server          0.0625      -1 
                -1 
-1

FQ 2
client         :client          0.383217    1          -1 1
                -1
:
                client          0.383217    1           -1 
                -1 
server         :server          0.383217    0.616777   -1 0.616777
                -1
:
                fork1           0.383217    0.181305    -1 
                fork2           0.383217    0.381935    -1 
                join            0.383217    0.0958043   -1 
                server          0.383217    0.0958043   -1 
                -1 
-1

P client 1
client          1  0 1  client          0.383217    0          -1 
-1
                       :
                        client          0.383217    0           -1 
                        -1 
                                        0.383217    
-1 

P server 1
server          1  0 1  server          0.574826    0          -1 
-1
                       :
                        fork1           0.153287    0.0731087   -1 
                        fork2           0.22993     0.396654    -1 
                        join            0.0958043   0           -1 
                        server          0.0958043   0           -1 
                        -1 
                                        0.574826    
-1 

-1

