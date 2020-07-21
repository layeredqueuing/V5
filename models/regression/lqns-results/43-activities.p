# lqns 5.15
# lqns --pragma=variance=mol,threads=mak --parseable 43-activities.in
# $Id: 43-activities.p 13705 2020-07-20 21:46:53Z greg $
V y
C 7.36526e-07
I 23
PP 2
NP 2

#!Comment: Activities with AND fork-join - reply on branch.
#!User:  0:00:00.027
#!Sys:   0:00:00.000
#!Real:  0:00:00.027
#!Solver: 3 69 742 17470 102303 5.38843e+08 0

B 2
client         :client          1.08108     
server         :server          1.55844     
-1

W 1
client         :client          server          0.22994     0           -1 
                -1 
-1

J 0
server         :fork1           fork2           0.934293    0.321572   
-1 
-1

X 2
client         :client          2.37087     0           -1 
                -1 
server         :server          1.14093     0.608083    -1 
                -1
:
                fork1           0.552341    -1 
                fork2           0.733572    -1 
                join            0.407362    -1 
                server          0.407362    -1 
                -1 
-1

VAR 2
client         :client          9.06641     0           -1 
                -1 
server         :server          0.465104    0.408834    -1 
                -1
:
                fork1           0.183208    -1 
                fork2           0.377841    -1 
                join            0.0872627   -1 
                server          0.0872627   -1 
                -1 
-1

FQ 2
client         :client          0.843571    2           0           -1 2
                -1 
server         :server          0.843571    0.962458    0.512961    -1 1.47542
                -1
:
                fork1           0.843571    0.465939    -1 
                fork2           0.843571    0.61882     -1 
                join            0.843571    0.343638    -1 
                server          0.843571    0.343638    -1 
                -1 
-1

P client 1
client          1  0 2  client          0.843571    0           0           -1 
                        -1 
-1 

P server 1
server          1  0 2  server          1.26536     0           0           -1 
-1
                       :
                        fork1           0.337428    0.152341    -1 
                        fork2           0.506143    0.133572    -1 
                        join            0.210893    0.157362    -1 
                        server          0.210893    0.157362    -1 
                        -1 
                                        1.26536     
-1 

-1

