# petrisrvn 3.10
# petrisrvn -M -p -osync2.out  sync2.in
V y
C 2.6396e-06
I 13
PP 3
NP 2
#!User:  0:00:00.30
#!Sys:   0:00:00.15
#!Real:  0:00:00.48

W 2
t1 : e1 e3 0.401489 0.000000 -1
  -1
t2 : e2 e4 0.006433 0.000000 -1
  -1
-1


J 1
t3 : a1 a2 1.009091 0.000000
  -1
-1


X 4
t1 : e1 1.501490 0.000000 -1
  -1
t2 : e2 1.501488 0.000000 -1
  -1
t3 : e3 0.100000 0.814035 -1
     e4 0.495055 0.000000 -1
  -1

:
     a1 0.100000 -1
     a2 0.100000 -1
     a3 0.100000 -1
  -1
-1

FQ 3
t1 : e1 0.666005 1.000000 0.000000 -1 1.000000
-1
t2 : e2 0.666006 1.000000 0.000000 -1 1.000000
-1
t3 : e3 0.666006 0.057148 0.465202 -1 0.522349
  e4 0.666006 0.282912 0.000000 -1 0.282912
-1
:
    a1              0.666006 0.057148 -1
    a2              0.666006 0.057148 -1
    a3              0.666006 0.057148 -1
  -1
    0.666006 0.340060 0.465202 -1 0.805262
-1

P p1 1
t1 1 0 1 e1 0.666005 0.000000 0.000000 -1 -1
-1

P p2 1
t2 1 0 1 e2 0.666006 0.000000 0.000000 -1 -1
-1

P p3 1
t3 2 0 1 e3 0.133201 0.000000 0.000000 -1
         e4 0.133201 0.000000 0.000000 -1 -1

:
         a1 0.0666006 0 -1
         a2 0.0666006 0 -1
         a3 0.0666006 0 -1
 -1
      0.266402
-1
-1
