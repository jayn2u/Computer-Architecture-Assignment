ADD X7, X1, X2
SUB X8,                X3, X4
AND X9, X5, X6
SW X9,       0(X7)
ORI X10, X1, 7
LW X11, 0(X7)
XOR X12,           X11, X10
ANDI X13,          X12, 15
SLLI X14, X13, 1
SW X14, 4(X6)
ADD X15, X7, X8
SUB X16,      X9, X10
SLL X17,      X15, X2
SRL X18, X16, X1
ORI X19,            X18, 3
XORI X20,           X19, 7
AND X21, X20, X17
SLLI X22, X21, 2
SUB X23, X22,         X15
ADD X24,      X23, X10
SRAI X25,         X24, 1
ADDI X26, X25, 4
OR X27, X26, X12
ANDI X28, X27, 14
SRLI X29, X28, 1
SLL X30, X29, X3
ADD X31, X30, X8
SUB X1, X31, X5
SLLI X2, X1, 1
EXIT