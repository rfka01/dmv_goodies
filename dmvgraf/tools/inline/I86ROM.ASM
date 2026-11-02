; Kennbuchstabe der Firmware bei 0FF9h (hwRom in DMVHWD.INC). Variablen: hwRomz
        pushf
        cli
        push ds
        xor ax,ax
        mov ds,ax
        out 11h,al
        mov bl,[0FF9h]
        out 10h,al
        pop ds
        popf
        mov [hwRomz],bl
