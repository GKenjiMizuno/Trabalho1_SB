;=============================================
; module1.asm
;=============================================
SECTION TEXT
MOD1: BEGIN
PUBLIC MYFUNC       ; Export MYFUNC so other modules can call/use it
EXTERN EXT_SYM      ; Declare EXT_SYM is defined in another module

;---------------------------------------------
; MYFUNC: a small routine that uses EXT_SYM
;---------------------------------------------
MYFUNC:
    LOAD VAR1        ; Accumulate the value in VAR1
    ADD EXT_SYM      ; Add the external symbol (defined in module2)
    STORE VAR1       ; Store result back in VAR1
    STOP             ; End program (or function)
END

SECTION DATA
VAR1: SPACE         ; Reserve 1 word of data
