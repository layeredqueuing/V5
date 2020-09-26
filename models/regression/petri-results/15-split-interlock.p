# petrisrvn 5.17
# petrisrvn --parseable --output=15-split-interlock.out 15-split-interlock.in
# $Id: 15-split-interlock.p 13867 2020-09-25 15:10:41Z greg $
V y
C 2.98023e-08
I 0
PP 2
NP 1

#!Comment: Split Interlock on processor
#!User:  0:00:00.062
#!Sys:   0:00:00.044
#!Real:  0:00:01.006

W 2
t0             :e0              e1              0           -1 
                e0              e2              0           -1 
                -1 
-1

X 3
t0             :e0              3           -1 
                -1 
t1             :e1              1           -1 
                -1 
t2             :e2              1           -1 
                -1 
-1

FQ 3
t0             :e0              0.333333    1           -1 1
                -1 
t1             :e1              0.333333    0.333333    -1 0.333333
                -1 
t2             :e2              0.333333    0.333333    -1 0.333333
                -1 
-1

P p0 1
t0              1  0 1  e0              0.333333    0           -1 
                        -1 
-1 

P p1 2
t1              1  0 1  e1              0.333333    0           -1 
                        -1 
t2              1  0 1  e2              0.333333    0           -1 
                        -1 
                -1 
                                        0.666667
-1 

-1

