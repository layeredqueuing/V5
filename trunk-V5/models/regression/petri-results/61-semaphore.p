# petrisrvn 4.4
# petrisrvn -M -p -o61-semaphore.out  61-semaphore.in
V y
C 5.7661e-06
I 38
PP 2
NP 2
#!User:  0:00:00.61
#!Sys:   0:00:00.39
#!Real:  0:00:01.24

W 3
customer : customer app_wait 6.292237 0.000000 -1
  -1
app_wait : app_wait app_signal 0.000000 0.000000 -1
     app_wait wait 0.000000 0.000000 -1
  -1
-1


Z 1
app_signal : app_signal signal 0.549999 0.000000 -1
  -1
-1


X 5
customer : customer 9.009414 0.000000 -1
  -1
app_wait : app_wait 1.599998 1.399998 -1
  -1
app_signal : app_signal 1.149998 0.000000 -1
  -1
semaphore : wait 0.699999 0.000000 -1
     signal 0.549999 0.000000 -1
  -1
-1


H 1
semaphore : wait signal 0.699999 1.549998 0.000000 
-1

FQ 4
customer : customer 0.332985 3.000000 0.000000 -1 3.000000
-1
app_wait : app_wait 0.332984 0.532774 0.466177 -1 0.998951
-1
app_signal : app_signal 0.332984 0.382931 0.000000 -1 0.382931
-1
semaphore : wait 0.332984 0.419559 0.000000 -1 0.419559
  signal 0.332984 0.329654 0.000000 -1 0.329654
-1
    0.332984 0.749213 0.000000 -1 0.749213
-1

P customer 1
customer 1 0 3 customer 0.332985 0.117187 0.000000 -1 -1
-1

P server 3
app_wait 1 0 1 app_wait 0.332984 0.000000 0.000000 -1 -1
app_signal 1 0 1 app_signal 0.332984 0.000000 0.000000 -1 -1
semaphore 2 0 1 wait 0.233089 0.000000 0.000000 -1
         signal 0.099895 0.250000 0.000000 -1 -1
      0.332984
-1
	0.998951
-1
-1

