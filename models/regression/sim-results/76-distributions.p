# lqsim 5.16
# lqsim --confidence=1.0,1000 --seed=1049217653 --parseable --output=76-distributions.p 76-distributions.in
# $Id: 76-distributions.p 13764 2020-08-17 19:50:05Z greg $
V y
C 0.480477
I 3
PP 4
NP 1

#!Comment: Distribution tests.
#!User:  5:28:49.000
#!Sys:   2:08:36.000
#!Real:  7:42:39.000

B 8
constant_client:constant_client 0.333333    
constant_server:constant_server 1           
gamma_client   :gamma_client    0.333333    
gamma_server   :gamma_server    1           
exponential_client:exponential_client 0.333333    
exponential_server:exponential_server 1           
hyperexponential_client:hyperexponential_client 0.333333    
hyperexponential_server:hyperexponential_server 1           
-1

W 4
constant_client:constant_client constant_server 0           -1 
                                            %95 0           -1 
                                            %99 0           -1 
                -1 
gamma_client   :gamma_client    gamma_server    0           -1 
                                            %95 0           -1 
                                            %99 0           -1 
                -1 
exponential_client:exponential_client exponential_server 0           -1 
                                            %95 0           -1 
                                            %99 0           -1 
                -1 
hyperexponential_client:hyperexponential_client hyperexponential_server 0           -1 
                                            %95 0           -1 
                                            %99 0           -1 
                -1 
-1

VARW 4
constant_client:constant_client constant_server 0            -1 
                                            %95 0            -1 
                                            %99 0            -1 
                -1 
gamma_client   :gamma_client    gamma_server    0            -1 
                                            %95 0            -1 
                                            %99 0            -1 
                -1 
exponential_client:exponential_client exponential_server 0            -1 
                                            %95 0            -1 
                                            %99 0            -1 
                -1 
hyperexponential_client:hyperexponential_client hyperexponential_server 0            -1 
                                            %95 0            -1 
                                            %99 0            -1 
                -1 
-1

X 8
constant_client:constant_client 2.9989      -1 
                            %95 0.00710557  -1 
                            %99 0.0163892   -1 
                -1 
constant_server:constant_server 1.00002     -1 
                            %95 0.00466714  -1 
                            %99 0.0107649   -1 
                -1 
gamma_client   :gamma_client    2.99783     -1 
                            %95 0.00734297  -1 
                            %99 0.0169368   -1 
                -1 
gamma_server   :gamma_server    1.00006     -1 
                            %95 0.00115514  -1 
                            %99 0.00266436  -1 
                -1 
exponential_client:exponential_client 2.99622     -1 
                            %95 0.0158082   -1 
                            %99 0.0364621   -1 
                -1 
exponential_server:exponential_server 0.999344    -1 
                            %95 0.00252031  -1 
                            %99 0.00581318  -1 
                -1 
hyperexponential_client:hyperexponential_client 2.99767     -1 
                            %95 0.0260103   -1 
                            %99 0.0599936   -1 
                -1 
hyperexponential_server:hyperexponential_server 0.999043    -1 
                            %95 0.00635574  -1 
                            %99 0.0146597   -1 
                -1 
-1

VAR 8
constant_client:constant_client 13.0102     -1 
                            %95 0.127657    -1 
                            %99 0.294444    -1 
                -1 
constant_server:constant_server 1.00169     -1 
                            %95 0.0095103   -1 
                            %99 0.0219358   -1 
                -1 
gamma_client   :gamma_client    11.2317     -1 
                            %95 0.0176805   -1 
                            %99 0.0407805   -1 
                -1 
gamma_server   :gamma_server    0.250163    -1 
                            %95 0.000965061 -1 
                            %99 0.00222594  -1 
                -1 
exponential_client:exponential_client 12.9324     -1 
                            %95 0.219137    -1 
                            %99 0.505445    -1 
                -1 
exponential_server:exponential_server 0.997758    -1 
                            %95 0.00941322  -1 
                            %99 0.0217119   -1 
                -1 
hyperexponential_client:hyperexponential_client 31.6163     -1 
                            %95 0.794058    -1 
                            %99 1.83152     -1 
                -1 
hyperexponential_server:hyperexponential_server 9.0061      -1 
                            %95 0.278544    -1 
                            %99 0.642471    -1 
                -1 
-1

FQ 8
constant_client:constant_client 0.333456    1           -1 1
                            %95 0.000795644 0           -1 0
                            %99 0.00183518  0           -1 0
                -1 
constant_server:constant_server 0.666642    0.666656    -1 0.666656
                            %95 0.00188604  0.00152134  -1 0.00152134
                            %99 0.00435021  0.00350903  -1 0.00350903
                -1 
gamma_client   :gamma_client    0.333575    1           -1 1
                            %95 0.00081399  0           -1 0
                            %99 0.00187749  0           -1 0
                -1 
gamma_server   :gamma_server    0.666668    0.666706    -1 0.666706
                            %95 0.000550581 0.000383755 -1 0.000383755
                            %99 0.00126993  0.000885143 -1 0.000885143
                -1 
exponential_client:exponential_client 0.333754    1           -1 1
                            %95 0.00175967  0           -1 0
                            %99 0.00405872  0           -1 0
                -1 
exponential_server:exponential_server 0.666979    0.666541    -1 0.666541
                            %95 0.000320039 0.00166132  -1 0.00166132
                            %99 0.000738181 0.00383189  -1 0.00383189
                -1 
hyperexponential_client:hyperexponential_client 0.333593    1           -1 1
                            %95 0.00289804  0           -1 0
                            %99 0.00668443  0           -1 0
                -1 
hyperexponential_server:hyperexponential_server 0.667378    0.666739    -1 0.666739
                            %95 0.00463966  0.00283714  -1 0.00283714
                            %99 0.0107015   0.00654395  -1 0.00654395
                -1 
-1

P constant 2
constant_client 1  0 1  constant_client 0.333344    0           -1 
                                    %95 0.00152134  0           -1 
                                    %99 0.00350903  0           -1 
                        -1 
constant_server 1  0 1  constant_server 0.666656    0           -1 
                                    %95 0.00152134  0           -1 
                                    %99 0.00350903  0           -1 
                        -1 
                -1 
                                        1
                                    %95 0.00215151
                                    %99 0.00496251
-1 

P exponential 2
exponential_client 1  0 1  exponential_client 0.333459    0           -1 
                                    %95 0.00166132  0           -1 
                                    %99 0.00383189  0           -1 
                        -1 
exponential_server 1  0 1  exponential_server 0.666541    0           -1 
                                    %95 0.00166132  0           -1 
                                    %99 0.00383189  0           -1 
                        -1 
                -1 
                                        1
                                    %95 0.00234946
                                    %99 0.00541911
-1 

P gamma 2
gamma_client    1  0 1  gamma_client    0.333294    0           -1 
                                    %95 0.000383755 0           -1 
                                    %99 0.000885143 0           -1 
                        -1 
gamma_server    1  0 1  gamma_server    0.666706    0           -1 
                                    %95 0.000383755 0           -1 
                                    %99 0.000885143 0           -1 
                        -1 
                -1 
                                        1
                                    %95 0.000542712
                                    %99 0.00125178
-1 

P hyperexponential 2
hyperexponential_client 1  0 1  hyperexponential_client 0.333261    0           -1 
                                    %95 0.00283714  0           -1 
                                    %99 0.00654395  0           -1 
                        -1 
hyperexponential_server 1  0 1  hyperexponential_server 0.666739    0           -1 
                                    %95 0.00283714  0           -1 
                                    %99 0.00654395  0           -1 
                        -1 
                -1 
                                        1
                                    %95 0.00401232
                                    %99 0.00925455
-1 

-1

