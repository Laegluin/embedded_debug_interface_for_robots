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
        ldr r0, =in_thread_mode     @ load continuation address for when we're in thread mode
        str r0, [sp, #0x18]         @ overwrite the saved pc for exception return
        ldr lr, exception_return    @ write magic value that forces return to thread mode
        bx lr                       @ return to thread mode
    in_thread_mode:
    ldr r0, stackpointer_addr   @ load stackpointer for user application
    ldr r1, entry_point_addr    @ load entry point for user application
    ldr sp, [r0]    @ set new stackpointer
    isb             @ sync due to stackpointer change            
    blx r1          @ call user application
                   
    infinite_loop:
    b infinite_loop

    exception_return:
    .word 0xfffffff9
    stackpointer_addr:
    .word 0x90000000
    entry_point_addr:
    .word 0x90000004
.size start_user_app, .-start_user_app
