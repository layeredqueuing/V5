# lqsim 5.16
# lqsim --confidence=1.0,1000 --seed=1049217653 --parseable --output=06-sanity.p 06-sanity.in
# $Id: 06-sanity.p 13764 2020-08-17 19:50:05Z greg $
V y
C 0.846154
I 4
PP 2
NP 1

#!Comment: Simplest model - client think time.
#!User:  0:13:30.000
#!Sys:   0:05:34.000
#!Real:  0:19:49.000

B 2
client         :client          0.666667    
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
client         :client          1.49739     -1 
                            %95 0.0128117   -1 
                            %99 0.0235176   -1 
                -1 
server         :server          1.00008     -1 
                            %95 0.0083667   -1 
                            %99 0.0153582   -1 
                -1 
-1

VAR 2
client         :client          4.24644     -1 
                            %95 0.096368    -1 
                            %99 0.176897    -1 
                -1 
server         :server          1.00083     -1 
                            %95 0.0272997   -1 
                            %99 0.0501125   -1 
                -1 
-1

FQ 2
client         :client          0.500603    0.749596    -1 0.749596
                            %95 0.00423309  0.000156554 -1 0.000156554
                            %99 0.00777042  0.000287377 -1 0.000287377
                -1 
server         :server          0.499708    0.499748    -1 0.499748
                            %95 0.00316389  0.0021504   -1 0.0021504
                            %99 0.00580775  0.00394736  -1 0.00394736
                -1 
-1

P client 1
client          1  0 1  client          0.249847    0           -1 
                                    %95 0.00203452  0           -1 
                                    %99 0.00373464  0           -1 
                        -1 
-1 

P server 1
server          1  0 1  server          0.499748    0           -1 
                                    %95 0.0021504   0           -1 
                                    %99 0.00394736  0           -1 
                        -1 
-1 

-1

