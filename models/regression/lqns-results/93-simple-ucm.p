# lqns 5.5
# lqns --pragma=variance=mol,threads=mak --parseable 93-simple-ucm.in
V y
C 7.14017e-06
I 9
PP 2
NP 1
#!User:  0:00:00.00
#!Sys:   0:00:00.00
#!Real:  0:00:00.00
#!Solver: 4 36 261 2355 20525 4.76623e+07 0

B 4
RefTask1       :RefTask1_RefE   0.1         
t1             :t1_E1           0.25        
t2             :t2_E1           0.4         
t3             :t3_E1           1           
-1

W 0
t1             :-1
               :
                a1_h8_SyncCall  t2_E1           2.04453e-05 -1 
  -1 
t2             :-1
               :
                a2_h6_SyncCall  t3_E1           3.10253e-06 -1 
  -1 
-1

Z 0
RefTask1       :-1
               :
                RefTask1_A1_AsyncCall t1_E1           5.60443     -1 
  -1 
-1

X 4
RefTask1       :RefTask1_RefE   10          0           0           -1 
                -1
:
                RefTask1_A1_AsyncCall 10          -1 
  -1 
t1             :t1_E1           4.00005     0           0           -1 
                -1
:
                a1_h8_SyncCall  4.00005     -1 
                t1_A1           0           -1 
                t1_A2_          0           -1 
  -1 
t2             :t2_E1           2.50001     0           0           -1 
                -1
:
                a2_h6_SyncCall  2.50001     -1 
                t2_A1_SendReply 0           -1 
  -1 
t3             :t3_E1           1           0           0           -1 
                -1
:
                a4_h5_SendReply 1           -1 
  -1 
-1

VAR 4
RefTask1       :RefTask1_RefE   0           0           0           -1 
                -1
:
                RefTask1_A1_AsyncCall 100         -1 
  -1 
t1             :t1_E1           0           0           0           -1 
                -1
:
                a1_h8_SyncCall  3.25        -1 
                t1_A1           0           -1 
                t1_A2_          0           -1 
  -1 
t2             :t2_E1           0           0           0           -1 
                -1
:
                a2_h6_SyncCall  2.125       -1 
                t2_A1_SendReply 0           -1 
  -1 
t3             :t3_E1           0           0           0           -1 
                -1
:
                a4_h5_SendReply 1           -1 
  -1 
-1

FQ 4
RefTask1       :RefTask1_RefE   0.1         0           0           0           -1 1          
                -1
:
                RefTask1_A1_AsyncCall 0.1         1           -1 
  -1 
t1             :t1_E1           0.1         0           0           0           -1 0.400005   
                -1
:
                a1_h8_SyncCall  0.1         0.400005    -1 
                t1_A1           0.1         0           -1 
                t1_A2_          0.1         0           -1 
  -1 
t2             :t2_E1           0.100001    0           0           0           -1 0.250004   
                -1
:
                a2_h6_SyncCall  0.100001    0.250004    -1 
                t2_A1_SendReply 0.100001    0           -1 
  -1 
t3             :t3_E1           0.100002    0           0           0           -1 0.100002   
                -1
:
                a4_h5_SendReply 0.100002    0.100002    -1 
  -1 
-1

P P_Infinite 1
RefTask1        1  0 1  RefTask1_RefE   1           0           0           0           -1 
-1
                       :
                        RefTask1_A1_AsyncCall 1           0           -1 
                        -1 
                                        1           
  -1 

P p1 3
t1              1  0 1  t1_E1           0.15        0           0           0           -1 
-1
                       :
                        a1_h8_SyncCall  0.15        6.09898e-06 -1 
                        t1_A1           0           0           -1 
                        t1_A2_          0           0           -1 
                        -1 
                                        0.15        
t2              1  0 1  t2_E1           0.150002    0           0           0           -1 
-1
                       :
                        a2_h6_SyncCall  0.150002    3.4277e-06  -1 
                        t2_A1_SendReply 0           0           -1 
                        -1 
                                        0.150002    
t3              1  0 1  t3_E1           0.100002    0           0           0           -1 
-1
                       :
                        a4_h5_SendReply 0.100002    1.33563e-06 -1 
                        -1 
                                        0.100002    
  -1 
                                        0.400003    
  -1 

-1

