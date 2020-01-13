.syntax unified
.cpu cortex-m7
.thumb

.section .text.start_user_app
.global start_user_app
.type start_user_app, %function
start_user_app:
    mrs r0, ipsr            @ load special register storing current exception type
    cmp r0, #0              @ 0 means processor is running in thread mode
    beq in_thread_mode      @ exit handler mode if necessary
        mvn r1, #7      @ align stackpointer at 8 bytes  
        and r0, r1
        mov sp, r0      @ save aligned stackpointer
        isb             @ sync due to stackpointer change  
        ldr r0, =start_user_app     @ load continuation address for when we're in thread mode
        str r0, [sp, #0x18]         @ overwrite the saved pc for exception return
        mrs r0, xpsr            @ mask off the last 9 bits of xPSR (the current ISR number),
        ldr r1, xpsr_mask       @ since we want to skip all pending exception handlers
        and r0, r1
        orr r0, r0, #0x1000000  @ set thumb bit (this bit is never read by the msr instruction)
        str r0, [sp, #0x1c]     @ store the modified xPSR for recovery on exception return
        ldr lr, exception_return    @ write magic value that forces return to thread mode
        bx lr                       @ return to thread mode
    in_thread_mode:
    ldr r0, scb_vtor_addr       @ load address of VTOR register
    ldr r1, stackpointer_addr   @ load user application image address
    str r1, [r0]                @ relocate to user supplied isr vector
    ldr r0, stackpointer_addr   @ load stackpointer address for user application
    ldr r1, entry_point_addr    @ load entry point address for user application
    ldr sp, [r0]    @ set new stackpointer
    isb             @ sync due to stackpointer change            
    ldr r1, [r1]    @ load entry point itself
    blx r1          @ call user application
                   
    infinite_loop:
    b infinite_loop

    xpsr_mask:
    .word 0xfffffe00
    exception_return:
    .word 0xfffffff9
    scb_vtor_addr:
    .word 0xe000ed08
    stackpointer_addr:
    .word 0x90000000
    entry_point_addr:
    .word 0x90000004
.size start_user_app, .-start_user_app
