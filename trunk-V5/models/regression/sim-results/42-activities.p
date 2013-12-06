# lqsim 5.2
# lqsim -C1.0,1000 -S1049217653 -p -o42-activities.p 42-activities.in
V y
C 0.874777
I 4
PP 2
NP 2
#!User:  0:00:00.00
#!Sys:   0:00:00.00
#!Real:  0:00:00.00

W 0
client         :client          server          0.199224    0           -1 
                                            %95 0.00567246  0           -1 
                                            %99 0.0104126   0           -1 
  -1 
-1

J 0
server         :fork1           fork2           0.998722    0.517615   
                                            %95 0.00757453  0.00730584 
                                            %99 0.0139041   0.0134109  
-1 
-1

X 2
client         :client          2.14648     0           -1 
                            %95 0.0240441   0           -1 
                            %99 0.0441363   0           -1 
  -1 
server         :server          0.948843    0.549119    0           0           -1 
                            %95 0.00743613  0.00544288  0           0           -1 
                            %99 0.01365     0.00999116  0           0           -1 
                -1
:
                fork1           0.698891    -1 
                            %95 0.00716566  -1 
                            %99 0.0131536   -1 
                fork2           0.798473    -1 
                            %95 0.00499201  -1 
                            %99 0.00916352  -1 
                join            0.249289    -1 
                            %95 0.00214217  -1 
                            %99 0.00393225  -1 
                server          0.249953    -1 
                            %95 0.00130519  -1 
                            %99 0.00239586  -1 
  -1 
-1

VAR 2
client         :client          6.77516     0           -1 
                            %95 0.18635     0           -1 
                            %99 0.34207     0           -1 
  -1 
server         :server          5.46127e-06 2.92589e-06 0           0           -1 
                            %95 0.0140091   0.0129172   0           0           -1 
                            %99 0.0257156   0.0237113   0           0           -1 
                -1
:
                fork1           0.428042    -1 
                            %95 0.011588    -1 
                            %99 0.0212714   -1 
                fork2           0.477312    -1 
                            %95 0.00833122  -1 
                            %99 0.0152931   -1 
                join            0.06232     -1 
                            %95 0.00152949  -1 
                            %99 0.00280759  -1 
                server          0.0628112   -1 
                            %95 0.00072313  -1 
                            %99 0.0013274   -1 
  -1 
-1

FQ 2
client         :client          0.465879    1           0           -1 1          
                            %95 0.00522266  0           0           -1 0          
                            %99 0.00958691  0           0           -1 0          
  -1 
server         :server          0.465437    0           0           0           0           -1 0.697209   
                            %95 0.00222487  0.00185783  0.00283014  0           0           -1 0.00338544 
                            %99 0.00408405  0.0034103   0.00519511  0           0           -1 0.00621444 
                -1
:
                fork1           0.465438    0.325289    -1 
                            %95 1.07007     0.00209171  -1 
                            %99 1.96426     0.00383962  -1 
                fork2           0.465437    0.37164     -1 
                            %95 1.07575     0.00155424  -1 
                            %99 1.97469     0.00285302  -1 
                join            0.465437    0.116028    -1 
                            %95 1.07074     0.00121844  -1 
                            %99 1.9655      0.00223662  -1 
                server          0.465438    0.116337    -1 
                            %95 1.07508     0.000730902 -1 
                            %99 1.97346     0.00134167  -1 
  -1 
-1

P client 1
client          1  0 1  client          0.465647    0           0           -1 
                                    %95 0.00181915  0           0           -1 
                                    %99 0.00333929  0           0           -1 
                        -1 
  -1 

P server 1
server          1  0 1  server          0.697209    0           0           0           0           -1 
                                    %95 0.00252982  0           0           0           0           -1 
                                    %99 0.00464383  0           0           0           0           -1 
-1
                       :
                        fork1           0.186072    0.299112    -1 
                                    %95 0.000899559 0.00416066  -1 
                                    %99 0.00165126  0.00763746  -1 
                        fork2           0.278772    0.19953     -1 
                                    %95 0.00169387  0.00104826  -1 
                                    %99 0.00310933  0.00192422  -1 
                        join            0.116028    0           -1 
                                    %95 0.00121844  0           -1 
                                    %99 0.00223662  0           -1 
                        server          0.116337    0           -1 
                                    %95 0.000730902 0           -1 
                                    %99 0.00134167  0           -1 
                        -1 
                                        0.697209    
                                    %95 0.00252982  
                                    %99 0.00464383  
  -1 

-1
