# petrisrvn 4.4
# petrisrvn -M -p -o62-semaphore.out  62-semaphore.in
V y
C 6.6666e-06
I 43
PP 2
NP 1
#!User:  0:00:00.58
#!Sys:   0:00:00.38
#!Real:  0:00:01.18

W 3
customer : customer app_wait 3.296371 -1
  -1
app_wait : app_wait wait 0.777388 -1
  -1
customer : customer app_signal 0.000000 -1
  -1
-1


Z 1
app_signal : app_signal signal 0.796944 -1
  -1
-1


X 5
customer : customer 7.399815 -1
  -1
app_wait : app_wait 2.975869 -1
  -1
app_signal : app_signal 1.637777 -1
  -1
semaphore : wait 0.943889 -1
     signal 0.796944 -1
  -1
-1


H 1
semaphore : wait signal 0.943889 2.037777 0.000000 
-1

FQ 4
customer : customer 0.331947 3.000000 -1 3.000000
-1
app_wait : app_wait 0.331946 0.987828 -1 0.987828
-1
app_signal : app_signal 0.331946 0.543654 -1 0.543654
-1
semaphore : wait 0.331946 0.536649 -1 0.536649
  signal 0.331946 0.453103 -1 0.453103
-1
    0.331946 0.989752 -1 0.989752
-1

P customer 1
customer 1 0 3 customer 0.331947 0.127585 -1 -1
-1

P server 3
app_wait 1 0 1 app_wait 0.331946 0.509183 -1 -1
app_signal 1 0 1 app_signal 0.331946 0.240833 -1 -1
semaphore 2 0 1 wait 0.232362 0.243889 -1
         signal 0.099584 0.496944 -1 -1
      0.331946
-1
	0.995838
-1
-1

