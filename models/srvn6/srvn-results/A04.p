# srvn 3.7
# srvn -p  A04.in
V y
C 7.4623e-07
I 46
PP 4
NP 3

B 7
e1 0.189394
e2 0.156740
e3 0.400000
e4 0.909091
e5 0.384615
e6 0.333333
e7 1.250000
-1

W 7
e1 e3 0.099575 0.099575 0.099575 -1
e1 e4 0.099575 0.099575 0.099575 -1
e1 e5 9.993670 9.993670 9.993670 -1
e2 e5 11.564797 11.564797 11.564797 -1
e2 e6 3.393721 3.393721 3.393721 -1
e2 e7 3.393721 3.393721 3.393721 -1
e5 e6 1.929143 1.929143 1.929143 -1
-1


X 7
e1 0.000000 25.967623 13.435044 -1
e2 0.000000 37.153986 37.742913 -1
e3 0.500000 2.000000 0.000000 -1
e4 0.600000 0.500000 0.000000 -1
e5 1.765461 5.082566 0.000000 -1
e6 1.001778 2.002271 0.000000 -1
e7 0.100000 0.702271 0.000000 -1
-1

FQ 5
t1 1 e1 0.025379 0.000000 0.659032 0.340968 -1 1.000000 -1
t2 1 e2 0.013352 0.000000 0.496068 0.503932 -1 1.000000 -1
t3 2 e3 0.025379 0.012689 0.050758 0.000000 -1 0.063447
  e4 0.012689 0.007614 0.006345 0.000000 -1 0.013958 -1
    0.038068 0.020303 0.057103 0.000000 -1 0.077406
t4 1 e5 0.120065 0.211970 0.610239 0.000000 -1 0.822209 -1
t5 2 e6 0.310874 0.311427 0.622454 0.000000 -1 0.933881
  e7 0.013352 0.001335 0.009377 0.000000 -1 0.010712 -1
    0.324226 0.312762 0.631831 0.000000 -1 0.944593
-1

P p1 1
t1 1 0 e1 0.050758 0.000000 0.000000 0.000000 -1 -1
-1

P p2 2
t2 1 0 e2 0.004006 1.499493 1.499493 1.499493 -1 -1
t5 2 0 e6 0.932622 0.001778 0.001778 0.001778 -1
  e7 0.010681 0.000000 0.000000 0.000000 -1 -1
      0.943304
-1

P p3 1
t3 2 0 e3 0.063447 0.000000 0.000000 0.000000 -1
  e4 0.013958 0.000000 0.000000 0.000000 -1 -1
      0.077406
-1

P p4 1
t4 1 0 e5 0.048026 0.000000 0.000000 0.000000 -1 -1
-1
-1
