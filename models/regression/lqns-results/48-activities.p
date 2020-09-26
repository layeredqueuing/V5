# lqns 5.17
# lqns --pragma=variance=mol,threads=mak --parseable 48-activities.in
# $Id: 48-activities.p 13878 2020-09-26 02:30:34Z greg $
V y
C 6.1881e-06
I 10
PP 1
NP 1

#!Comment: Activities with AND fork/join.
#!User:  0:00:00.002
#!Sys:   0:00:00.000
#!Real:  0:00:00.002
#!Solver: 2 20 138 2112 12672 2.3792e+07 0

B 1
client         :client          0.779221    
-1

J 0
client         :fork1           fork2           0.887093    0.305907   
-1 
-1

X 1
client         :client          1.38709    -1 
                -1
:
                client          0.25        -1 
                fork1           0.529577    -1 
                fork2           0.676683    -1 
                join            0.25        -1 
                -1 
-1

VAR 1
client         :client          0.430907   -1 
                -1
:
                client          0.0625      -1 
                fork1           0.17679     -1 
                fork2           0.36588     -1 
                join            0.0625      -1 
                -1 
-1

FQ 1
client         :client          0.720933    1          -1 1
                -1
:
                client          0.720933    0.180233    -1 
                fork1           0.720933    0.38179     -1 
                fork2           0.720933    0.487843    -1 
                join            0.720933    0.180233    -1 
                -1 
-1

P client 1
client          1  0 1  client          1.0814      0          -1 
-1
                       :
                        client          0.180233    0           -1 
                        fork1           0.288373    0.129578    -1 
                        fork2           0.43256     0.0766828   -1 
                        join            0.180233    0           -1 
                        -1 
                                        1.0814      
-1 

-1

