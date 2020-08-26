# lqns 5.17
# lqns --pragma=variance=mol,threads=hyper --parseable 94-5101-a2-q2b1.in
# $Id: 94-5101-a2-q2b1.p 13793 2020-08-24 15:45:01Z greg $
V y
C 7.02981e-06
I 34
PP 5
NP 1

#!Comment: \n	Experiment name: q2b1\n	SRVN description file: web-server.in\n	Comment: ''\n\n	Declared variables:\n	n_users = 225\n\n	Controlled parameters:\n	[9]  n_users  = 225\n
#!User:  0:00:00.033
#!Sys:   0:00:00.000
#!Real:  0:00:00.033
#!Solver: 5 170 1963 59701 701871 5.80608e+10 0

B 9
User           :User            0.0740431   
Protocol       :Protocol        0.16129     
WebServer      :WebServer       0.307007    
CGI            :CGI_Update      0.0267953   
                CGI_Read        0.016786    
WS_Disk        :WS_Disk         0.2         
DB             :DB_Read         0.0247525   
                DB_Update       0.0165017   
DB_Disk        :DB_Disk         0.2         
-1

W 9
User           :User            Protocol        447.514     -1 
                -1 
WebServer      :WebServer       CGI_Update      161.291     -1 
                WebServer       CGI_Read        161.291     -1 
                WebServer       WS_Disk         3.5987      -1 
                -1 
CGI            :CGI_Update      DB_Update       54.0241     -1 
                CGI_Read        DB_Read         54.0241     -1 
                -1 
DB             :DB_Read         DB_Disk         0           -1 
                DB_Update       DB_Disk         0           -1 
                -1 
Protocol       :Protocol        WebServer       3.84025     -1 
                -1 
-1

X 9
User           :User            572.816     -1 
                -1 
Protocol       :Protocol        15.8444     -1 
                -1 
WebServer      :WebServer       105.618     -1 
                -1 
CGI            :CGI_Update      250.516     -1 
                CGI_Read        428.396     -1 
                -1 
WS_Disk        :WS_Disk         5           -1 
                -1 
DB             :DB_Read         40.4        -1 
                DB_Update       60.6        -1 
                -1 
DB_Disk        :DB_Disk         5           -1 
                -1 
-1

VAR 9
User           :User            806106      -1 
                -1 
Protocol       :Protocol        131.454     -1 
                -1 
WebServer      :WebServer       83115.7     -1 
                -1 
CGI            :CGI_Update      114126      -1 
                CGI_Read        272568      -1 
                -1 
WS_Disk        :WS_Disk         25          -1 
                -1 
DB             :DB_Read         1852.16     -1 
                DB_Update       3993.79     -1 
                -1 
DB_Disk        :DB_Disk         25          -1 
                -1 
-1

FQ 7
User           :User            0.0629755   36.0734     -1 36.0734
                -1 
Protocol       :Protocol        0.0629755   0.997809    -1 0.997809
                -1 
WebServer      :WebServer       0.0629755   6.65132     -1 6.65132
                -1 
CGI            :CGI_Update      0.00251902  0.631055    -1 0.631055
                CGI_Read        0.00377854  1.61871     -1 1.61871
                -1 
                                0.00629756  2.24977     -1 2.24977
WS_Disk        :WS_Disk         0.0950932   0.475466    -1 0.475466
                -1 
DB             :DB_Read         0.0162477   0.656408    -1 0.656408
                DB_Update       0.00403044  0.244245    -1 0.244245
                -1 
                                0.0202782   0.900653    -1 0.900653
DB_Disk        :DB_Disk         0.0891735   0.445868    -1 0.445868
                -1 
-1

P User 1
User            1  0 225 User            0           0           -1 
                        -1 
-1 

P WS_Disk 1
WS_Disk         1  0 1  WS_Disk         0.475466    0           -1 
                        -1 
-1 

P WS_CPU 3
Protocol        1  0 1  Protocol        0.390448    9.64439     -1 
                        -1 
WebServer       1  0 10 WebServer       0.618483    30.959      -1 
                        -1 
CGI             2  0 3  CGI_Update      0.0377854   52.1172     -1 
                        CGI_Read        0.0188927   17.3724     -1 
                        -1 
                                        0.056678    
                -1 
                                        1.06561
-1 

P DB_CPU 1
DB              2  0 1  DB_Read         0.331453    0           -1 
                        DB_Update       0.123332    0           -1 
                        -1 
                                        0.454785    
-1 

P DB_Disk 1
DB_Disk         1  0 1  DB_Disk         0.445868    0           -1 
                        -1 
-1 

-1

