# lqns 5.19
# lqns --pragma=variance=mol,threads=hyper --no-warnings --parseable 43-activities.lqnx
# $Id: 43-activities.p 14556 2021-03-17 18:08:06Z greg $
V y
C 4.49167e-07
I 23
PP 2
NP 2

#!Comment: Activities with AND fork-join - reply on branch.
#!User:  0:00:00.037
#!Sys:   0:00:00.000
#!Real:  0:00:00.038
#!Solver: 3 69 978 30360 178779 1.38026e+09 0

B 2
client         :client          1.08108     
server         :server          1.55844     
-1

W 1
client         :client          server          0.229948     0            -1 
                -1 
-1

J 0
server         :fork1           fork2           0.934305    0.321575   
-1 
-1

X 2
client         :client          2.3709      0           -1 
                -1 
server         :server          1.14095     0.608092   -1 
                -1
:
                fork1           0.552351    -1 
                fork2           0.733584    -1 
                join            0.407371    -1 
                server          0.407371    -1 
                -1 
-1

VAR 2
client         :client          9.06669     0           -1 
                -1 
server         :server          0.46511     0.408841   -1 
                -1
:
                fork1           0.183211    -1 
                fork2           0.377845    -1 
                join            0.0872656   -1 
                server          0.0872656   -1 
                -1 
-1

FQ 2
client         :client          0.843561    2           0           -1 2
                -1 
server         :server          0.843561    0.962464    0.512963   -1 1.47543
                -1
:
                fork1           0.843561    0.465941    -1 
                fork2           0.843561    0.618822    -1 
                join            0.843561    0.343642    -1 
                server          0.843561    0.343642    -1 
                -1 
-1

P client 1
client          1  0 2  client          0.843561    0           0           -1 
                        -1 
-1 

P server 1
server          1  0 2  server          1.26534     0           0          -1 
-1
                       :
                        fork1           0.337424    0.152351    -1 
                        fork2           0.506136    0.133584    -1 
                        join            0.21089     0.157371    -1 
                        server          0.21089     0.157371    -1 
                        -1 
                                        1.26534     
-1 

-1

