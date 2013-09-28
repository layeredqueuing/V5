# lqns 5.7
# lqns -?pragma=variance=mol,threads=mak -?parseable 09-sanity.in
V y
C 1.51945e-06
I 10
PP 2
NP 2
#!User:  0:00:00.00
#!Sys:   0:00:00.00
#!Real:  0:00:00.00
#!Solver: 3 30 267 2649 13896 1.03413e+07 0

B 4
client         :client          1.82927     
server1        :entry1          1.92308     
server2        :entry2          5           
                entry3          3.33333     
-1

W 0
client         :client          entry1          0.604751    0           -1 
                -1 
server1        :entry1          entry2          0.114732    0           -1 
                -1 
-1

Z 0
server1        :entry1          entry3          0           0.513331    -1 
                -1 
-1

X 4
client         :client          2.55559     0           -1 
                -1 
server1        :entry1          0.722403    0.608217    -1 
                -1 
server2        :entry2          0.347979    0           -1 
                entry3          0.447979    0           -1 
                -1 
-1

VAR 4
client         :client          12.1959     0           -1 
                -1 
server1        :entry1          0.513562    0.261711    -1 
                -1 
server2        :entry2          0.0618978   0           -1 
                entry3          0.111898    0           -1 
                -1 
-1

FQ 3
client         :client          1.1739      3           0           -1 3
                -1 
server1        :entry1          1.1739      0.848025    0.713983    -1 1.56201
                -1 
server2        :entry2          0.821728    0.285944    0           -1 0.285944
                entry3          0.352169    0.157764    0           -1 0.157764
                -1 
                                1.1739      0.443708    0           -1 0.443708
-1

P client 1
client          1  0 3  client          1.1739      0           0           -1 
                        -1 
-1 

P server 2
server1         1  0 2  entry1          1.1739      0.12986     0.108217    -1 
                        -1 
server2         2  0 1  entry2          0.164346    0.147979    0           -1 
                        entry3          0.105651    0.147979    0           -1 
                        -1 
                                        0.269996    
                -1 
                                        1.44389
-1 

-1

