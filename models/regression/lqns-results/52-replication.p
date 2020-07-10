# lqns 5.14
# lqns --pragma=variance=mol,threads=mak --no-warnings --parseable 52-replication.lqnx
# $Id: 52-replication.p 13675 2020-07-10 15:29:36Z greg $
V y
C 0.00391427
I 2
PP 2
NP 1

#!Comment: Simplified model from bug 166
#!User:  0:00:00.000
#!Sys:   0:00:00.000
#!Real:  0:00:00.000
#!Solver: 2 6 35 205 543 50985 0

B 2
t1             :e1              0.333333    
t2             :e2              1           
-1

W 1
t1             :e1              e2              0           -1 
                -1 
-1

X 2
t1             :e1              3           -1 
                -1 
t2             :e2              1           -1 
                -1 
-1

VAR 2
t1             :e1              13.6667     -1 
                -1 
t2             :e2              1           -1 
                -1 
-1

FQ 2
t1             :e1              0.333588    1.00077     -1 1.00077
                -1 
t2             :e2              0.333588    0.333588    -1 0.333588
                -1 
-1

P p1 1
t1              1  0 1  e1              0.333588    0           -1 
                        -1 
-1 

P p2 1
t2              1  0 1  e2              0.333588    0           -1 
                        -1 
-1 

-1

