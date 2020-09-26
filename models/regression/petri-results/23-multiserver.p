# petrisrvn 5.17
# petrisrvn --parseable --output=23-multiserver.out 23-multiserver.in
# $Id: 23-multiserver.p 13867 2020-09-25 15:10:41Z greg $
V y
C 8.40915e-06
I 0
PP 2
NP 1

#!Comment: Use an infinite server for server.
#!User:  0:00:00.028
#!Sys:   0:00:00.045
#!Real:  0:00:00.916

W 1
client         :client          server          0           -1 
                -1 
-1

X 2
client         :client          2           -1 
                -1 
server         :server          1           -1 
                -1 
-1

FQ 2
client         :client          1.5         3           -1 3
                -1 
server         :server          1.5         1.5         -1 1.5
                -1 
-1

P client 1
client          1  0 3  client          1.5         0           -1 
                        -1 
-1 

P server 1
server          1  0 1  server          1.5         0           -1 
                        -1 
-1 

-1

