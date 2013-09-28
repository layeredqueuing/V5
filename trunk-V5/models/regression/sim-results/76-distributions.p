# lqsim 5.2
# lqsim -C1.0,1000 -S1049217653 -p -o76-distributions.p 76-distributions.in
V y
C 0.975066
I 7
PP 4
NP 1
#!User:  0:00:00.00
#!Sys:   0:00:00.00
#!Real:  0:00:00.00

W 0
constant_client:constant_client constant_server 0           -1 
                                            %95 0           -1 
                                            %99 0           -1 
  -1 
exponential_client:exponential_client exponential_server 0           -1 
                                            %95 0           -1 
                                            %99 0           -1 
  -1 
gamma_client   :gamma_client    gamma_server    0           -1 
                                            %95 0           -1 
                                            %99 0           -1 
  -1 
hyperexponential_client:hyperexponential_client hyperexponential_server 0           -1 
                                            %95 0           -1 
                                            %99 0           -1 
  -1 
-1

X 8
constant_client:constant_client 2.99863     -1 
                            %95 0.0242473   -1 
                            %99 0.0367326   -1 
  -1 
constant_server:constant_server 1           -1 
                            %95 0           -1 
                            %99 0           -1 
  -1 
exponential_client:exponential_client 2.99099     -1 
                            %95 0.0234749   -1 
                            %99 0.0355625   -1 
  -1 
exponential_server:exponential_server 1.00005     -1 
                            %95 0.00520467  -1 
                            %99 0.00788463  -1 
  -1 
gamma_client   :gamma_client    3.00105     -1 
                            %95 0.0196893   -1 
                            %99 0.0298277   -1 
  -1 
gamma_server   :gamma_server    0.999645    -1 
                            %95 0.00261973  -1 
                            %99 0.00396867  -1 
  -1 
hyperexponential_client:hyperexponential_client 2.99653     -1 
                            %95 0.0464298   -1 
                            %99 0.0703372   -1 
  -1 
hyperexponential_server:hyperexponential_server 0.998916    -1 
                            %95 0.0148656   -1 
                            %99 0.0225201   -1 
  -1 
-1

VAR 8
constant_client:constant_client 10.7073     -1 
                            %95 0.220339    -1 
                            %99 0.333795    -1 
  -1 
constant_server:constant_server 0           -1 
                            %95 0           -1 
                            %99 0           -1 
  -1 
exponential_client:exponential_client 12.9364     -1 
                            %95 0.219398    -1 
                            %99 0.33237     -1 
  -1 
exponential_server:exponential_server 0.998207    -1 
                            %95 0.0142901   -1 
                            %99 0.0216483   -1 
  -1 
gamma_client   :gamma_client    11.2694     -1 
                            %95 0.124979    -1 
                            %99 0.189333    -1 
  -1 
gamma_server   :gamma_server    0.249739    -1 
                            %95 0.00276864  -1 
                            %99 0.00419426  -1 
  -1 
hyperexponential_client:hyperexponential_client 31.6616     -1 
                            %95 1.38285     -1 
                            %99 2.09491     -1 
  -1 
hyperexponential_server:hyperexponential_server 8.98629     -1 
                            %95 0.449629    -1 
                            %99 0.681151    -1 
  -1 
-1

FQ 8
constant_client:constant_client 0.333489    1           -1 1          
                            %95 0.00270059  0           -1 0          
                            %99 0.00409116  0           -1 0          
  -1 
constant_server:constant_server 0.666628    0.666628    -1 0.666628   
                            %95 0.000675385 0.000675509 -1 0.000675509
                            %99 0.00102315  0.00102334  -1 0.00102334 
  -1 
exponential_client:exponential_client 0.33434     1           -1 1          
                            %95 0.00261888  0           -1 0          
                            %99 0.00396739  0           -1 0          
  -1 
exponential_server:exponential_server 0.666645    0.666679    -1 0.666679   
                            %95 0.00215186  0.00161822  -1 0.00161822 
                            %99 0.00325989  0.00245147  -1 0.00245147 
  -1 
gamma_client   :gamma_client    0.333218    1           -1 1          
                            %95 0.00218353  0           -1 0          
                            %99 0.00330786  0           -1 0          
  -1 
gamma_server   :gamma_server    0.666851    0.666614    -1 0.666614   
                            %95 0.00164145  0.000523072 -1 0.000523072
                            %99 0.00248666  0.000792411 -1 0.000792411
  -1 
hyperexponential_client:hyperexponential_client 0.333732    1           -1 1          
                            %95 0.00517041  0           -1 0          
                            %99 0.00783274  0           -1 0          
  -1 
hyperexponential_server:hyperexponential_server 0.666489    0.665754    -1 0.665754   
                            %95 0.0064008   0.00456517  -1 0.00456517 
                            %99 0.00969668  0.00691586  -1 0.00691586 
  -1 
-1

P constant 2
constant_client 1  0 1  constant_client 0.333372    0           -1 
                                    %95 0.000675509 0           -1 
                                    %99 0.00102334  0           -1 
                        -1 
constant_server 1  0 1  constant_server 0.666628    0           -1 
                                    %95 0.000675509 0           -1 
                                    %99 0.00102334  0           -1 
                        -1 
  -1 
                                        1           
                                    %95 0.000955314 
                                    %99 0.00144722  
  -1 

P exponential 2
exponential_client 1  0 1  exponential_client 0.333321    0           -1 
                                    %95 0.00161822  0           -1 
                                    %99 0.00245147  0           -1 
                        -1 
exponential_server 1  0 1  exponential_server 0.666679    0           -1 
                                    %95 0.00161822  0           -1 
                                    %99 0.00245147  0           -1 
                        -1 
  -1 
                                        1           
                                    %95 0.00228851  
                                    %99 0.00346691  
  -1 

P gamma 2
gamma_client    1  0 1  gamma_client    0.333386    0           -1 
                                    %95 0.000523072 0           -1 
                                    %99 0.000792411 0           -1 
                        -1 
gamma_server    1  0 1  gamma_server    0.666614    0           -1 
                                    %95 0.000523072 0           -1 
                                    %99 0.000792411 0           -1 
                        -1 
  -1 
                                        1           
                                    %95 0.000739736 
                                    %99 0.00112064  
  -1 

P hyperexponential 2
hyperexponential_client 1  0 1  hyperexponential_client 0.334246    0           -1 
                                    %95 0.00456517  0           -1 
                                    %99 0.00691586  0           -1 
                        -1 
hyperexponential_server 1  0 1  hyperexponential_server 0.665754    0           -1 
                                    %95 0.00456517  0           -1 
                                    %99 0.00691586  0           -1 
                        -1 
  -1 
                                        1           
                                    %95 0.00645613  
                                    %99 0.0097805   
  -1 

-1

