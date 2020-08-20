# lqsim 5.16
# lqsim --confidence=1.0,1000 --seed=1049217653 --parseable --output=75-distributions.p 75-distributions.in
# $Id: 75-distributions.p 13764 2020-08-17 19:50:05Z greg $
V y
C 0.443845
I 3
PP 5
NP 1

#!Comment: Distribution tests.
#!User:  1:15:01.000
#!Sys:   0:28:47.000
#!Real:  1:44:53.000

B 5
constant       :constant        0.333333    
exponential    :exponential     0.333333    
gamma          :gamma           0.333333    
hyperexponential:hyperexponential 0.333333    
pareto         :pareto          0.333333    
-1

X 5
constant       :constant        3.00196     -1 
                            %95 0.0043104   -1 
                            %99 0.00994206  -1 
                -1 
exponential    :exponential     2.99801     -1 
                            %95 0.00466799  -1 
                            %99 0.0107668   -1 
                -1 
gamma          :gamma           2.99956     -1 
                            %95 0.00858519  -1 
                            %99 0.019802    -1 
                -1 
hyperexponential:hyperexponential 2.99119     -1 
                            %95 0.026575    -1 
                            %99 0.0612961   -1 
                -1 
pareto         :pareto          2.99742     -1 
                            %95 0.00786689  -1 
                            %99 0.0181452   -1 
                -1 
-1

VAR 5
constant       :constant        9.01315     -1 
                            %95 0.0207077   -1 
                            %99 0.0477628   -1 
                -1 
exponential    :exponential     8.99013     -1 
                            %95 0.0559689   -1 
                            %99 0.129094    -1 
                -1 
gamma          :gamma           1.9986      -1 
                            %95 0.0119407   -1 
                            %99 0.0275417   -1 
                -1 
hyperexponential:hyperexponential 80.2653     -1 
                            %95 0.685282    -1 
                            %99 1.58062     -1 
                -1 
pareto         :pareto          7.43193     -1 
                            %95 3.1165      -1 
                            %99 7.1883      -1 
                -1 
-1

FQ 5
constant       :constant        0.333116    1           -1 1
                            %95 0.000478337 0           -1 0
                            %99 0.0011033   0           -1 0
                -1 
exponential    :exponential     0.333555    1           -1 1
                            %95 0.000519946 0           -1 0
                            %99 0.00119927  0           -1 0
                -1 
gamma          :gamma           0.333382    1           -1 1
                            %95 0.000953626 0           -1 0
                            %99 0.00219957  0           -1 0
                -1 
hyperexponential:hyperexponential 0.334316    1           -1 1
                            %95 0.00297235  0           -1 0
                            %99 0.00685583  0           -1 0
                -1 
pareto         :pareto          0.33362     1           -1 1
                            %95 0.000876067 0           -1 0
                            %99 0.00202068  0           -1 0
                -1 
-1

P constant 1
constant        1  0 1  constant        1           0           -1 
                                    %95 0           0           -1 
                                    %99 0           0           -1 
                        -1 
-1 

P exponential 1
exponential     1  0 1  exponential     1           0           -1 
                                    %95 0           0           -1 
                                    %99 0           0           -1 
                        -1 
-1 

P gamma 1
gamma           1  0 1  gamma           1           0           -1 
                                    %95 0           0           -1 
                                    %99 0           0           -1 
                        -1 
-1 

P hyperexponential 1
hyperexponential 1  0 1  hyperexponential 1           0           -1 
                                    %95 0           0           -1 
                                    %99 0           0           -1 
                        -1 
-1 

P pareto 1
pareto          1  0 1  pareto          1           0           -1 
                                    %95 0           0           -1 
                                    %99 0           0           -1 
                        -1 
-1 

-1

