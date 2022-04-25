# lqns 5.21
# lqns --pragma=variance=mol,threads=hyper --parseable 18-interlock.in
# $Id: 18-interlock.p 14886 2021-07-07 18:31:56Z greg $
V y
C 0
I 2
PP 2
NP 1

#!Comment: Interlock - forwarded
#!User:  0:00:00.000
#!Sys:   0:00:00.000
#!Real:  0:00:00.000
#!Solver: 2 4 27 199 1094 466436 0

B 3
c0             :c0              0.25        
t0             :e0              1           
t1             :e1              1           
-1

W 2
c0             :c0              e0              0            -1 
                c0              e1              0            -1 
                -1 
-1

F 1
t0             :e0              e1              0           
                -1 
-1

X 3
c0             :c0              4           -1 
                -1 
t0             :e0              1           -1 
                -1 
t1             :e1              1           -1 
                -1 
-1

VAR 3
c0             :c0              25.5278     -1 
                -1 
t0             :e0              1           -1 
                -1 
t1             :e1              1           -1 
                -1 
-1

FQ 3
c0             :c0              0.25        1           -1 1
                -1 
t0             :e0              0.25        0.25        -1 0.25
                -1 
t1             :e1              0.5         0.5         -1 0.5
                -1 
-1

P c0 1
c0              1  0 1  c0              0.25        0           -1 
                        -1 
-1 

P p0 2
t0              1  0 1  e0              0.25        0           -1 
                        -1 
t1              1  0 1  e1              0.5         0           -1 
                        -1 
                -1 
                                        0.75
-1 

-1

