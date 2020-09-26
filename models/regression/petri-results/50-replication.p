# lqn2ps 5.17
# lqn2ps -Oparseable -o50-replication.p -merge-replicas -parse-file=50-replication-flat.p 50-replication-flat.in
# $Id: 50-replication.p 13867 2020-09-25 15:10:41Z greg $
V y
C 3.42884e-06
I 0
PP 2
NP 1

#!Comment: Simplest model.
#!Real:  0:00:01.000

W 1
client         :client          server          0.500001    -1 
                -1 
-1

X 2
client         :client          2.5         -1 
                -1 
server         :server          1           -1 
                -1 
-1

VAR 2
client         :client          0           -1 
                -1 
server         :server          0           -1 
                -1 
-1

FQ 2
client         :client          0.4         1           -1 1
                -1 
server         :server          0.8         0.8         -1 0.8
                -1 
-1

P client 1
client          1  0 1  client          0.4         0           -1 
                        -1 
-1 

P server 1
server          1  0 1  server          0.8         0           -1 
                        -1 
-1 

-1

