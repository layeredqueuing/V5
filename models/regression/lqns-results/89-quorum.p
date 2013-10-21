# lqns 3.10
# lqns -p quorum2.in
V y
C 1.18189e-07
I 9
PP 4
NP 1
#!User:  0:00:01.04
#!Sys:   0:00:00.00
#!Real:  0:00:01.06
#!Solver:   52   0   0        537     10.327     11.734 2.3849e+05     4586.4      14060  0:00:01.04  0:00:00.00  0:00:01.06 
B 4
t0              : e0              0.458363    
t1              : e1              0.466924    
d1              : d1              25          
d2              : d2              25          
 -1

W 3
t0              : e0              e1              3.22754e-08  -1 
 -1
t1              :  -1
  :
                b1              d1              0            -1 
                b2              d2              0            -1 
 -1
 -1

J 2
t1              : b1              b2              0.136124    0.00326594  
 -1
 -1


X 4
t0              : e0              2.18291      -1 
 -1
t1              : e1              2.14291      -1 
 -1
  :
                a1              1            -1 
                b1              0.182597     -1 
                b2              0.182597     -1 
                c1              1            -1 
 -1
d1              : d1              0.04         -1 
 -1
d2              : d2              0.04         -1 
 -1
 -1


VAR 4
t0              : e0              11.2948      -1 
 -1
t1              : e1              1.93757      -1 
 -1
  :
                a1              1            -1 
                b1              0.00750727   -1 
                b2              0.00750727   -1 
                c1              1            -1 
 -1
d1              : d1              0.0016       -1 
 -1
d2              : d2              0.0016       -1 
 -1
 -1

FQ 4
t0              : e0              0.458103    1            -1 1           
 -1
t1              : e1              0.458103    0.981676     -1 0.981676    
 -1
  :
                a1              0.458103    0.458103     -1 
                b1              0.458103    0.0836485    -1 
                b2              0.458103    0.0836485    -1 
                c1              0.458103    0.458103     -1 
 -1
d1              : d1              0.458103    0.0183241    -1 0.0183241   
 -1
d2              : d2              0.458103    0.0183241    -1 0.0183241   
 -1
 -1

P p0 1
t0              1 0   1   e0              0.0183241   0            -1 
 -1
 -1
P p1 1
t1              1 0   1   e1              1.00783     0            -1 
 -1
          :
                        a1              0.458103    0            -1 
                        b1              0.0458103   0.0425975    -1 
                        b2              0.0458103   0.0425975    -1 
                        c1              0.458103    0            -1 
 -1
                                          1.00783     
 -1
P d1 1
d1              1 0   1   d1              0.0183241   0            -1 
 -1
 -1
P d2 1
d2              1 0   1   d2              0.0183241   0            -1 
 -1
 -1
 -1
