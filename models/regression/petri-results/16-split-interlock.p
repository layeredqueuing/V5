# petrisrvn 5.17
# petrisrvn --parseable --output=16-split-interlock.out 16-split-interlock.in
# $Id: 16-split-interlock.p 13867 2020-09-25 15:10:41Z greg $
V y
C 4.93652e-06
I 0
PP 2
NP 2

#!Comment: Split interlock with second phase (BUG 697)
#!User:  0:00:00.056
#!Sys:   0:00:00.059
#!Real:  0:00:00.984

W 2
c0             :c0              s0              0.672412    0.814657    -1 
                c0              s1              0           0           -1 
                -1 
-1

X 3
c0             :c0              8.64232     2.81466     -1 
                -1 
s0             :s0              1           1.875       -1 
                -1 
s1             :s1              3.48491     0           -1 
                -1 
-1

FQ 3
c0             :c0              0.087283    0.754328    0.245672    -1 1
                -1 
s0             :s0              0.174567    0.174567    0.327313    -1 0.50188
                -1 
s1             :s1              0.174568    0.608353    0           -1 0.608353
                -1 
-1

P client 1
c0              1  0 1  c0              0.0872833   0           0           -1 
                        -1 
-1 

P server 2
s0              1  0 1  s0              0.436417    0           0.375002    -1 
                        -1 
s1              1  0 1  s1              0.523703    0.484915    0           -1 
                        -1 
                -1 
                                        0.960121
-1 

-1

