# lqns 5.17
# lqns --pragma=variance=mol,threads=mak --parseable 42-activities.in
# $Id: 42-activities.p 13878 2020-09-26 02:30:34Z greg $
V y
C 9.4888e-07
I 8
PP 2
NP 2

#!Comment: Activities with AND fork-join - reply on branch.
#!User:  0:00:00.003
#!Sys:   0:00:00.000
#!Real:  0:00:00.003
#!Solver: 3 24 147 1689 9435 1.23489e+07 0

B 2
client         :client          0.540541    
server         :server          0.779221    
-1

W 1
client         :client          server          0.195042    0           -1 
                -1 
-1

J 0
server         :fork1           fork2           0.843813    0.301964   
-1 
-1

X 2
client         :client          2.0915      0           -1 
                -1 
server         :server          0.896455    0.447359   -1 
                -1
:
                fork1           0.475829    -1 
                fork2           0.646455    -1 
                join            0.25        -1 
                server          0.25        -1 
                -1 
-1

VAR 2
client         :client          6.75709     0           -1 
                -1 
server         :server          0.424658    0.364464   -1 
                -1
:
                fork1           0.16575     -1 
                fork2           0.362158    -1 
                join            0.0625      -1 
                server          0.0625      -1 
                -1 
-1

FQ 2
client         :client          0.478127    1           0           -1 1
                -1 
server         :server          0.478127    0.428619    0.213894   -1 0.642513
                -1
:
                fork1           0.478127    0.227506    -1 
                fork2           0.478127    0.309087    -1 
                join            0.478127    0.119532    -1 
                server          0.478127    0.119532    -1 
                -1 
-1

P client 1
client          1  0 1  client          0.478127    0           0           -1 
                        -1 
-1 

P server 1
server          1  0 1  server          0.71719     0           0          -1 
-1
                       :
                        fork1           0.191251    0.0758294   -1 
                        fork2           0.286876    0.0464545   -1 
                        join            0.119532    0           -1 
                        server          0.119532    0           -1 
                        -1 
                                        0.71719     
-1 

-1

