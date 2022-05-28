# lqns 5.26
# lqns --pragma=variance=mol,threads=hyper --parseable 16-split-interlock.in
# $Id: 16-split-interlock.p 15603 2022-05-27 18:30:56Z greg $
V y
C 4.13932e-07
I 8
PP 2
NP 2

#pragma threads=hyper
#pragma variance=mol

#!Comment: Split interlock with second phase (BUG 697)
#!User:  0:00:00.001
#!Sys:   0:00:00.000
#!Real:  0:00:00.001
#!MaxRSS: 11976
#!Solver: 2 16 119 1025 5684 4.07192e+06 0

B 3
c0             :c0              0.111111    
s0             :s0              0.4         
s1             :s1              0.333333    
-1

W 2
c0             :c0              s0              1.10085      1.34499      -1 
                c0              s1              0            0            -1 
                -1 
-1

X 3
c0             :c0              9.69273     4.04536     -1 
                -1 
s0             :s0              1.70038     2.20038     -1 
                -1 
s1             :s1              3.44575     0           -1 
                -1 
-1

VAR 3
c0             :c0              161.041     34.9134     -1 
                -1 
s0             :s0              1.49053     2.74053     -1 
                -1 
s1             :s1              9.19869     0           -1 
                -1 
-1

FQ 3
c0             :c0              0.0727903   0.705537    0.294463    -1 1
                -1 
s0             :s0              0.145581    0.247542    0.320332    -1 0.567874
                -1 
s1             :s1              0.145581    0.501635    0           -1 0.501635
                -1 
-1

P client 1
c0              1  0 1  c0              0.0727903   0           0           -1 
                        -1 
-1 

P server 2
s0              1  0 1  s0              0.363952    0.700377    0.700377    -1 
                        -1 
s1              1  0 1  s1              0.436742    0.445752    0           -1 
                        -1 
                -1 
                                        0.800693
-1 

-1

