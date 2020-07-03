# petrisrvn 6.0
# petrisrvn --parseable --output=52-replication.out 52-replication.in
# $Id$
V y
C 2.98023e-08
I 0
PP 3
NP 1

#!Comment: Simplified model from bug 166
#!User:  0:00:00.185
#!Sys:   0:00:00.234
#!Real:  0:00:01.898

W 2
t1             :e1              e2_1            0           -1 
                e1              e2_2            0           -1 
                -1 
-1

X 3
t1             :e1              3           -1 
                -1 
t2_1           :e2_1            1           -1 
                -1 
t2_2           :e2_2            1           -1 
                -1 
-1

FQ 3
t1             :e1              0.333333    1           -1 1
                -1 
t2_1           :e2_1            0.333333    0.333333    -1 0.333333
                -1 
t2_2           :e2_2            0.333333    0.333333    -1 0.333333
                -1 
-1

P p1 1
t1              1  0 1  e1              0.333333    0           -1 
                        -1 
-1 

P p2_1 1
t2_1            1  0 1  e2_1            0.333333    0           -1 
                        -1 
-1 

P p2_2 1
t2_2            1  0 1  e2_2            0.333333    0           -1 
                        -1 
-1 

-1

