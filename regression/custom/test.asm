        device zxspectrum128

        org #8000
        ld hl,0
incl    push hl
        ld de,16384
        ld bc,6144
        ldir
        pop hl
        inc l
        jr nz,incl
