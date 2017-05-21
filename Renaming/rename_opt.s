.text
main:
ADDIU $r1    ,$r0    ,1      
ADDIU $r2    ,$r0    ,2      
LW    $r7    ,3($r0) 
LOOP:
ADDIU $r3    ,$r0    ,4      
LW    $r6    ,2($r7) 
ADDIU $r4    ,$r1    ,1      
ADDIU $r7    ,$r7    ,1      
LW    $r5    ,0($r6) 
LW    $r5    ,0($r1) 
BEQ   $r1    ,$r1    ,LOOP   
LW    $r6    ,0($r4) 
TEQ   $r0    ,$r0    
