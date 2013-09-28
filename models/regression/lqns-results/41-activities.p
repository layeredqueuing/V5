# lqns 4.4
# lqns -Pvariance=mol,threads=mak,quorum-distribution=threepoint -M -p -o41-activities.out 41-activities.in
V y
C 0
I 2
PP 2
NP 1
#!User:  0:00:00.00
#!Sys:   0:00:00.01
#!Real:  0:00:00.01
#!Solver:    4   0   0         21       5.25    0.43301        272         68     10.464  0:00:00.00  0:00:00.00  0:00:00.00 
B 2
client      : client      0.487805    
server      : server      0.952381    
 -1

W 1
client      : client      server      0            -1 
 -1
 -1


X 2
client      : client      2.05         -1 
 -1
server      : server      1.05         -1 
 -1
  :
            server      0.25         -1 
            fork1       0.4          -1 
            fork2       0.6          -1 
            join        0.25         -1 
 -1
 -1


VAR 2
client      : client      6.22375      -1 
 -1
server      : server      0.321562     -1 
 -1
  :
            server      0.0625       -1 
            fork1       0.16         -1 
            fork2       0.36         -1 
            join        0.0625       -1 
 -1
 -1

FQ 2
client      : client      0.487805    1            -1 1           
 -1
server      : server      0.487805    0.512195     -1 0.512195    
 -1
  :
            server      0.487805    0.121951     -1 
            fork1       0.121951    0.0487805    -1 
            fork2       0.365854    0.219512     -1 
            join        0.487805    0.121951     -1 
 -1
 -1

P client 1
client      1 0   1   client      0.487805    0            -1 
 -1
 -1
P server 1
server      1 0   1   server      0.512195    0            -1 
 -1
          :
                    server      0.121951    0            -1 
                    fork1       0.0487805   0            -1 
                    fork2       0.219512    0            -1 
                    join        0.121951    0            -1 
 -1
                                  0.512195    
 -1
 -1

