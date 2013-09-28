# lqns 4.3
# lqns -Pvariance=mol -M -p -odistributions2.out distributions2.in
V y
C 0
I 2
PP 4
NP 1
#!User:  0:00:00.15
#!Sys:   0:00:00.00
#!Real:  0:00:00.15
#!Solver:    4   0   0         78       19.5        8.5 1.5827e+05      39568      35696  0:00:00.15  0:00:00.00  0:00:00.15 
B 8
constant_client             : constant_client             0.333333    
constant_server             : constant_server             1           
exponential_client          : exponential_client          0.333333    
exponential_server          : exponential_server          1           
gamma_client                : gamma_client                0.333333    
gamma_server                : gamma_server                1           
hyperexponential_client     : hyperexponential_client     0.333333    
hyperexponential_server     : hyperexponential_server     1           
 -1

W 4
constant_client             : constant_client             constant_server             0            -1 
 -1
exponential_client          : exponential_client          exponential_server          0            -1 
 -1
gamma_client                : gamma_client                gamma_server                0            -1 
 -1
hyperexponential_client     : hyperexponential_client     hyperexponential_server     0            -1 
 -1
 -1


X 8
constant_client             : constant_client             3            -1 
 -1
constant_server             : constant_server             1            -1 
 -1
exponential_client          : exponential_client          3            -1 
 -1
exponential_server          : exponential_server          1            -1 
 -1
gamma_client                : gamma_client                3            -1 
 -1
gamma_server                : gamma_server                1            -1 
 -1
hyperexponential_client     : hyperexponential_client     3            -1 
 -1
hyperexponential_server     : hyperexponential_server     1            -1 
 -1
 -1


VAR 8
constant_client             : constant_client             11.3333      -1 
 -1
constant_server             : constant_server             0            -1 
 -1
exponential_client          : exponential_client          13.6667      -1 
 -1
exponential_server          : exponential_server          1            -1 
 -1
gamma_client                : gamma_client                11.9167      -1 
 -1
gamma_server                : gamma_server                0.25         -1 
 -1
hyperexponential_client     : hyperexponential_client     32.3333      -1 
 -1
hyperexponential_server     : hyperexponential_server     9            -1 
 -1
 -1

FQ 8
constant_client             : constant_client             0.333333    1            -1 1           
 -1
constant_server             : constant_server             0.666667    0.666667     -1 0.666667    
 -1
exponential_client          : exponential_client          0.333333    1            -1 1           
 -1
exponential_server          : exponential_server          0.666667    0.666667     -1 0.666667    
 -1
gamma_client                : gamma_client                0.333333    1            -1 1           
 -1
gamma_server                : gamma_server                0.666667    0.666667     -1 0.666667    
 -1
hyperexponential_client     : hyperexponential_client     0.333333    1            -1 1           
 -1
hyperexponential_server     : hyperexponential_server     0.666667    0.666667     -1 0.666667    
 -1
 -1

P constant 2
constant_client             1 0   1   constant_client             0.333333    0            -1 
 -1
constant_server             1 0   1   constant_server             0.666667    0            -1 
 -1
 -1
                                                                1           
 -1
P exponential 2
exponential_client          1 0   1   exponential_client          0.333333    0            -1 
 -1
exponential_server          1 0   1   exponential_server          0.666667    0            -1 
 -1
 -1
                                                                1           
 -1
P gamma 2
gamma_client                1 0   1   gamma_client                0.333333    0            -1 
 -1
gamma_server                1 0   1   gamma_server                0.666667    0            -1 
 -1
 -1
                                                                1           
 -1
P hyperexponential 2
hyperexponential_client     1 0   1   hyperexponential_client     0.333333    0            -1 
 -1
hyperexponential_server     1 0   1   hyperexponential_server     0.666667    0            -1 
 -1
 -1
                                                                1           
 -1
 -1

