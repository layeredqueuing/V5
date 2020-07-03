# lqns 5.3
# lqns -Pvariance=mol,threads=mak 42-activities.in
# $Id: 42-activities.p 13584 2020-06-02 13:06:10Z greg $
V y
C 6.38819e-07
I 21
PP 2
NP 2

#!Comment: Activities with AND fork-join - reply on branch.
#!Real:  0:00:01.000
#!Solver: 3 63 413 5733 32625 8.05571e+07 0

W 1
client         :client          server          0.330792    0           -1 
                -1 
-1

J 0
server         :fork1           fork2           1.06679     0.420184   
-1 
-1

X 2
client         :client          2.02983     0           -1 
                -1 
server         :server          0.699041    0.867752    -1 
                -1
:
                fork1           0.449041    -1 
                fork2           0.954412    -1 
                join            0.25        -1 
                server          0.25        -1 
                -1 
-1

VAR 2
client         :client          6.24133     0           -1 
                -1 
server         :server          0           0           -1 
                -1
:
                fork1           0.162405    -1 
                fork2           0.485608    -1 
                join            0.0625      -1 
                server          0.0625      -1 
                -1 
-1

FQ 2
client         :client          0.492651    1           0           -1 1
                -1 
server         :server          0.492651    0           0           -1 0.771883
                -1
:
                fork1           0           0.221221    -1 
                fork2           0           0.470192    -1 
                join            0           0.123163    -1 
                server          0           0.123163    -1 
                -1 
-1

P client 1
client          1  0 1  client          0.492651    0           0           -1 
                        -1 
-1 

P server 1
server          1  0 1  server          0.738977    0           0           -1 
-1
                       :
                        fork1           0           0.0490406   -1 
                        fork2           0           0.354412    -1 
                        join            0           0           -1 
                        server          0           0           -1 
                        -1 
                                        0.738977    
-1 

-1

