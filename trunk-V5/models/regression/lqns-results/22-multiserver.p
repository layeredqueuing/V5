# lqns 4.3
# lqns -Pvariance=mol -M -p -o22-multiserver.out 22-multiserver.in
V y
C 0
I 2
PP 2
NP 1
#!User:  0:00:00.00
#!Sys:   0:00:00.00
#!Real:  0:00:00.00
#!Solver:    4   0   0         24          6          1        346       86.5       11.5  0:00:00.00  0:00:00.00  0:00:00.00 
B 2
client      : client      1.5         
server      : server      3           
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
client      : client      6.5          -1 
 -1
server      : server      1            -1 
 -1
 -1

FQ 2
client      : client      1.5         3            -1 3           
 -1
server      : server      1.5         1.5          -1 1.5         
 -1
 -1

P client 1
client      1 0   3   client      1.5         0            -1 
 -1
 -1
P server 1
server      1 0   3   server      1.5         0            -1 
 -1
 -1
 -1

