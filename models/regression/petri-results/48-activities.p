# petrisrvn 6.0
# petrisrvn --parseable --output=48-activities.out 48-activities.in
# $Id: 48-activities.p 13867 2020-09-25 15:10:41Z greg $
V y
C 0
I 0
PP 1
NP 2

#!Comment: Activities with AND fork/join.
#!User:  0:00:00.120
#!Sys:   0:00:00.149
#!Real:  0:00:01.448

J 0
client         :fork1           fork2           0.999999    0          
-1 
-1

X 1
client         :client          1.5         0           -1 
                -1
:
                client          0.25        -1 
                fork1           0.7         -1 
                fork2           0.8         -1 
                join            0.25        -1 
                -1 
-1

FQ 1
client         :client          0.666667    1           0           -1 1
                -1
:
                client          0.666667    0.166667    -1 
                fork1           0.666667    0.466667    -1 
                fork2           0.666667    0.533333    -1 
                join            0.666667    0.166667    -1 
                -1 
-1

P client 1
client          1  0 1  client          1           0           0           -1 
-1
                       :
                        client          0.166667    0           -1 
                        fork1           0.266667    0.3         -1 
                        fork2           0.4         0.2         -1 
                        join            0.166667    0           -1 
                        -1 
                                        1           
-1 

-1

