# petrisrvn 3.10
# petrisrvn -p  A09.in
V y
C 9.5567e-06
I 1740
PP 4
NP 3
#!User:  0:00:00.78
#!Sys:   0:00:00.24
#!Real:  0:00:01.06

W 7
t1 : e1 e4 0.000000 0.607177 0.000000 -1
     e1 e3 0.000000 0.607177 0.000000 -1
     e1 e5 0.000000 2.606391 2.681250 -1
  -1
t2 : e2 e5 0.000000 2.200337 2.266370 -1
     e2 e7 0.000000 0.000000 0.699332 -1
     e2 e6 0.000000 0.542596 0.699332 -1
  -1
t4 : e5 e6 0.459527 0.593724 0.000000 -1
  -1
-1


X 7
t1 : e1 0.000000 10.651926 4.287940 -1
  -1
t2 : e2 0.000000 7.679772 8.392748 -1
  -1
t3 : e4 1.000007 1.000007 0.000000 -1
     e3 1.000007 2.000014 0.000000 -1
  -1
t4 : e5 0.682495 1.635227 0.000000 -1
  -1
t5 : e7 0.299998 0.717853 0.000000 -1
     e6 0.306789 0.504659 0.000000 -1
  -1
-1

FQ 5
t1 : e1 0.066935 0.000000 0.712987 0.287013 -1 1.000000
-1
t2 : e2 0.062218 0.000000 0.477820 0.522180 -1 1.000000
-1
t3 : e4 0.033467 0.033467 0.033467 0.000000 -1 0.066934
  e3 0.066934 0.066934 0.133869 0.000000 -1 0.200803
-1
    0.100401 0.100402 0.167336 0.000000 -1 0.267738
t4 : e5 0.400354 0.273239 0.654670 0.000000 -1 0.927909
-1
t5 : e7 0.062219 0.018666 0.044664 0.000000 -1 0.063330
  e6 1.098535 0.337018 0.554386 0.000000 -1 0.891404
-1
    1.160754 0.355684 0.599050 0.000000 -1 0.954734
-1

P p1 1
t1 1 0 1 e1 0.133869 0.000000 0.000000 0.000000 -1 -1
-1

P p2 2
t2 1 0 1 e2 0.018665 0.000000 1.150825 1.543439 -1 -1
t5 2 0 1 e7 0.062219 0.000000 0.017857 0.000000 -1
         e6 0.878828 0.006789 0.004660 0.000000 -1 -1
      0.941046
-1
	0.959711
-1

P p3 1
t3 2 0 1 e4 0.066934 0.000000 0.000000 0.000000 -1
         e3 0.200803 0.000000 0.000000 0.000000 -1 -1
      0.267738
-1

P p4 1
t4 1 0 1 e5 0.160142 0.000000 0.000000 0.000000 -1 -1
-1
-1
