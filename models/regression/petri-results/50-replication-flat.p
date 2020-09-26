# petrisrvn 5.17
# petrisrvn --parseable 50-replication-flat.in
# $Id: 50-replication-flat.p 13867 2020-09-25 15:10:41Z greg $
V y
C 3.42884e-06
I 0
PP 6
NP 1

#!Comment: Simplest model.
#!User:  0:00:00.070
#!Sys:   0:00:00.052
#!Real:  0:00:01.012

W 4
client_1       :client_1        server_1        0.500001    -1 
                -1 
client_2       :client_2        server_2        0.500001    -1 
                -1 
client_3       :client_3        server_1        0.500001    -1 
                -1 
client_4       :client_4        server_2        0.500001    -1 
                -1 
-1

X 6
client_1       :client_1        2.5         -1 
                -1 
client_2       :client_2        2.5         -1 
                -1 
client_3       :client_3        2.5         -1 
                -1 
client_4       :client_4        2.5         -1 
                -1 
server_1       :server_1        1           -1 
                -1 
server_2       :server_2        1           -1 
                -1 
-1

FQ 6
client_1       :client_1        0.4         1           -1 1
                -1 
client_2       :client_2        0.4         1           -1 1
                -1 
client_3       :client_3        0.4         1           -1 1
                -1 
client_4       :client_4        0.4         1           -1 1
                -1 
server_1       :server_1        0.8         0.8         -1 0.8
                -1 
server_2       :server_2        0.8         0.8         -1 0.8
                -1 
-1

P client_1 1
client_1        1  0 1  client_1        0.4         0           -1 
                        -1 
-1 

P client_2 1
client_2        1  0 1  client_2        0.4         0           -1 
                        -1 
-1 

P client_3 1
client_3        1  0 1  client_3        0.4         0           -1 
                        -1 
-1 

P client_4 1
client_4        1  0 1  client_4        0.4         0           -1 
                        -1 
-1 

P server_1 1
server_1        1  0 1  server_1        0.8         0           -1 
                        -1 
-1 

P server_2 1
server_2        1  0 1  server_2        0.8         0           -1 
                        -1 
-1 

-1

