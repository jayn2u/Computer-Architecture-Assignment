ADD x7, x1, x2
SuB x8, x3, x4
AND x9, x5, x6
SW x9, 0(x7)
ORI x10, x1, 7
LW x11, 0(x7)
XOR x12, x11, x10
ANDI x13, x12, 15
SLLI x14, x13, 1
SW x14, 4(x6)
ADD x15, x7, x8
SUB x16, x9, x10
SLL x17, x15, x2
SrL x18, x16, x1
ORI x19, x18, 3
XoRI x20, x19, 7
AND x21, x20, x17
SlLI x22, x21, 2
SUB x23, x22, x15
ADD x24, x23, x10
SRAI x25, x24, 1
ADDI x26, x25, 4
OR x27, x26, x12
ANdI x28, x27, 14
SRLI x29, x28, 1
SLL x30, x29, x3
ADD x31, x30, x8
SuB x1, x31, x5
SlLI x2, x1, 1
EXIT