# lqns 5.18
# lqns --pragma=variance=mol,threads=mak --parseable 09-sanity.in
# $Id: 09-sanity.p 13899 2020-09-30 13:07:24Z greg $
V y
C 2.3976e-06
I 10
PP 2
NP 2

#!Comment: Most Common features.
#!User:  0:00:00.003
#!Sys:   0:00:00.000
#!Real:  0:00:00.003
#!Solver: 3 30 286 2990 15942 1.23688e+07 0

B 4
client         :client          1.82927     
server1        :entry1          1.92308     
server2        :entry2          5           
                entry3          3.33333     
-1

W 2
client         :client          entry1          0.601684    0           -1 
                -1 
server1        :entry1          entry2          0.142575    0           -1 
                -1 
-1

F 1
server1        :entry1          entry2          0.13266     
                -1 
-1

Z 1
server1        :entry1          entry3          0           0.510349    -1 
                -1 
-1

X 4
client         :client          2.56681     0           -1 
                -1 
server1        :entry1          0.725843    0.606789    -1 
                -1 
server2        :entry2          0.345904    0           -1 
                entry3          0.445904    0           -1 
                -1 
-1

VAR 4
client         :client          12.3008     0           -1 
                -1 
server1        :entry1          0.531411    0.261404    -1 
                -1 
server2        :entry2          0.061288    0           -1 
                entry3          0.111288    0           -1 
                -1 
-1

FQ 3
client         :client          1.16877     3           0           -1 3
                -1 
server1        :entry1          1.16876     0.84834     0.709194    -1 1.55753
                -1 
server2        :entry2          0.818136    0.282997    0           -1 0.282997
                entry3          0.350629    0.156347    0           -1 0.156347
                -1 
                                1.16877     0.439344    0           -1 0.439344
-1

P client 1
client          1  0 3  client          1.16877     0           0           -1 
                        -1 
-1 

P server 2
server1         1  0 2  entry1          1.16876     0.128147    0.106789    -1 
                        -1 
server2         2  0 1  entry2          0.163627    0.145904    0           -1 
                        entry3          0.105189    0.145904    0           -1 
                        -1 
                                        0.268816    
                -1 
                                        1.43758
-1 

-1

