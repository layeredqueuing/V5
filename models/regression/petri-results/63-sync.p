# petrisrvn 5.17
# petrisrvn --parseable --output=63-sync.out 63-sync.in
# $Id: 63-sync.p 13875 2020-09-25 20:02:38Z greg $
V y
C 1.28334e-07
I 0
PP 2
NP 2

#!Comment: Sync-server called from a sequence of phases.
#!User:  0:00:00.073
#!Sys:   0:00:00.045
#!Real:  0:00:00.929

W 3
customer       :customer        app_wait        4.07226     0           -1 
                customer        app_signal      0           0           -1 
                -1 
app_wait       :app_wait        wait            1.27219     0           -1 
                -1 
-1

Z 1
app_signal     :app_signal      signal          0.550009    0           -1 
                -1 
-1

DP 1
app_signal     :app_signal      signal          0            0            -1 
                -1 
-1

J 0
semaphore      :wait            signal          2.27223     0          
-1 
-1

X 5
customer       :customer        7.79446     1.87223     -1 
                -1 
app_wait       :app_wait        3.2222      0           -1 
                -1 
app_signal     :app_signal      1.37223     0           -1 
                -1 
semaphore      :signal          0.550009    0          -1 
                wait            0.949998    2.27223    -1 
                -1
:
                done            0           -1 
                signal          0.550009    -1 
                wait            0.949998    -1 
                -1 
-1

FQ 4
customer       :customer        0.310344    2.41896     0.581035    -1 3
                -1 
app_wait       :app_wait        0.310344    0.999992    0           -1 0.999992
                -1 
app_signal     :app_signal      0.310344    0.425863    0           -1 0.425863
                -1 
semaphore      :signal          0.310344    0.145804    0          -1 0.145804
                wait            0.310344    0.251839    0.602355   -1 0.854194
                -1
:
                done            0.310344    0           -1 
                signal          0.310344    0.170692    -1 
                wait            0.310344    0.294826    -1 
                -1 
                                0.310344    0.397643    0.602355    -1 0.999999
-1

P customer 1
customer        1  0 3  customer        0.310344    0           0           -1 
                        -1 
-1 

P server 3
app_wait        1  0 1  app_wait        0.310344    2.71734e-05 0           -1 
                        -1 
app_signal      1  0 1  app_signal      0.310344    0.222221    0           -1 
                        -1 
semaphore       2  0 1  signal          0.0931031   0           0          -1 
                        wait            0.217241    0           0          -1 
-1
                       :
                        done            0           0           -1 
                        signal          0.0931031   0.250009    -1 
                        wait            0.217241    0.249998    -1 
                        -1 
                                        0.310344    
                -1 
                                        0.931031
-1 

-1

