# lqns 4.3
# lqns -Pvariance=mol -M -p -o21-multiserver.out 21-multiserver.in
V y
C 0
I 2
PP 2
NP 2
#!User:  0:00:00.00
#!Sys:   0:00:00.00
#!Real:  0:00:00.00
#!Solver:    4   0   0         28          7          0        490      122.5       24.5  0:00:00.00  0:00:00.00  0:00:00.00 
B 2
client      : client      2           
server      : server      3           
 -1

W 1
client      : client      server      0.166667    0            -1 
 -1
 -1


X 2
client      : client      1.66667     0            -1 
 -1
server      : server      0.5         0.5          -1 
 -1
 -1


VAR 2
client      : client      3.88889     0            -1 
 -1
server      : server      0.25        0.25         -1 
 -1
 -1

FQ 2
client      : client      1.8         3           0            -1 3           
 -1
server      : server      1.8         0.9         0.9          -1 1.8         
 -1
 -1

P client 1
client      1 0   3   client      1.8         0           0            -1 
 -1
 -1
P server 1
server      1 0   3   server      1.8         0           0            -1 
 -1
 -1
 -1

