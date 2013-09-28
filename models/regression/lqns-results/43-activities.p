# lqns 4.4
# lqns -Pvariance=mol,threads=mak,quorum-distribution=threepoint -M -p -o43-activities.out 43-activities.in
V y
C 3.66237e-07
I 18
PP 2
NP 2
#!User:  0:00:00.75
#!Sys:   0:00:00.00
#!Real:  0:00:00.76
#!Solver:   71   0   0       1017     14.324     12.085 1.4762e+05     2079.2     2391.7  0:00:00.75  0:00:00.00  0:00:00.76 
B 2
client      : client      1.21212     
server      : server      1.81818     
 -1

W 1
client      : client      server      0.354679    0            -1 
 -1
 -1

J 1
server      : fork1       fork2       0.943616    0.323998    
 -1
 -1


X 2
client      : client      2.33223     0            -1 
 -1
server      : server      0.977554    0.798732     -1 
 -1
  :
            server      0.416335     -1 
            fork1       0.561219     -1 
            fork2       0.7413       -1 
            join        0.416335     -1 
 -1
 -1


VAR 2
client      : client      8.69319     0            -1 
 -1
server      : server      0.276159    0.228174     -1 
 -1
  :
            server      0.0901673    -1 
            fork1       0.185992     -1 
            fork2       0.379966     -1 
            join        0.0901673    -1 
 -1
 -1

FQ 2
client      : client      0.857547    2           0            -1 2           
 -1
server      : server      0.857547    0.838299    0.68495      -1 1.52325     
 -1
  :
            server      0.857547    0.357027    0  -1 
            fork1       0.857547    0.481272    0  -1 
            fork2       0.857547    0.6357      0  -1 
            join        0.857547    0.357027    0  -1 
 -1
 -1

P client 1
client      1 0   2   client      0.857547    0           0            -1 
 -1
 -1
P server 1
server      1 0   2   server      1.28632     0           0            -1 
 -1
          :
                    server      0.214387    0.166335    0  -1 
                    fork1       0.343019    0.161219    0  -1 
                    fork2       0.514528    0.1413      0  -1 
                    join        0.214387    0.166335    0  -1 
 -1
                                  1.28632     
 -1
 -1

