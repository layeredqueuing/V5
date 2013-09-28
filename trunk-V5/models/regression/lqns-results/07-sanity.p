# lqns 4.4
# lqns -Pvariance=mol -M -p -o07-sanity.out 07-sanity.in
V y
C 0
I 2
PP 2
NP 1
#!User:  0:00:00.00
#!Sys:   0:00:00.00
#!Real:  0:00:00.00
#!Solver:    4   0   0         21       5.25    0.43301        333      83.25     14.289  0:00:00.00  0:00:00.00  0:00:00.00 
B 2
client      : client      0.5         
server      : server      1           
 -1

W 1
client      : client      server      0            -1 
 -1
 -1


X 2
client      : client      2            -1 
 -1
server      : server      1            -1 
 -1
 -1


VAR 2
client      : client      6.875        -1 
 -1
server      : server      1.375        -1 
 -1
 -1

FQ 2
client      : client      0.5         1            -1 1           
 -1
server      : server      0.5         0.5          -1 0.5         
 -1
 -1

P client 1
client      1 0   1   client      0.5         0            -1 
 -1
 -1
P server 1
server      1 0   1   server      0.25        0            -1 
 -1
 -1
 -1

