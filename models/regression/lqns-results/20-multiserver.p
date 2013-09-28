# lqns 4.3
# lqns -Pvariance=mol -M -p -o20-multiserver.out 20-multiserver.in
V y
C 1.16611e-06
I 4
PP 2
NP 1
#!User:  0:00:00.00
#!Sys:   0:00:00.00
#!Real:  0:00:00.00
#!Solver:    8   0   0         81     10.125     3.4799       2555     319.38     258.35  0:00:00.00  0:00:00.00  0:00:00.00 
B 2
client      : client      2           
server      : server      2           
 -1

W 1
client      : client      server      0.626858     -1 
 -1
 -1


X 2
client      : client      2.62686      -1 
 -1
server      : server      1            -1 
 -1
 -1


VAR 2
client      : client      13.5171      -1 
 -1
server      : server      1            -1 
 -1
 -1

FQ 2
client      : client      1.52273     4            -1 4           
 -1
server      : server      1.52273     1.52273      -1 1.52273     
 -1
 -1

P client 1
client      1 0   4   client      1.52273     0            -1 
 -1
 -1
P server 1
server      1 0   2   server      1.52273     0            -1 
 -1
 -1
 -1

