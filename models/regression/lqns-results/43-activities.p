# lqns 5.3
# lqns -Pvariance=mol,threads=mak 43-activities.in
# $Id: 43-activities.p 13584 2020-06-02 13:06:10Z greg $
V y
C 9.57222e-07
I 25
PP 2
NP 2

#!Comment: Activities with AND fork-join - reply on branch.
#!User:  0:00:02.000
#!Real:  0:00:02.000
#!Solver: 3 75 824 19698 115377 6.04614e+08 0

W 1
client         :client          server          0.35468     0           -1 
                -1 
-1

J 0
server         :fork1           fork2           0.943616    0.323998   
-1 
-1

X 2
client         :client          2.33223     0           -1 
                -1 
server         :server          0.977554    0.798732    -1 
                -1
:
                fork1           0.561219    -1 
                fork2           0.741301    -1 
                join            0.416335    -1 
                server          0.416335    -1 
                -1 
-1

VAR 2
client         :client          8.69321     0           -1 
                -1 
server         :server          0           0           -1 
                -1
:
                fork1           0.185992    -1 
                fork2           0.379966    -1 
                join            0.0901674   -1 
                server          0.0901674   -1 
                -1 
-1

FQ 2
client         :client          0.857547    2           0           -1 2
                -1 
server         :server          0.857547    0           0           -1 1.52325
                -1
:
                fork1           0           0.481272    -1 
                fork2           0           0.6357      -1 
                join            0           0.357027    -1 
                server          0           0.357027    -1 
                -1 
-1

P client 1
client          1  0 2  client          0.857547    0           0           -1 
                        -1 
-1 

P server 1
server          1  0 2  server          1.28632     0           0           -1 
-1
                       :
                        fork1           0           0.161219    -1 
                        fork2           0           0.141301    -1 
                        join            0           0.166335    -1 
                        server          0           0.166335    -1 
                        -1 
                                        1.28632     
-1 

-1

