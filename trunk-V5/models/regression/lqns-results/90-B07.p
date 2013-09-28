# lqns 5.4
# lqns --pragma=variance=mol,threads=mak --parseable 90-B07.in
V y
C 6.46667e-07
I 26
PP 2
NP 3
#!User:  0:00:03.00
#!Sys:   0:00:00.00
#!Real:  0:00:04.00
#!Solver: 4 104 1064 15470 276255 5.15962e+09 0

B 7
t1             :e1              0.211416    
t2             :e2              0.15674     
t3             :e3              0.47619     
                e4              1.25        
t4             :e5              0.384615    
t5             :e6              0.333333    
                e7              1.25        
-1

W 0
t1             :e1              e3              0           0.415666    0           -1 
                e1              e4              0           0.415666    0           -1 
                e1              e5              0           7.50887     7.87362     -1 
  -1 
t2             :e2              e5              0           10.7739     10.1162     -1 
                e2              e6              0           2.49953     2.57331     -1 
                e2              e7              0           0           2.57331     -1 
  -1 
t4             :e5              e6              1.19099     1.63484     0           -1 
  -1 
-1

X 7
t1             :e1              0           21.0576     11.118      -1 
  -1 
t2             :e2              0           34.2228     32.9686     -1 
  -1 
t3             :e3              0.122649    2.02265     0           -1 
                e4              0.322649    0.522649    0           -1 
  -1 
t4             :e5              1.5716      4.89628     0           -1 
  -1 
t5             :e6              1.00007     2.00007     0           -1 
                e7              0.100075    0.700075    0           -1 
  -1 
-1

VAR 7
t1             :e1              0           1219.8      618.022     -1 
  -1 
t2             :e2              0           2652.55     1873.37     -1 
  -1 
t3             :e3              0.010513    4.00051     0           -1 
                e4              0.090513    0.250513    0           -1 
  -1 
t4             :e5              8.80055     51.9269     0           -1 
  -1 
t5             :e6              1           4           0           -1 
                e7              0.01        0.49        0           -1 
  -1 
-1

FQ 5
t1             :e1              0.0310794   0           0.654458    0.345542    -1 1          
  -1 
t2             :e2              0.01        0           0.342228    0.329686    -1 0.671914   
  -1 
t3             :e3              0.0310794   0.00381185  0.0628627   0           -1 0.0666745  
                e4              0.0155397   0.00501386  0.0081218   0           -1 0.0131357  
  -1 
                                0.0466191   0.00882571  0.0709845   0           -1 0.0798102  
t4             :e5              0.127347    0.200138    0.623524    0           -1 0.823662   
  -1 
t5             :e6              0.315204    0.315228    0.630432    0           -1 0.94566    
                e7              0.01        0.00100075  0.00700077  0           -1 0.00800153 
  -1 
                                0.325204    0.316229    0.637433    0           -1 0.953662   
-1

R 0
t2             :e2              0.01        204.971    
-1

P p1 3
t1              1  0 1  e1              0.0621589   0           0.489141    0.228266    -1 
                        -1 
t3              2  0 1  e3              0.0652667   0.0226489   0.0226489   0           -1 
                        e4              0.0124318   0.0226489   0.0226489   0           -1 
                        -1 
                                        0.0776985   
t4              1  0 1  e5              0.0509386   0.176067    0.31692     0           -1 
                        -1 
  -1 
                                        0.190796    
  -1 

P p2 2
t2              1  0 1  e2              0.003       0           5.93221     8.3051      -1 
                        -1 
t5              2  0 1  e6              0.945613    7.4958e-05  7.4958e-05  0           -1 
                        e7              0.00800003  7.4958e-05  7.4958e-05  0           -1 
                        -1 
                                        0.953613    
  -1 
                                        0.956613    
  -1 

-1
