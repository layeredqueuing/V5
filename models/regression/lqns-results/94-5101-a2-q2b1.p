# lqns 5.19
# lqns --pragma=variance=mol,threads=hyper --no-warnings --no-advisories --parseable 94-5101-a2-q2b1.lqnx
# $Id: 94-5101-a2-q2b1.p 14556 2021-03-17 18:08:06Z greg $
V y
C 7.8559e-06
I 36
PP 5
NP 1

#!Comment:   Experiment name: q2b1  SRVN description file: web-server.in  Comment: ''   Declared variables:  n_users = 225   Controlled parameters:  [9]  n_users  = 225 
#!User:  0:00:00.047
#!Sys:   0:00:00.000
#!Real:  0:00:00.048
#!Solver: 5 180 4840 303646 1.44511e+06 7.33497e+10 0

B 9
DB             :DB_Read         0.0247525   
                DB_Update       0.0165017   
DB_Disk        :DB_Disk         0.2         
User           :User            0.0740431   
Protocol       :Protocol        0.16129     
WebServer      :WebServer       0.307007    
CGI            :CGI_Update      0.0267953   
                CGI_Read        0.016786    
WS_Disk        :WS_Disk         0.2         
-1

W 8
DB             :DB_Read         DB_Disk         0            -1 
                DB_Update       DB_Disk         0            -1 
                -1 
User           :User            Protocol        445.751      -1 
                -1 
WebServer      :WebServer       WS_Disk         3.60796      -1 
                WebServer       CGI_Update      162.303      -1 
                WebServer       CGI_Read        162.303      -1 
                -1 
CGI            :CGI_Update      DB_Update       54.0966      -1 
                CGI_Read        DB_Read         54.0966      -1 
                -1 
-1

F 1
Protocol       :Protocol        WebServer       3.9337      
                -1 
-1

X 9
DB             :DB_Read         40.4        -1 
                DB_Update       60.6        -1 
                -1 
DB_Disk        :DB_Disk         5           -1 
                -1 
User           :User            571.359     -1 
                -1 
Protocol       :Protocol        15.8603     -1 
                -1 
WebServer      :WebServer       105.814     -1 
                -1 
CGI            :CGI_Update      250.719     -1 
                CGI_Read        428.737     -1 
                -1 
WS_Disk        :WS_Disk         5           -1 
                -1 
-1

VAR 9
DB             :DB_Read         1852.16     -1 
                DB_Update       3993.79     -1 
                -1 
DB_Disk        :DB_Disk         25          -1 
                -1 
User           :User            802366      -1 
                -1 
Protocol       :Protocol        131.761     -1 
                -1 
WebServer      :WebServer       83510.5     -1 
                -1 
CGI            :CGI_Update      114292      -1 
                CGI_Read        272997      -1 
                -1 
WS_Disk        :WS_Disk         25          -1 
                -1 
-1

FQ 7
DB             :DB_Read         0.0162543   0.656674    -1 0.656674
                DB_Update       0.00403208  0.244344    -1 0.244344
                -1 
                                0.0202864   0.901018    -1 0.901018
DB_Disk        :DB_Disk         0.0892097   0.446048    -1 0.446048
                -1 
User           :User            0.0630012   35.9963     -1 35.9963
                -1 
Protocol       :Protocol        0.0630012   0.999216    -1 0.999216
                -1 
WebServer      :WebServer       0.0630012   6.66644     -1 6.66644
                -1 
CGI            :CGI_Update      0.00252005  0.631823    -1 0.631823
                CGI_Read        0.00378007  1.62066     -1 1.62066
                -1 
                                0.00630012  2.25248     -1 2.25248
WS_Disk        :WS_Disk         0.0951318   0.475659    -1 0.475659
                -1 
-1

P DB_CPU 1
DB              2  0 1  DB_Read         0.331588    0           -1 
                        DB_Update       0.123382    0           -1 
                        -1 
                                        0.454969    
-1 

P DB_Disk 1
DB_Disk         1  0 1  DB_Disk         0.446048    0           -1 
                        -1 
-1 

P User 1
User            1  0 225 User            0           0           -1 
                        -1 
-1 

P WS_CPU 3
Protocol        1  0 1  Protocol        0.390608    9.66026     -1 
                        -1 
WebServer       1  0 10 WebServer       0.618735    31.0122     -1 
                        -1 
CGI             2  0 3  CGI_Update      0.0378007   52.2042     -1 
                        CGI_Read        0.0189004   17.4014     -1 
                        -1 
                                        0.0567011   
                -1 
                                        1.06604
-1 

P WS_Disk 1
WS_Disk         1  0 1  WS_Disk         0.475659    0           -1 
                        -1 
-1 

-1

