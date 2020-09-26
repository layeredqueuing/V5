# lqns 5.17
# lqns --pragma=variance=mol,threads=hyper --parseable 44-activities.in
# $Id: 44-activities.p 13856 2020-09-24 14:29:45Z greg $
V y
C 0
I 2
PP 2
NP 1

#!Comment: Activities with LOOP
#!User:  0:00:00.000
#!Sys:   0:00:00.000
#!Real:  0:00:00.000
#!Solver: 2 4 21 111 272 18934 0

B 2
client         :client          0.5         
server         :server          1           
-1

W 1
client         :client          server          0           -1 
                -1 
-1

X 2
client         :client          2           -1 
                -1 
server         :server          1          -1 
                -1
:
                done            0.25        -1 
                loop            1           -1 
                server          0.25        -1 
                -1 
-1

VAR 2
client         :client          6.875       -1 
                -1 
server         :server          1.375      -1 
                -1
:
                done            0.0625      -1 
                loop            1           -1 
                server          0.0625      -1 
                -1 
-1

FQ 2
client         :client          0.5         1           -1 1
                -1 
server         :server          0.5         0.5        -1 0.5
                -1
:
                done            0.5         0.125       -1 
                loop            0.25        0.25        -1 
                server          0.5         0.125       -1 
                -1 
-1

P client 1
client          1  0 1  client          0.5         0           -1 
                        -1 
-1 

P server 1
server          1  0 1  server          0.5         0          -1 
-1
                       :
                        done            0.125       0           -1 
                        loop            0.25        0           -1 
                        server          0.125       0           -1 
                        -1 
                                        0.5         
-1 

-1

