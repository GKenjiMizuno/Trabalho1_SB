MACRO ADD_AB
    ADD A
    ADD B
ENDMACRO

SECTION TEXT
ADD_AB
INPUT OLD_DATA
LOAD OLD_DATA
L1: DIV DOIS
STORE  NEW_DATA
MULT DOIS
STORE TMP_DATA
LOAD OLD_DATA
SUB TMP_DATA
STORE TMP_DATA
OUTPUT TMP_DATA
COPY NEW_DATA, OLD_DATA
LOAD OLD_DATA
JMPP L1
STOP
SECTION DATA
DOIS: CONST 2
A: CONST 2
B: CONST 4
OLD_DATA: SPACE
NEW_DATA: SPACE
TMP_DATA: SPACE
