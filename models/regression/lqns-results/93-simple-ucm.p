# lqns 5.15
# lqns --pragma=variance=mol,threads=mak --parseable 93-simple-ucm.in
# $Id: 93-simple-ucm.p 13685 2020-07-14 02:53:54Z greg $
V y
C 0
I 2
PP 2
NP 1

#!User:  0:00:00.000
#!Sys:   0:00:00.000
#!Real:  0:00:00.000
#!Solver: 4 8 58 542 4784 1.29922e+07 0

B 4
RefTask1       :RefTask1_RefE   0.1         
t1             :t1_E1           0.25        
t2             :t2_E1           0.4         
t3             :t3_E1           1           
-1

W 2
t1             :-1
               :
                a1_h8_SyncCall  t2_E1           0           -1 
                -1 
t2             :-1
               :
                a2_h6_SyncCall  t3_E1           0           -1 
                -1 
-1

Z 1
RefTask1       :-1
               :
                RefTask1_A1_AsyncCall t1_E1           5.60417     -1 
                -1 
-1

X 4
RefTask1       :RefTask1_RefE   10          -1 
                -1
:
                RefTask1_A1_AsyncCall 10          -1 
                -1 
t1             :t1_E1           4           -1 
                -1
:
                a1_h8_SyncCall  4           -1 
                t1_A1           0           -1 
                t1_A2_          0           -1 
                -1 
t2             :t2_E1           2.5         -1 
                -1
:
                a2_h6_SyncCall  2.5         -1 
                t2_A1_SendReply 0           -1 
                -1 
t3             :t3_E1           1           -1 
                -1
:
                a4_h5_SendReply 1           -1 
                -1 
-1

VAR 4
RefTask1       :RefTask1_RefE   100         -1 
                -1
:
                RefTask1_A1_AsyncCall 100         -1 
                -1 
t1             :t1_E1           3.25        -1 
                -1
:
                a1_h8_SyncCall  3.25        -1 
                t1_A1           0           -1 
                t1_A2_          0           -1 
                -1 
t2             :t2_E1           2.125       -1 
                -1
:
                a2_h6_SyncCall  2.125       -1 
                t2_A1_SendReply 0           -1 
                -1 
t3             :t3_E1           1           -1 
                -1
:
                a4_h5_SendReply 1           -1 
                -1 
-1

FQ 4
RefTask1       :RefTask1_RefE   0.1         1           -1 1
                -1
:
                RefTask1_A1_AsyncCall 0.1         1           -1 
                -1 
t1             :t1_E1           0.1         0.4         -1 0.4
                -1
:
                a1_h8_SyncCall  0.1         0.4         -1 
                t1_A1           0.1         0           -1 
                t1_A2_          0.1         0           -1 
                -1 
t2             :t2_E1           0.1         0.25        -1 0.25
                -1
:
                a2_h6_SyncCall  0.1         0.25        -1 
                t2_A1_SendReply 0.1         0           -1 
                -1 
t3             :t3_E1           0.1         0.1         -1 0.1
                -1
:
                a4_h5_SendReply 0.1         0.1         -1 
                -1 
-1

P P_Infinite 1
RefTask1        1  0 1  RefTask1_RefE   1           0           -1 
-1
                       :
                        RefTask1_A1_AsyncCall 1           0           -1 
                        -1 
                                        1           
-1 

P p1 3
t1              1  0 1  t1_E1           0.15        0           -1 
-1
                       :
                        a1_h8_SyncCall  0.15        0           -1 
                        t1_A1           0           0           -1 
                        t1_A2_          0           0           -1 
                        -1 
                                        0.15        
t2              1  0 1  t2_E1           0.15        0           -1 
-1
                       :
                        a2_h6_SyncCall  0.15        0           -1 
                        t2_A1_SendReply 0           0           -1 
                        -1 
                                        0.15        
t3              1  0 1  t3_E1           0.1         0           -1 
-1
                       :
                        a4_h5_SendReply 0.1         0           -1 
                        -1 
                                        0.1         
                -1 
                                        0.4
-1 

-1

