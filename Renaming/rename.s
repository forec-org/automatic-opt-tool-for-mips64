.text
main:
ADDIU $r1, $r0, 1
ADDIU $r2, $r0, 2
LW    $r2, 3($r0)
LOOP:
ADDIU $r3, $r0, 4
LW    $r3, 2($r2)
ADDIU $r4, $r1, 1
ADDIU $r2, $r2, 1 
LW    $r2, 0($r3)
LW    $r2, 0($r1)
BEQ   $r1, $r1, LOOP
LW    $r3, 0($r4)
TEQ   $r0, $r0
