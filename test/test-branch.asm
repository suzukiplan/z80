org $0000

.Start
    call Sub1
    nop

.Sub1
    call Sub1_1
    cmp $FF
    call c, Sub1_2
    call nc, Sub1_3
    ret z
    ret nz

.Sub1_1
    ld a, $01
    ret

.Sub1_2
    ld a, $02
    ret

.Sub1_3
    ld a, $03
    ret
