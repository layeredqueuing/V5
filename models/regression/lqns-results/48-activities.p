# lqns 5.18
# lqns --pragma=variance=mol,threads=hyper --parseable 48-activities.in
# $Id: 48-activities.p 13905 2020-10-01 11:32:09Z greg $
V y
C 4.43442e-06
I 13
PP 1
NP 1

#!Comment: Activities with AND fork/join.
#!User:  0:00:00.004
#!Sys:   0:00:00.000
#!Real:  0:00:00.004
#!Solver: 2 26 193 3323 19938 5.20305e+07 0

B 1
client         :client          0.779221    
-1

J 0
client         :fork1           fork2           1.11352     0.444281   
-1 
-1

X 1
client         :client          1.61352    -1 
                -1
:
                client          0.25        -1 
                fork1           0.486778    -1 
                fork2           0.995584    -1 
                join            0.25        -1 
                -1 
-1

VAR 1
client         :client          0.569281   -1 
                -1
:
                client          0.0625      -1 
                fork1           0.16753     -1 
                fork2           0.516487    -1 
                join            0.0625      -1 
                -1 
-1

FQ 1
client         :client          0.619763    0.999998   -1 0.999998
                -1
:
                client          0.619763    0.154941    -1 
                fork1           0.619763    0.301687    -1 
                fork2           0.619763    0.617027    -1 
                join            0.619763    0.154941    -1 
                -1 
-1

P client 1
client          1  0 1  client          0.929645    0          -1 
-1
                       :
                        client          0.154941    0           -1 
                        fork1           0.247905    0.0867771   -1 
                        fork2           0.371858    0.395584    -1 
                        join            0.154941    0           -1 
                        -1 
                                        0.929645    
-1 

-1

