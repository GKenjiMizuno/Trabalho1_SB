;=============================================
; module2.asm
;=============================================
SECTION TEXT
MOD2: BEGIN
PUBLIC EXT_SYM      ; Export the symbol EXT_SYM so module1 can link to it
EXTERN MYFUNC       ; We'll call MYFUNC from module1

;---------------------------------------------
; Some instructions referencing EXT_SYM
; Then we jump to MYFUNC
;---------------------------------------------
    LOAD EXT_SYM     ; Load current value of EXT_SYM
    ADD DEZ          ; Add 10 to it
    STORE EXT_SYM    ; Store back the incremented value
    JMP MYFUNC       ; Jump to MYFUNC (defined in module1)
END

SECTION DATA
DEZ: CONST 10        ; A constant value 10
EXT_SYM: CONST 5    ; The external symbol that module1 is referring to
