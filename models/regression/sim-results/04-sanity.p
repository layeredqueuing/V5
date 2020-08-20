# lqsim 5.16
# lqsim --confidence=1.0,1000 --seed=1049217653 --parseable --output=04-sanity.p 04-sanity.in
# $Id: 04-sanity.p 13764 2020-08-17 19:50:05Z greg $
V y
C 0.817916
I 4
PP 2
NP 1

#!Comment: Simplest model - send-no-reply.
#!User:  0:13:01.000
#!Sys:   0:04:55.000
#!Real:  0:18:44.000

B 2
client         :client          1           
server         :server          2           
-1

Z 1
client         :client          server          1.00315     -1 
                                            %95 0.0197034   -1 
                                            %99 0.0361683   -1 
                -1 
-1

VARZ 1
client         :client          server          1.00604      -1 
                                            %95 0.0355967    -1 
                                            %99 0.0653427    -1 
                -1 
-1

DP 1
client         :client          server          0            -1 
                                            %95 0            -1 
                                            %99 0            -1 
                -1 
-1

X 2
client         :client          0.99966     -1 
                            %95 0.00882557  -1 
                            %99 0.0162005   -1 
                -1 
server         :server          0.50038     -1 
                            %95 0.0037396   -1 
                            %99 0.00686455  -1 
                -1 
-1

VAR 2
client         :client          1.00149     -1 
                            %95 0.0194534   -1 
                            %99 0.0357093   -1 
                -1 
server         :server          0.250229    -1 
                            %95 0.0040504   -1 
                            %99 0.00743506  -1 
                -1 
-1

FQ 2
client         :client          1.00035     1           -1 1
                            %95 0.00885356  0           -1 0
                            %99 0.0162519   0           -1 0
                -1 
server         :server          1.00088     0.50082     -1 0.50082
                            %95 0.00679159  0.00617558  -1 0.00617558
                            %99 0.0124669   0.0113361   -1 0.0113361
                -1 
-1

P client 1
client          1  0 1  client          1           0           -1 
                                    %95 0           0           -1 
                                    %99 0           0           -1 
                        -1 
-1 

P server 1
server          1  0 1  server          0.50082     0           -1 
                                    %95 0.00617558  0           -1 
                                    %99 0.0113361   0           -1 
                        -1 
-1 

-1

