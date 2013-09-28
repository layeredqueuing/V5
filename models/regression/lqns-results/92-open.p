# lqns 3.9
# lqns -Pvariance=mol -p -oopen2.out open2.in
V y
C 0
I 2
PP 5
NP 1
#!User:  0:00:00.00
#!Sys:   0:00:00.00
#!Real:  0:00:00.00
#!Solver:    2   0   0         27       13.5        2.5      12064       6032       2160  0:00:00.00  0:00:00.00  0:00:00.00 
B 5
WSP             : WSP             0.0584112   
DBD             : DBD             0.095057    
choose          : choose          0.021777    
WSD             : WSD             0.13245     
DBP             : DBP             0.0931966   
 -1

W 4
choose          : choose          WSP             3.53637      -1 
                  choose          WSD             0.616577     -1 
                  choose          DBP             1.28972      -1 
                  choose          DBD             1.23682      -1 
 -1
 -1


X 5
WSP             : WSP             17.12        -1 
 -1
DBD             : DBD             10.52        -1 
 -1
choose          : choose          52.5995      -1 
 -1
WSD             : WSD             7.55         -1 
 -1
DBP             : DBP             10.73        -1 
 -1
 -1


VAR 5
WSP             : WSP             293.094      -1 
 -1
DBD             : DBD             110.67       -1 
 -1
choose          : choose          4457.19      -1 
 -1
WSD             : WSD             57.0025      -1 
 -1
DBP             : DBP             115.133      -1 
 -1
 -1

FQ 5
WSP             : WSP             0.01        0.1712       -1 0.1712      
 -1
DBD             : DBD             0.01        0.1052       -1 0.1052      
 -1
choose          : choose          0.01        0.525995     -1 0.525995    
 -1
WSD             : WSD             0.01        0.0755       -1 0.0755      
 -1
DBP             : DBP             0.01        0.1073       -1 0.1073      
 -1
 -1



R 1
choose          choose          0.01        51.9315     
 -1

P WSP 1
WSP             1 0   1   WSP             0.1712      0            -1 
 -1
 -1
P DBD 1
DBD             1 0   1   DBD             0.1052      0            -1 
 -1
 -1
P choose 1
choose          1 0   1   choose          1e-07       0            -1 
 -1
 -1
P WSD 1
WSD             1 0   1   WSD             0.0755      0            -1 
 -1
 -1
P DBP 1
DBP             1 0   1   DBP             0.1073      0            -1 
 -1
 -1
 -1

