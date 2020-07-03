# lqns 5.9.2
# lqns --pragma=variance=mol,threads=mak --parseable 48-activities.in
V y
C 5.79159e-06
I 24
PP 1
NP 1
#!User:  0:00:00.00
#!Sys:   0:00:00.01
#!Real:  0:00:00.00
#!Solver: 1 24 266 3302 19812 3.29448e+07 0

B 1
client         :client          0.779221    
-1

J 0
client         :fork1           fork2           0.783333    0.36       
-1 
-1

X 1
client         :client          3.27597     0           -1 
                -1
:
                client          0.25        -1 
                fork1           0.402269    -1 
                fork2           2.77597     -1 
                join            0.25        -1 
                -1 
-1

VAR 1
client         :client          0.485       0           -1 
                -1
:
                client          0.0625      -1 
                fork1           0.16        -1 
                fork2           0.36        -1 
                join            0.0625      -1 
                -1 
-1

FQ 1
client         :client          2           0           0           -1 6.55193
                -1
:
                client          2           0.5         -1 
                fork1           2           0.804539    -1 
                fork2           2           5.55193     -1 
                join            2           0.5         -1 
                -1 
-1

P client 1
client          1  0 1  client          3           0           0           -1 
-1
                       :
                        client          0.5         0           -1 
                        fork1           0.8         0.00226942  -1 
                        fork2           1.2         2.17597     -1 
                        join            0.5         0           -1 
                        -1 
                                        3           
-1 

-1

