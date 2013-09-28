# petrisrvn 4.3
# petrisrvn -M -p -odistributions2.out  distributions2.in
V y
C 9.5491e-06
I 512
PP 4
NP 1
#!User:  0:00:00.38
#!Sys:   0:00:00.61
#!Real:  0:00:01.34

W 4
constant_client : constant_client constant_server 0.000000 -1
  -1
gamma_client : gamma_client gamma_server 0.000000 -1
  -1
exponential_client : exponential_client exponential_server 0.000000 -1
  -1
hyperexponential_client : hyperexponential_client hyperexponential_server 0.000000 -1
  -1
-1


X 8
constant_client : constant_client 3.000003 -1
  -1
constant_server : constant_server 1.000000 -1
  -1
gamma_client : gamma_client 3.000003 -1
  -1
gamma_server : gamma_server 1.000000 -1
  -1
exponential_client : exponential_client 3.000003 -1
  -1
exponential_server : exponential_server 1.000000 -1
  -1
hyperexponential_client : hyperexponential_client 2.999616 -1
  -1
hyperexponential_server : hyperexponential_server 0.999807 -1
  -1
-1

FQ 8
constant_client : constant_client 0.333333 1.000000 -1 1.000000
-1
constant_server : constant_server 0.666667 0.666667 -1 0.666667
-1
gamma_client : gamma_client 0.333333 1.000000 -1 1.000000
-1
gamma_server : gamma_server 0.666667 0.666667 -1 0.666667
-1
exponential_client : exponential_client 0.333333 1.000000 -1 1.000000
-1
exponential_server : exponential_server 0.666667 0.666667 -1 0.666667
-1
hyperexponential_client : hyperexponential_client 0.333376 1.000000 -1 1.000000
-1
hyperexponential_server : hyperexponential_server 0.666743 0.666614 -1 0.666614
-1
-1

P constant 2
constant_client 1 0 1 constant_client 0.333333 0.000000 -1 -1
constant_server 1 0 1 constant_server 0.666667 0.000000 -1 -1
-1
	1
-1

P exponential 2
exponential_client 1 0 1 exponential_client 0.333333 0.000000 -1 -1
exponential_server 1 0 1 exponential_server 0.666667 0.000000 -1 -1
-1
	1
-1

P gamma 2
gamma_client 1 0 1 gamma_client 0.333333 0.000000 -1 -1
gamma_server 1 0 1 gamma_server 0.666667 0.000000 -1 -1
-1
	1
-1

P hyperexponential 2
hyperexponential_client 1 0 1 hyperexponential_client 0.333386 0.000000 -1 -1
hyperexponential_server 1 0 1 hyperexponential_server 0.666614 0.000000 -1 -1
-1
	1
-1
-1

