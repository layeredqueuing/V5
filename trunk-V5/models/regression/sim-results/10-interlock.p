# lqsim 5.2
# lqsim -C1.0,1000 -S1049217653 -p -o10-interlock.p 10-interlock.in
V y
C 0.922394
I 4
PP 3
NP 1
#!User:  0:00:00.00
#!Sys:   0:00:00.00
#!Real:  0:00:00.00

W 0
t0             :e0              e1              0           -1 
                                            %95 0           -1 
                                            %99 0           -1 
                e0              e2              0           -1 
                                            %95 0           -1 
                                            %99 0           -1 
  -1 
t1             :e1              e2              0           -1 
                                            %95 0           -1 
                                            %99 0           -1 
  -1 
-1

X 3
t0             :e0              3.98574     -1 
                            %95 0.0208871   -1 
                            %99 0.0383412   -1 
  -1 
t1             :e1              2.0014      -1 
                            %95 0.0252917   -1 
                            %99 0.0464264   -1 
  -1 
t2             :e2              0.997889    -1 
                            %95 0.00823403  -1 
                            %99 0.0151147   -1 
  -1 
-1

VAR 3
t0             :e0              27.8827     -1 
                            %95 0.541226    -1 
                            %99 0.993496    -1 
  -1 
t1             :e1              5.99312     -1 
                            %95 0.0914093   -1 
                            %99 0.167794    -1 
  -1 
t2             :e2              0.995561    -1 
                            %95 0.0176878   -1 
                            %99 0.0324684   -1 
  -1 
-1

FQ 3
t0             :e0              0.250895    1           -1 1          
                            %95 0.00130552  0           -1 0          
                            %99 0.00239646  0           -1 0          
  -1 
t1             :e1              0.250099    0.500545    -1 0.500545   
                            %95 0.00165223  0.00326355  -1 0.00326355 
                            %99 0.00303289  0.00599069  -1 0.00599069 
  -1 
t2             :e2              0.500546    0.499488    -1 0.499488   
                            %95 0.00289725  0.00169744  -1 0.00169744 
                            %99 0.00531831  0.00311589  -1 0.00311589 
  -1 
-1

P p0 1
t0              1  0 1  e0              0.250332    0           -1 
                                    %95 0.000665443 0           -1 
                                    %99 0.00122151  0           -1 
                        -1 
  -1 

P p1 1
t1              1  0 1  e1              0.250181    0           -1 
                                    %95 0.00145337  0           -1 
                                    %99 0.00266786  0           -1 
                        -1 
  -1 

P p2 1
t2              1  0 1  e2              0.499488    0           -1 
                                    %95 0.00169744  0           -1 
                                    %99 0.00311589  0           -1 
                        -1 
  -1 

-1
