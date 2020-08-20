# lqn2ps 5.16
# lqn2ps -Oparseable -o50-replication.p -merge-replicas -parse-file=50-replication-flat.p 50-replication-flat.in
# $Id: 50-replication.p 13764 2020-08-17 19:50:05Z greg $
V y
C 0.395506
I 3
PP 2
NP 1

#!Comment: Simplest model.
#!User:  2:15:31.000
#!Sys:   0:54:32.000
#!Real:  3:32:48.000

B 2
client         :client          0.5         
server         :server          1           
-1

W 1
client         :client          server          0.499179    -1 
                                            %95 0.00805296  -1 
                                            %99 0.0185744   -1 
                -1 
-1

VARW 1
client         :client          server          0.750752     -1 
                                            %95 0.016806     -1 
                                            %99 0.0387635    -1 
                -1 
-1

X 2
client         :client          2.50025     -1 
                            %95 0.0207747   -1 
                            %99 0.0479175   -1 
                -1 
server         :server          0.999928    -1 
                            %95 0.00498803  -1 
                            %99 0.011505    -1 
                -1 
-1

VAR 2
client         :client          10.3891     -1 
                            %95 0.28707     -1 
                            %99 0.662135    -1 
                -1 
server         :server          1.00121     -1 
                            %95 0.0218322   -1 
                            %99 0.0503567   -1 
                -1 
-1

FQ 2
client         :client          0.399961    1           -1 1
                            %95 0.00332088  0           -1 0
                            %99 0.00765971  0           -1 0
                -1 
server         :server          0.800235    0.800177    -1 0.800177
                            %95 0.00282415  0.00129523  -1 0.00129523
                            %99 0.00651398  0.00298749  -1 0.00298749
                -1 
-1

P client 1
client          1  0 1  client          0.399794    0           -1 
                                    %95 0.00270641  0           -1 
                                    %99 0.00624242  0           -1 
                        -1 
-1 

P server 1
server          1  0 1  server          0.800177    0           -1 
                                    %95 0.00129523  0           -1 
                                    %99 0.00298749  0           -1 
                        -1 
-1 

-1

