# lqn2ps 5.17
# lqn2ps -Oparseable -o52-replication.p -merge-replicas -parse-file=52-replication-flat.p 52-replication-flat.in
# $Id: 52-replication.p 13867 2020-09-25 15:10:41Z greg $
V y
C 2.98023e-08
I 0
PP 2
NP 1

#!Comment: Simplified model from bug 166
#!Real:  0:00:00.000

W 1
t1             :e1              e2              0           -1 
                -1 
-1

VARW 1
t1             :e1              e2              0            -1 
                -1 
-1

DP 1
t1             :e1              e2              0            -1 
                -1 
-1

X 2
t1             :e1              3           -1 
                -1 
t2             :e2              1           -1 
                -1 
-1

VAR 2
t1             :e1              0           -1 
                -1 
t2             :e2              0           -1 
                -1 
-1

FQ 2
t1             :e1              0.333333    1           -1 1
                -1 
t2             :e2              0.333333    0.333333    -1 0.333333
                -1 
-1

P p1 1
t1              1  0 1  e1              0.333333    0           -1 
                        -1 
-1 

P p2 1
t2              1  0 1  e2              0.333333    0           -1 
                        -1 
-1 

-1

