# petrisrvn 3.10
# petrisrvn -M -p -osimple-ucm.out  simple-ucm.in
V y
C 7.7529e-06
I 81
PP 2
NP 1
#!User:  0:00:00.38
#!Sys:   0:00:00.19
#!Real:  0:00:00.59

W 2
  t1 : -1

:
     a1_h8_SyncCall t2_E1 0.000000 -1
  -1
  t2 : -1

:
     a2_h6_SyncCall t3_E1 0.000000 -1
  -1
-1


Z 1
  RefTask1 : -1

:
     RefTask1_A1_AsyncCall t1_E1 5.604337 -1
  -1
-1


X 4
t1 : t1_E1 4.000010 -1
  -1

:
     a1_h8_SyncCall 4.000010 -1
     t1_A1 0.000000 -1
     t1_A2_ 0.000000 -1
  -1
t2 : t2_E1 2.500006 -1
  -1

:
     a2_h6_SyncCall 2.500006 -1
     t2_A1_SendReply 0.000000 -1
  -1
t3 : t3_E1 1.000002 -1
  -1

:
     a4_h5_SendReply 1.000002 -1
  -1
RefTask1 : RefTask1_RefE 10.000000 -1
  -1

:
     RefTask1_A1_AsyncCall 10.000000 -1
  -1
-1

FQ 4
t1 : t1_E1 0.100001 0.400005 -1 0.400005
-1

:
    a1_h8_SyncCall  0.100001 0.400005 -1
    t1_A1           0.100001 0.000000 -1
    t1_A2_          0.100001 0.000000 -1
  -1
t2 : t2_E1 0.100001 0.250003 -1 0.250003
-1

:
    a2_h6_SyncCall  0.100001 0.250003 -1
    t2_A1_SendReply 0.100001 0.000000 -1
  -1
t3 : t3_E1 0.100001 0.100001 -1 0.100001
-1

:
    a4_h5_SendReply 0.100001 0.100001 -1
  -1
RefTask1 : RefTask1_RefE 0.100000 1.000000 -1 1.000000
-1

:
    RefTask1_A1_Asy 0.100000 1.000000 -1
  -1
-1

P P_Infinite 1
RefTask1 1 0 1 RefTask1_RefE 1.000000 0.000000 -1 -1

:
         RefTask1_A1_AsyncCall 1 0 -1
 -1
-1

P p1 3
t1 1 0 1 t1_E1 0.150002 0.000000 -1 -1

:
         a1_h8_SyncCall 0.150002 0 -1
         t1_A1 0 0 -1
         t1_A2_ 0 0 -1
 -1
t2 1 0 1 t2_E1 0.150002 0.000000 -1 -1

:
         a2_h6_SyncCall 0.150002 0 -1
         t2_A1_SendReply 0 0 -1
 -1
t3 1 0 1 t3_E1 0.100001 0.000000 -1 -1

:
         a4_h5_SendReply 0.100001 0 -1
 -1
-1
	0.400005
-1
-1

