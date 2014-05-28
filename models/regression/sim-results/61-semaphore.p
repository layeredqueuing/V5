# lqsim 5.2
# lqsim -C1.0,1000 -S1049217653 -p -o61-semaphore.p 61-semaphore.in
V y
C 0.840866
I 3
PP 2
NP 2
#!User:  0:00:00.00
#!Sys:   0:00:00.00
#!Real:  0:00:00.00

W 0
app_wait       :app_wait        app_signal      0           0           -1 
                                            %95 0           0           -1 
                                            %99 0           0           -1 
                app_wait        wait            0           0           -1 
                                            %95 0           0           -1 
                                            %99 0           0           -1 
  -1 
customer       :customer        app_wait        6.39541     0           -1 
                                            %95 0.0284121   0           -1 
                                            %99 0.0655334   0           -1 
  -1 
-1

Z 0
app_signal     :app_signal      signal          0.799271    0           -1 
                                            %95 0.00425608  0           -1 
                                            %99 0.00981678  0           -1 
  -1 
-1

DP 0
app_signal     :app_signal      signal          0           0           -1 
                                            %95 0            0            -1 
                                            %99 0            0            -1 
  -1 
-1

X 5
app_signal     :app_signal      0.998327    0           -1 
                            %95 0.00884592  0           -1 
                            %99 0.0204034   0           -1 
  -1 
app_wait       :app_wait        1.60103     1.39828     -1 
                            %95 0.00835788  0.0124645   -1 
                            %99 0.0192777   0.0287497   -1 
  -1 
customer       :customer        8.99005     0           -1 
                            %95 0.125367    0           -1 
                            %99 0.289164    0           -1 
  -1 
semaphore      :signal          0.799271    0           -1 
                            %95 0.00425608  0           -1 
                            %99 0.00981678  0           -1 
                wait            0.699295    0           -1 
                            %95 0.00444991  0           -1 
                            %99 0.0102638   0           -1 
  -1 
-1

VAR 5
app_signal     :app_signal      0.497941    0           -1 
                            %95 0.010559    0           -1 
                            %99 0.0243546   0           -1 
  -1 
app_wait       :app_wait        0.894261    0.593347    -1 
                            %95 0.022102    0.0146275   -1 
                            %99 0.050979    0.0337388   -1 
  -1 
customer       :customer        154.83      0           -1 
                            %95 1.76551     0           -1 
                            %99 4.0722      0           -1 
  -1 
semaphore      :signal          0.338946    0           -1 
                            %95 0.00546152  0           -1 
                            %99 0.0125972   0           -1 
                wait            0.490154    0           -1 
                            %95 0.00806382  0           -1 
                            %99 0.0185995   0           -1 
  -1 
-1

H 0
semaphore      :wait            signal          2.49943     1.28593     0.833326   
                                            %95 0.00671505  0.0419184   0.00116394 
                                            %99 0.0154885   0.0966861   0.00268465 
-1

FQ 4
app_signal     :app_signal      0.333407    0.332848    0           -1 0.332848   
                            %95 0.00113812  0.00212428  0           -1 0.00212428 
                            %99 0.00262512  0.00489971  0           -1 0.00489971 
  -1 
app_wait       :app_wait        0.333407    0.533794    0.466196    -1 0.99999    
                            %95 0.00113812  0.00314137  0.00314593  -1 0.00444579 
                            %99 0.00262512  0.00724566  0.00725618  -1 0.0102544  
  -1 
customer       :customer        0.333705    3           0           -1 3          
                            %95 0.00463281  0           0           -1 0          
                            %99 0.0106857   0           0           -1 0          
  -1 
semaphore      :signal          0.333407    0.266482    0           -1 0.266482   
                            %95 0.00113812  0.00124555  0           -1 0.00124555 
                            %99 0.00262512  0.0028729   0           -1 0.0028729  
                wait            0.333407    0.233149    0           -1 0.233149   
                            %95 0.00113812  0.000844644 0           -1 0.000844644
                            %99 0.00262512  0.0019482   0           -1 0.0019482  
  -1 
                                0.333407    0.499632    0           -1 0.499632   
                            %95 0.00160955  0.00150493  0           -1 0.000667545
                            %99 0.00371248  0.00347117  0           -1 0.00153971 
-1

P customer 1
customer        1  0 3  customer        0.333931    0           0           -1 
                                    %95 0.00279652  0           0           -1 
                                    %99 0.00645026  0           0           -1 
                        -1 
  -1 

P server 3
app_wait        1  0 1  app_wait        0.333904    0           0.3002      -1 
                                    %95 0.00297161  0           0.00239018  -1 
                                    %99 0.00685412  0           0.00551303  -1 
                        -1 
app_signal      1  0 1  app_signal      0.332848    0           0           -1 
                                    %95 0.00212428  0           0           -1 
                                    %99 0.00489971  0           0           -1 
                        -1 
semaphore       2  0 1  signal          0.100089    0.499071    0           -1 
                                    %95 0.000585213 0.00212863  0           -1 
                                    %99 0.00134981  0.00490975  0           -1 
                        wait            0.233149    0           0           -1 
                                    %95 0.000844644 0           0           -1 
                                    %99 0.0019482   0           0           -1 
                        -1 
                                        0.333238    
                                    %95 0.00102757  
                                    %99 0.00237012  
  -1 
                                        0.99999     
                                    %95 0.00379459  
                                    %99 0.00875235  
  -1 

-1
