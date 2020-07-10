# lqn2ps 5.14
# lqn2ps -Oparseable -o50-replication.p -merge-replicas -parse-file=50-replication-flat.p 50-replication-flat.in
# $Id: 50-replication.p 13675 2020-07-10 15:29:36Z greg $
V y
C 0.932934
I 5
PP 2
NP 1

#!Comment: Simplest model.
#!User:  0:36:21.000
#!Sys:   0:14:30.000
#!Real:  0:52:19.000

W 1
client         :client          server          0.49776     -1 
                                            %95 0.00922127  -1 
                                            %99 0.0152935   -1 
                -1 
-1

VARW 1
client         :client          server          0.745698     -1 
                                            %95 0.0194968    -1 
                                            %99 0.0323355    -1 
                -1 
-1

X 2
client         :client          2.50048     -1 
                            %95 0.0549875   -1 
                            %99 0.0911968   -1 
                -1 
server         :server          0.999922    -1 
                            %95 0.0062426   -1 
                            %99 0.0103534   -1 
                -1 
-1

VAR 2
client         :client          10.3809     -1 
                            %95 0.704441    -1 
                            %99 1.16832     -1 
                -1 
server         :server          1.00289     -1 
                            %95 0.0179108   -1 
                            %99 0.0297051   -1 
                -1 
-1

FQ 2
client         :client          0.39993     1           -1 1
                            %95 0.00877258  0           -1 0
                            %99 0.0145493   0           -1 0
                -1 
server         :server          0.800171    0.800107    -1 0.800107
                            %95 0.00435829  0.00133854  -1 0.00133854
                            %99 0.00722823  0.00221996  -1 0.00221996
                -1 
-1

P client 1
client          1  0 1  client          0.399805    0           -1 
                                    %95 0.00419389  0           -1 
                                    %99 0.00695557  0           -1 
                        -1 
-1 

P server 1
server          1  0 1  server          0.800107    0           -1 
                                    %95 0.00133854  0           -1 
                                    %99 0.00221996  0           -1 
                        -1 
-1 

-1

