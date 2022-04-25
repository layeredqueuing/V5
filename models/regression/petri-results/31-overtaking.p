# petrisrvn 5.26
# petrisrvn --parseable --output=31-overtaking.out 31-overtaking.in
# $Id: 31-overtaking.p 15541 2022-04-17 16:25:43Z greg $
V y
C 0
I 0
PP 2
NP 2

#!Comment: Simplest model - rendezvous with overtaking.
#!User:  0:00:00.025
#!Sys:   0:00:00.017
#!Real:  0:00:00.048
#!MaxRSS: 10532

W 1
t1             :e1              e2              0.5          0            -1 
                -1 
-1

X 2
t1             :e1              2.5         0           -1 
                -1 
t2             :e2              1           1           -1 
                -1 
-1

FQ 2
t1             :e1              0.4         1           0           -1 1
                -1 
t2             :e2              0.4         0.4         0.4         -1 0.8
                -1 
-1

P p1 1
t1              1  0 1  e1              0.4         0           0           -1 
                        -1 
-1 

P p2 1
t2              1  0 1  e2              0.8         0           0           -1 
                        -1 
-1 

-1

