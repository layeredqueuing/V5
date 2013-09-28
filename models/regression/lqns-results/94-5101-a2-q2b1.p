# lqns 5.4
# lqns --pragma=variance=mol,threads=mak --parseable 94-5101-a2-q2b1.in
V y
C 9.92874e-06
I 67
PP 5
NP 1
#!User:  0:00:04.00
#!Sys:   0:00:00.00
#!Real:  0:00:05.00
#!Solver: 5 335 3220 71780 914676 6.19655e+10 0

B 9
CGI            :CGI_Update      0.0267953   
                CGI_Read        0.016786    
DB             :DB_Read         0.0247525   
                DB_Update       0.0165017   
DB_Disk        :DB_Disk         0.2         
Protocol       :Protocol        0.16129     
User           :User            0.0740431   
WS_Disk        :WS_Disk         0.2         
WebServer      :WebServer       0.307007    
-1

W 0
CGI            :CGI_Update      DB_Update       54.0243     -1 
                CGI_Read        DB_Read         54.0243     -1 
  -1 
DB             :DB_Read         DB_Disk         0           -1 
                DB_Update       DB_Disk         0           -1 
  -1 
User           :User            Protocol        447.508     -1 
  -1 
WebServer      :WebServer       CGI_Read        161.295     -1 
                WebServer       CGI_Update      161.295     -1 
                WebServer       WS_Disk         3.5987      -1 
  -1 
-1

X 9
CGI            :CGI_Update      250.516     -1 
                CGI_Read        428.397     -1 
  -1 
DB             :DB_Read         40.4        -1 
                DB_Update       60.6        -1 
  -1 
DB_Disk        :DB_Disk         5           -1 
  -1 
Protocol       :Protocol        15.8445     -1 
  -1 
User           :User            572.811     -1 
  -1 
WS_Disk        :WS_Disk         5           -1 
  -1 
WebServer      :WebServer       105.618     -1 
  -1 
-1

VAR 9
CGI            :CGI_Update      114126      -1 
                CGI_Read        272570      -1 
  -1 
DB             :DB_Read         1852.16     -1 
                DB_Update       3993.79     -1 
  -1 
DB_Disk        :DB_Disk         25          -1 
  -1 
Protocol       :Protocol        131.456     -1 
  -1 
User           :User            806095      -1 
  -1 
WS_Disk        :WS_Disk         25          -1 
  -1 
WebServer      :WebServer       83117       -1 
  -1 
-1

FQ 7
CGI            :CGI_Update      0.00251903  0.631058    -1 0.631058   
                CGI_Read        0.00377854  1.61872     -1 1.61872    
  -1 
                                0.00629757  2.24977     -1 2.24977    
DB             :DB_Read         0.0162477   0.656409    -1 0.656409   
                DB_Update       0.00403045  0.244245    -1 0.244245   
  -1 
                                0.0202782   0.900654    -1 0.900654   
DB_Disk        :DB_Disk         0.0891736   0.445868    -1 0.445868   
  -1 
Protocol       :Protocol        0.0629756   0.997815    -1 0.997815   
  -1 
User           :User            0.0629756   36.0731     -1 36.0731    
  -1 
WS_Disk        :WS_Disk         0.0950933   0.475466    -1 0.475466   
  -1 
WebServer      :WebServer       0.0629756   6.65137     -1 6.65137    
  -1 
-1

P DB_CPU 1
DB              2  0 1  DB_Read         0.331454    0           -1 
                        DB_Update       0.123332    0           -1 
                        -1 
                                        0.454786    
  -1 

P DB_Disk 1
DB_Disk         1  0 1  DB_Disk         0.445868    0           -1 
                        -1 
  -1 

P User 1
User            1  0 225 User            0           0           -1 
                        -1 
  -1 

P WS_CPU 3
Protocol        1  0 1  Protocol        0.390449    9.64447     -1 
                        -1 
WebServer       1  0 10 WebServer       0.618484    30.9592     -1 
                        -1 
CGI             2  0 3  CGI_Update      0.0377854   52.1175     -1 
                        CGI_Read        0.0188927   17.3725     -1 
                        -1 
                                        0.0566781   
  -1 
                                        1.06561     
  -1 

P WS_Disk 1
WS_Disk         1  0 1  WS_Disk         0.475466    0           -1 
                        -1 
  -1 

-1

