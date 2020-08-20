# lqsim 5.16
# lqsim --confidence=1.0,1000 --seed=1049217653 --parseable --output=23-multiserver.p 23-multiserver.in
# $Id: 23-multiserver.p 13764 2020-08-17 19:50:05Z greg $
V y
C 0.521757
I 3
PP 2
NP 1

#!Comment: Use an infinite server for server.
#!User:  0:38:30.000
#!Sys:   0:15:06.000
#!Real:  0:54:07.000

B 2
client         :client          0.5         
server         :server          1           
-1

W 1
client         :client          server          0           -1 
                                            %95 0           -1 
                                            %99 0           -1 
                -1 
-1

VARW 1
client         :client          server          0            -1 
                                            %95 0            -1 
                                            %99 0            -1 
                -1 
-1

X 2
client         :client          1.99902     -1 
                            %95 0.00316176  -1 
                            %99 0.0072927   -1 
                -1 
server         :server          0.999239    -1 
                            %95 0.00720177  -1 
                            %99 0.0166111   -1 
                -1 
-1

VAR 2
client         :client          5.9912      -1 
                            %95 0.132261    -1 
                            %99 0.305064    -1 
                -1 
server         :server          0.997185    -1 
                            %95 0.0164118   -1 
                            %99 0.0378542   -1 
                -1 
-1

FQ 2
client         :client          1.50073     3           -1 3
                            %95 0.00242309  0           -1 0
                            %99 0.00558893  0           -1 0
                -1 
server         :server          1.50038     1.49924     -1 1.49924
                            %95 0.0068575   0.00486014  -1 0.00486014
                            %99 0.015817    0.0112101   -1 0.0112101
                -1 
-1

P client 1
client          1  0 3  client          1.50076     0           -1 
                                    %95 0.00486014  0           -1 
                                    %99 0.0112101   0           -1 
                        -1 
-1 

P server 1
server          1  0 1  server          1.49924     0           -1 
                                    %95 0.00486014  0           -1 
                                    %99 0.0112101   0           -1 
                        -1 
-1 

-1

