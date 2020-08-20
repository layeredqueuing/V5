# lqsim 5.16
# lqsim --confidence=1.0,1000 --seed=1049217653 --parseable --output=07-sanity.p 07-sanity.in
# $Id: 07-sanity.p 13764 2020-08-17 19:50:05Z greg $
V y
C 0.857856
I 3
PP 2
NP 1

#!Comment: Simplest model - entry think time.
#!User:  0:10:07.000
#!Sys:   0:04:08.000
#!Real:  0:14:21.000

B 2
client         :client          0.666667    
server         :server          2           
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
client         :client          1.99747     -1 
                            %95 0.0214041   -1 
                            %99 0.0493693   -1 
                -1 
server         :server          0.998941    -1 
                            %95 0.00568243  -1 
                            %99 0.0131067   -1 
                -1 
-1

VAR 2
client         :client          5.50543     -1 
                            %95 0.21245     -1 
                            %99 0.490023    -1 
                -1 
server         :server          0.501874    -1 
                            %95 0.00643751  -1 
                            %99 0.0148483   -1 
                -1 
-1

FQ 2
client         :client          0.500634    1           -1 1
                            %95 0.00537254  0           -1 0
                            %99 0.0123919   0           -1 0
                -1 
server         :server          0.499877    0.499347    -1 0.499347
                            %95 0.00370361  0.00175798  -1 0.00175798
                            %99 0.00854249  0.00405483  -1 0.00405483
                -1 
-1

P client 1
client          1  0 1  client          0.500653    0           -1 
                                    %95 0.00175798  0           -1 
                                    %99 0.00405483  0           -1 
                        -1 
-1 

P server 1
server          1  0 1  server          0.249745    0           -1 
                                    %95 0.00189068  0           -1 
                                    %99 0.0043609   0           -1 
                        -1 
-1 

-1

