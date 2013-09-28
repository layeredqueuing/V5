# lqns 4.3
# lqns -Pvariance=mol -M -p -odistributions1.out distributions1.in
V y
C 0
I 2
PP 5
NP 1
#!User:  0:00:00.01
#!Sys:   0:00:00.00
#!Real:  0:00:00.00
#!Solver:    2   0   0         26         13          0      16900       8450          0  0:00:00.01  0:00:00.00  0:00:00.00 
B 5
constant            : constant            0.333333    
exponential         : exponential         0.333333    
gamma               : gamma               0.333333    
hyperexponential    : hyperexponential    0.333333    
pareto              : pareto              0.333333    
 -1


X 5
constant            : constant            3            -1 
 -1
exponential         : exponential         3            -1 
 -1
gamma               : gamma               3            -1 
 -1
hyperexponential    : hyperexponential    3            -1 
 -1
pareto              : pareto              3            -1 
 -1
 -1


VAR 5
constant            : constant            0            -1 
 -1
exponential         : exponential         9            -1 
 -1
gamma               : gamma               2            -1 
 -1
hyperexponential    : hyperexponential    81           -1 
 -1
pareto              : pareto              9            -1 
 -1
 -1

FQ 5
constant            : constant            0.333333    1            -1 1           
 -1
exponential         : exponential         0.333333    1            -1 1           
 -1
gamma               : gamma               0.333333    1            -1 1           
 -1
hyperexponential    : hyperexponential    0.333333    1            -1 1           
 -1
pareto              : pareto              0.333333    1            -1 1           
 -1
 -1

P constant 1
constant            1 0   1   constant            1           0            -1 
 -1
 -1
P exponential 1
exponential         1 0   1   exponential         1           0            -1 
 -1
 -1
P gamma 1
gamma               1 0   1   gamma               1           0            -1 
 -1
 -1
P hyperexponential 1
hyperexponential    1 0   1   hyperexponential    1           0            -1 
 -1
 -1
P pareto 1
pareto              1 0   1   pareto              1           0            -1 
 -1
 -1
 -1

