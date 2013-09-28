# petrisrvn 3.9
# petrisrvn -p -obug_93-petri.p -zin-service  bug_93.in
V y
C 4.8407e-06
I 28218
PP 2
NP 2
#!User:  0:00:02.96
#!Sys:   0:00:00.33
#!Real:  0:00:06.49

W 1
SCR_T : SCR_E AccCtrl_E 275.702564 0.000000 -1
  -1
-1


X 2
SCR_T : SCR_E 283.738860 0.000000 -1
  -1
AccCtrl_T : AccCtrl_E 7.033741 503.236289 -1
  -1
-1

FQ 2
SCR_T 1 SCR_E 0.000485 0.137613 0.000000 -1 0.137613 -1
AccCtrl_T 1 AccCtrl_E 0.000485 0.003411 0.244070 -1 0.247481 -1
-1

P SCR_CPU 1
SCR_T 1 0 5 SCR_E  0.000486 0.000000 0.000000 -1 -1
-1

P ApplicCPU 1
AccCtrl_T 1 0 1 AccCtrl_E  0.123148 0.000000 0.000000 -1 -1
-1
-1

OT
SCR_E AccCtrl_E SCR_E AccCtrl_E 2  0.041237 0.000000  -1
-1

