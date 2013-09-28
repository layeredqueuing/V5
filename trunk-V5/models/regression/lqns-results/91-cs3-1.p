# lqns 3.7
# lqns -Pvariance=mol -p -ocs3-1.out cs3-1.in
V y
C 1.08144e-07
I 5
PP 4
NP 2
#!User:  0:00:00.01
#!Sys:   0:00:00.00
#!Real:  0:00:00.01
B 4
client1         : cl1             0.333333    
client2         : cl2             1           
client3         : cl3             0.666667    
server1         : s1              0.166667    
                  s2              1           
                  s3              0.666667    
 -1

W 3
client1         : cl1             s1              5.31748     0            -1 
 -1
client2         : cl2             s2              8.27258     0            -1 
 -1
client3         : cl3             s3              8.03553     0            -1 
 -1
 -1


X 6
client1         : cl1             8.31748     0            -1 
 -1
client2         : cl2             9.27258     0            -1 
 -1
client3         : cl3             9.53553     0            -1 
 -1
server1         : s1              1           5            -1 
                  s2              0.5         0.5          -1 
                  s3              0.5         1            -1 
 -1
 -1


VAR 6
client1         : cl1             168.957     0            -1 
 -1
client2         : cl2             278.376     0            -1 
 -1
client3         : cl3             273.064     0            -1 
 -1
server1         : s1              1           25           -1 
                  s2              0.25        0.25         -1 
                  s3              0.25        1            -1 
 -1
 -1

FQ 4
client1         : cl1             0.120229    1           0            -1 1           
 -1
client2         : cl2             0.107845    1           0            -1 1           
 -1
client3         : cl3             0.104871    1           0            -1 1           
 -1
server1         : s1              0.120229    0.120229    0.601143     -1 0.721372    
                  s2              0.107845    0.0539224   0.0539224    -1 0.107845    
                  s3              0.104871    0.0524355   0.104871     -1 0.157306    
 -1
                                  0.332945    0.226587    0.759937     -1 0.986523    
 -1

P client1Proc 1
client1         1 0   1   cl1             0.240457    0           0            -1 
 -1
 -1
P client2Proc 1
client2         1 0   1   cl2             0.0539224   0           0            -1 
 -1
 -1
P client3Proc 1
client3         1 0   1   cl3             0.104871    0           0            -1 
 -1
 -1
P server1Proc 1
server1         3 0   1   s1              0.721372    0           0            -1 
                          s2              0.107845    0           0            -1 
                          s3              0.157306    0           0            -1 
 -1
                                          0.986523    
 -1
 -1

