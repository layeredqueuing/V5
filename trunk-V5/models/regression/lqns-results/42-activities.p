# lqns 4.4
# lqns -Pvariance=mol,threads=mak,quorum-distribution=threepoint -M -p -o42-activities.out 42-activities.in
V y
C 3.32315e-07
I 12
PP 2
NP 2
#!User:  0:00:00.09
#!Sys:   0:00:00.00
#!Real:  0:00:00.13
#!Solver:   47   0   0        333     7.0851     5.6449      22077     469.72     673.59  0:00:00.09  0:00:00.00  0:00:00.13 
B 2
client      : client      0.606061    
server      : server      0.909091    
 -1

W 1
client      : client      server      0.174413    0            -1 
 -1
 -1

J 1
server      : fork1       fork2       0.884408    0.305395    
 -1
 -1


X 2
client      : client      1.95142     0            -1 
 -1
server      : server      0.777006    0.607402     -1 
 -1
  :
            server      0.25         -1 
            fork1       0.527006     -1 
            fork2       0.674311     -1 
            join        0.25         -1 
 -1
 -1


VAR 2
client      : client      5.46756     0            -1 
 -1
server      : server      0.238631    0.191764     -1 
 -1
  :
            server      0.0625       -1 
            fork1       0.176131     -1 
            fork2       0.365522     -1 
            join        0.0625       -1 
 -1
 -1

FQ 2
client      : client      0.512448    1           0            -1 1           
 -1
server      : server      0.512448    0.398175    0.311262     -1 0.709437    
 -1
  :
            server      0.512448    0.128112    0  -1 
            fork1       0.512448    0.270063    0  -1 
            fork2       0.512448    0.345549    0  -1 
            join        0.512448    0.128112    0  -1 
 -1
 -1

P client 1
client      1 0   1   client      0.512448    0           0            -1 
 -1
 -1
P server 1
server      1 0   1   server      0.768672    0           0            -1 
 -1
          :
                    server      0.128112    0           0  -1 
                    fork1       0.204979    0.127006    0  -1 
                    fork2       0.307469    0.0743109   0  -1 
                    join        0.128112    0           0  -1 
 -1
                                  0.768672    
 -1
 -1

