# lqns 5.15
# lqns --pragma=variance=mol,threads=mak --parseable 42-activities.in
# $Id: 42-activities.p 13705 2020-07-20 21:46:53Z greg $
V y
C 6.24973e-07
I 13
PP 2
NP 2

#!Comment: Activities with AND fork-join - reply on branch.
#!User:  0:00:00.004
#!Sys:   0:00:00.000
#!Real:  0:00:00.004
#!Solver: 3 39 282 4588 26355 9.00482e+07 0

B 2
client         :client          0.540541    
server         :server          0.779221    
-1

W 1
client         :client          server          0.235182    0           -1 
                -1 
-1

J 0
server         :fork1           fork2           1.10975     0.446795   
-1 
-1

X 2
client         :client          2.48177     0           -1 
                -1 
server         :server          1.24659     0.363168    -1 
                -1
:
                fork1           0.474065    -1 
                fork2           0.996586    -1 
                join            0.25        -1 
                server          0.25        -1 
                -1 
-1

VAR 2
client         :client          10.1845     0           -1 
                -1 
server         :server          0.579781    0.509295    -1 
                -1
:
                fork1           0.165486    -1 
                fork2           0.517281    -1 
                join            0.0625      -1 
                server          0.0625      -1 
                -1 
-1

FQ 2
client         :client          0.402939    1           0           -1 1
                -1 
server         :server          0.402939    0.502298    0.146334    -1 0.648632
                -1
:
                fork1           0.402939    0.191019    -1 
                fork2           0.402939    0.401563    -1 
                join            0.402939    0.100735    -1 
                server          0.402939    0.100735    -1 
                -1 
-1

P client 1
client          1  0 1  client          0.402939    0           0           -1 
                        -1 
-1 

P server 1
server          1  0 1  server          0.604408    0           0           -1 
-1
                       :
                        fork1           0.161175    0.0740646   -1 
                        fork2           0.241763    0.396586    -1 
                        join            0.100735    0           -1 
                        server          0.100735    0           -1 
                        -1 
                                        0.604408    
-1 

-1

