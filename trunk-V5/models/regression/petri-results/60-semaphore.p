# petrisrvn 4.4
# petrisrvn -M -p -o60-semaphore.out  60-semaphore.in
V y
C 9.3615e-06
I 33
PP 2
NP 2
#!User:  0:00:00.96
#!Sys:   0:00:00.40
#!Real:  0:00:01.64

W 3
customer : customer app_wait 4.097962 0.000000 -1
     customer app_signal 0.000000 0.000000 -1
  -1
app_wait : app_wait wait 1.313909 0.000000 -1
  -1
-1


Z 1
app_signal : app_signal signal 0.550000 0.000000 -1
  -1
-1


X 5
customer : customer 7.918873 1.872857 -1
  -1
app_wait : app_wait 3.263910 0.000000 -1
  -1
app_signal : app_signal 1.356908 0.000000 -1
  -1
semaphore : wait 0.950000 0.000000 -1
     signal 0.550000 0.000000 -1
  -1
-1


H 1
semaphore : wait signal 0.950000 2.313910 0.000000 
-1

FQ 4
customer : customer 0.306381 2.426192 0.573808 -1 3.000000
-1
app_wait : app_wait 0.306381 1.000000 0.000000 -1 1.000000
-1
app_signal : app_signal 0.306381 0.415731 0.000000 -1 0.415731
-1
semaphore : wait 0.306381 0.633333 0.000000 -1 0.633333
  signal 0.306381 0.366667 0.000000 -1 0.366667
-1
    0.306381 1.000000 0.000000 -1 1.000000
-1

P customer 1
customer 1 0 3 customer 0.306381 0.031898 0.031898 -1 -1
-1

P server 3
app_wait 1 0 1 app_wait 0.306381 0.000000 0.000000 -1 -1
app_signal 1 0 1 app_signal 0.306381 0.206908 0.000000 -1 -1
semaphore 2 0 1 wait 0.214467 0.250000 0.000000 -1
         signal 0.091914 0.250000 0.000000 -1 -1
      0.306381
-1
	0.919143
-1
-1

