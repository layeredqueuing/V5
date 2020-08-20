# lqsim 5.16
# lqsim --confidence=1.0,1000 --seed=1049217653 --parseable --output=20-multiserver.p 20-multiserver.in
# $Id: 20-multiserver.p 13764 2020-08-17 19:50:05Z greg $
V y
C 0.293529
I 3
PP 2
NP 1

#!Comment: Simplest multiserver model.
#!User:  0:45:07.000
#!Sys:   0:17:56.000
#!Real:  1:04:24.000

B 2
client         :client          0.5         
server         :server          1           
-1

W 1
client         :client          server          0.353427    -1 
                                            %95 0.000520518 -1 
                                            %99 0.00120059  -1 
                -1 
-1

VARW 1
client         :client          server          0.317502     -1 
                                            %95 0.00421024   -1 
                                            %99 0.00971105   -1 
                -1 
-1

X 2
client         :client          2.35433     -1 
                            %95 0.00805167  -1 
                            %99 0.0185714   -1 
                -1 
server         :server          1.00016     -1 
                            %95 0.00235325  -1 
                            %99 0.00542783  -1 
                -1 
-1

VAR 2
client         :client          8.80739     -1 
                            %95 0.0838735   -1 
                            %99 0.193457    -1 
                -1 
server         :server          1.004       -1 
                            %95 0.00970075  -1 
                            %99 0.0223751   -1 
                -1 
-1

FQ 2
client         :client          1.699       4           -1 4
                            %95 0.00575688  0           -1 0
                            %99 0.0132784   0           -1 0
                -1 
server         :server          1.70001     1.70028     -1 1.70028
                            %95 0.00414067  0.000503745 -1 0.000503745
                            %99 0.00955059  0.0011619   -1 0.0011619
                -1 
-1

P client 1
client          1  0 4  client          1.69889     0           -1 
                                    %95 0.00161935  0           -1 
                                    %99 0.00373508  0           -1 
                        -1 
-1 

P server 1
server          1  0 2  server          1.70028     0           -1 
                                    %95 0.000503745 0           -1 
                                    %99 0.0011619   0           -1 
                        -1 
-1 

-1

