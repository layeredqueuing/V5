# lqns 5.26
# lqns --pragma=variance=mol,threads=hyper --parseable 31-overtaking.in
# $Id$
V y
C 0
I 2
PP 2
NP 2

#pragma threads=hyper
#pragma variance=mol

#!Comment: Simplest model - rendezvous with overtaking.
#!User:  0:00:00.000
#!Sys:   0:00:00.000
#!Real:  0:00:00.000
#!MaxRSS: 4612096
#!Solver: 2 4 22 122 305 24973 0

B 2
t1             :e1              0.5         
t2             :e2              0.5         
-1

W 1
t1             :e1              e2              0.5          0            -1 
                -1 
-1

X 2
t1             :e1              2.5         0           -1 
                -1 
t2             :e2              1           1           -1 
                -1 
-1

VAR 2
t1             :e1              11.875      0           -1 
                -1 
t2             :e2              1           1           -1 
                -1 
-1

FQ 2
t1             :e1              0.4         1           0           -1 1
                -1 
t2             :e2              0.4         0.4         0.4         -1 0.8
                -1 
-1

P p1 1
t1              1  0 1  e1              0.4         0           0           -1 
                        -1 
-1 

P p2 1
t2              1  0 1  e2              0.8         0           0           -1 
                        -1 
-1 

-1

