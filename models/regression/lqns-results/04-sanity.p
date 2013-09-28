# lqns 4.4
# lqns -Pvariance=mol -M -p -o04-sanity.out 04-sanity.in
V y
C 0
I 2
PP 2
NP 1
#!User:  0:00:00.00
#!Sys:   0:00:00.00
#!Real:  0:00:00.00
#!Solver:    4   0   0         21       5.25    0.43301        222       55.5     9.5263  0:00:00.00  0:00:00.00  0:00:00.00 
B 2
client      : client      1           
server      : server      2           
 -1

Z 1
client      : client      server      1            -1 
 -1
 -1


X 2
client      : client      1            -1 
 -1
server      : server      0.5          -1 
 -1
 -1


VAR 2
client      : client      1            -1 
 -1
server      : server      0.25         -1 
 -1
 -1

FQ 2
client      : client      1           1            -1 1           
 -1
server      : server      1           0.5          -1 0.5         
 -1
 -1

P client 1
client      1 0   1   client      1           0            -1 
 -1
 -1
P server 1
server      1 0   1   server      0.5         0            -1 
 -1
 -1
 -1

