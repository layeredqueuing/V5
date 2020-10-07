# petrisrvn 5.18
# petrisrvn --parseable --random-queueing 09-sanity.in
# $Id: 09-sanity.p 13923 2020-10-07 14:29:28Z greg $
V y
C 8.7823e-06
I 0
PP 2
NP 2

#!Comment: Most Common features.
#!User:  0:00:00.992
#!Sys:   0:00:00.110
#!Real:  0:00:02.826

W 2
client         :client          entry1          0.277922    0           -1 
                -1 
server1        :entry1          entry2          0.071484    0           -1 
                -1 
-1

F 1
server1        :entry1          entry2          0.0780121   
                -1 
-1

Z 1
server1        :entry1          entry3          0           0.450727    -1 
                -1 
-1

DP 1
server1        :entry1          entry3          0            0            -1 
                -1 
-1

X 4
client         :client          1.97122     0           -1 
                -1 
server1        :entry1          0.554297    0.5         -1 
                -1 
server2        :entry2          0.2         0           -1 
                entry3          0.3         0           -1 
                -1 
-1

FQ 3
client         :client          1.5219      3           0           -1 3
                -1 
server1        :entry1          1.52189     0.84358     0.760945    -1 3.20905
                -1 
server2        :entry2          1.06533     0.213066    0           -1 0.213066
                entry3          0.456589    0.136977    0           -1 0.136977
                -1 
                                1.52192     0.350043    0           -1 0.350043
-1

P client 1
client          1  0 3  client          1.5219      0           0           -1 
                        -1 
-1 

P server 2
server1         1  0 2  entry1          1.52189     0           0           -1 
                        -1 
server2         2  0 1  entry2          0.213066    0           0           -1 
                        entry3          0.136977    0           0           -1 
                        -1 
                                        0.350043    
                -1 
                                        1.87193
-1 

-1

