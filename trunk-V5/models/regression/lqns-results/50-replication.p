# lqns 4.3
# lqns -Pvariance=mol -M -p -o50-replication.out 50-replication.in
V y
C 3.60727e-07
I 2
PP 2
NP 1
#!User:  0:00:00.01
#!Sys:   0:00:00.00
#!Real:  0:00:00.00
#!Solver:   10   0   0        149       14.9     4.9082      14478     1447.8     840.14  0:00:00.06  0:00:00.00  0:00:00.00 
B 2
client      : client      0.5         
server      : server      1           
 -1

W 1
client      : client      server      0.5          -1 
 -1
 -1


X 2
client      : client      2.5          -1 
 -1
server      : server      1            -1 
 -1
 -1


VAR 2
client      : client      11.875       -1 
 -1
server      : server      1            -1 
 -1
 -1

FQ 2
client      : client      0.4         1            -1 1           
 -1
server      : server      0.8         0.8          -1 0.8         
 -1
 -1

P client 1
client      1 0   4   client      0.4         0            -1 
 -1
 -1
P server 1
server      1 0   2   server      0.8         0            -1 
 -1
 -1
 -1

