* Subroutines needed by GCC output code on some machines.  
* Uniflex returns double as pointer in a0, so convert to d0,d1 

	lib sysdef
	lib sysfloat

    text

    global _uniflex_atof
_uniflex_atof
    jsr _atof
    move.l (a0),d0
    move.l 4(a0),d1
    rts


    global ___divdf3
___divdf3
    jsr ___uniflex_divdf3
    move.l (a0),d0
    move.l 4(a0),d1
    rts

    global ___muldf3
___muldf3
    jsr ___uniflex_muldf3
    move.l (a0),d0
    move.l 4(a0),d1
    rts

    global ___negdf2
___negdf2
    jsr ___uniflex_negdf2
    move.l (a0),d0
    move.l 4(a0),d1
    rts

    global ___adddf3
___adddf3
    jsr ___uniflex_adddf3
    move.l (a0),d0
    move.l 4(a0),d1
    rts

    global ___subdf3
___subdf3
    jsr ___uniflex_subdf3
    move.l (a0),d0
    move.l 4(a0),d1
    rts

    global ___floatsidf
___floatsidf
    jsr ___uniflex_floatsidf
    move.l (a0),d0
    move.l 4(a0),d1
    rts

    global ___floatunsidf
___floatunsidf
    jsr ___uniflex_floatunsidf
    move.l (a0),d0
    move.l 4(a0),d1
    rts

    global ___extendsfdf2
___extendsfdf2
    jsr ___uniflex_extendsfdf2
    move.l (a0),d0
    move.l 4(a0),d1
    rts

