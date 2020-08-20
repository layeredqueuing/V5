# lqsim 5.16
# lqsim --confidence=1.0,1000 --seed=1049217653 --parseable --output=05-sanity.p 05-sanity.in
# $Id: 05-sanity.p 13764 2020-08-17 19:50:05Z greg $
V y
C 0.817634
I 4
PP 1
NP 1

#!Comment: Simplest model - open arrival.
#!User:  0:19:33.000
#!Sys:   0:07:19.000
#!Real:  0:28:19.000

B 1
server         :server          1           
-1

X 1
server         :server          1.00016     -1 
                            %95 0.00817763  -1 
                            %99 0.0150112   -1 
                -1 
-1

VAR 1
server         :server          1.00059     -1 
                            %95 0.0192495   -1 
                            %99 0.0353351   -1 
                -1 
-1

FQ 1
server         :server          0.500217    0.500297    -1 0.500297
                            %95 0.00247848  0.00569458  -1 0.00569458
                            %99 0.00454959  0.0104532   -1 0.0104532
                -1 
-1

R 0
server         :server          0.500217    2.00267    
                            %95             0.0320227  
                            %99             0.0587821  
-1

P server 1
server          1  0 1  server          0.500297    0           -1 
                                    %95 0.00569458  0           -1 
                                    %99 0.0104532   0           -1 
                        -1 
-1 

-1

