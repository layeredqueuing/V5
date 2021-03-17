# lqns 5.19
# lqns --pragma=variance=mol,threads=hyper --parseable --no-warnings 09-sanity.lqnx
# $Id: 09-sanity.p 14556 2021-03-17 18:08:06Z greg $
V y
C 1.80431e-06
I 10
PP 2
NP 2

#!Comment: Most Common features.
#!User:  0:00:00.004
#!Sys:   0:00:00.000
#!Real:  0:00:00.004
#!Solver: 3 30 337 3955 21276 1.93603e+07 0

B 4
client         :client          1.82927     
server1        :entry1          1.92308     
server2        :entry2          5           
                entry3          3.33333     
-1

W 2
client         :client          entry1          0.601673     0            -1 
                -1 
server1        :entry1          entry2          0.142577     0            -1 
                -1 
-1

F 1
server1        :entry1          entry2          0.132662    
                -1 
-1

Z 1
server1        :entry1          entry3          0            0.510351     -1 
                -1 
-1

X 4
client         :client          2.5668      0           -1 
                -1 
server1        :entry1          0.725845    0.60679     -1 
                -1 
server2        :entry2          0.345906    0           -1 
                entry3          0.445906    0           -1 
                -1 
-1

VAR 4
client         :client          12.3007     0           -1 
                -1 
server1        :entry1          0.531414    0.261404    -1 
                -1 
server2        :entry2          0.0612885   0           -1 
                entry3          0.111288    0           -1 
                -1 
-1

FQ 3
client         :client          1.16877     3           0           -1 3
                -1 
server1        :entry1          1.16877     0.848344    0.709197    -1 1.55754
                -1 
server2        :entry2          0.818138    0.282999    0           -1 0.282999
                entry3          0.35063     0.156348    0           -1 0.156348
                -1 
                                1.16877     0.439347    0           -1 0.439347
-1

P client 1
client          1  0 3  client          1.16877     0           0           -1 
                        -1 
-1 

P server 2
server1         1  0 2  entry1          1.16877     0.128148    0.10679     -1 
                        -1 
server2         2  0 1  entry2          0.163628    0.145906    0           -1 
                        entry3          0.105189    0.145906    0           -1 
                        -1 
                                        0.268817    
                -1 
                                        1.43758
-1 

-1

