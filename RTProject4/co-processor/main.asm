; Co-processor code 
    LIST        P=16F877A
    #include    <P16F877A.INC>
    __CONFIG    _HS_OSC & _WDT_OFF & _PWRTE_ON & _LVP_OFF & _CP_OFF & _BODEN_OFF

; Communication Definitions
INT_PIN     EQU 0        ; RB0 for interrupt in
ACK_PIN     EQU 1        ; RB1 for acknowledgment out
DATA_PORT   EQU PORTC    ; PORTC for data transfer

; Primary Registers
CBLOCK 0x20
    byteCount, currentByte, tempCounter
    srcPtr, dstPtr, temp, BCount
    nratorH, nratorM4, nratorM3, nratorM2, nratorM1, nratorL
    denomH,  denomM4,  denomM3,  denomM2,  denomM1,  denomL 
ENDC

CBLOCK 0x34
    ascii_buf:12
    ascii_res:12
ENDC

CBLOCK 0x50
    idx, STATUS_SAV, ASCPTR
ENDC

CBLOCK 0x53
    BIN0, BIN1, BIN2, BIN3, BIN4, BIN5
    REM, PTR, BYTECTR, BIGHI, BIGHLO, QUOTB
    TMP0, TMP1, TMP2, TMP3, TMP4, TMP5
    DIGIT
ENDC

CBLOCK 0x66	
    TMP1xL, TMP1xM1, TMP1xM2, TMP1xM3, TMP1xM4, TMP1xH
    TMP2xL, TMP2xM1, TMP2xM2, TMP2xM3, TMP2xM4, TMP2xH
    A_tempL, A_tempM1, A_tempM2, A_tempM3, A_tempM4, A_tempH
    shiftH, shiftM4, shiftM3, shiftM2, shiftM1, shiftL 
ENDC

CBLOCK 0xA0
    TMP1L, TMP1M1, TMP1M2, TMP1M3, TMP1M4, TMP1H
    TMP2L, TMP2M1, TMP2M2, TMP2M3, TMP2M4, TMP2H
ENDC

CBLOCK 0xB4
    receiveBuffer:24
    sendBuffer:24
ENDC

    ORG 0x0000
    GOTO MAIN

MAIN:
    BANKSEL TRISD
    CLRF    TRISD
    BANKSEL PORTD
    CLRF    PORTD

    BCF     STATUS, RP1
    BCF     STATUS, RP0
    BCF     STATUS, IRP

    ; Initialize ports
    BANKSEL TRISB
    BSF     TRISB, INT_PIN
    BCF     TRISB, ACK_PIN
    BANKSEL TRISC
    MOVLW   0xFF
    MOVWF   TRISC
    BANKSEL PORTB
    BCF     PORTB, ACK_PIN

; MAIN PROCESSING LOOP
WaitAndDivide:
    ; Complete system reset
    CALL    COMPLETE_SYSTEM_RESET
    
    ; Wait for new data from master
    CALL    RECEIVE_DATA_BLOCK
    
    ; Process the received data
    CALL    DIVIDE_AND_PREPARE_RESPONSE
    CALL    SEND_DATA_BLOCK

    ; Loop back for next operation
    GOTO    MAIN

; ===============================================
; ensures clean state for each new operation
; ===============================================
COMPLETE_SYSTEM_RESET:
    BCF     STATUS, RP1
    BCF     STATUS, RP0
    BCF     STATUS, IRP
    
    ; Clear all calculation buffers
    CLRF    nratorH
    CLRF    nratorM4
    CLRF    nratorM3
    CLRF    nratorM2
    CLRF    nratorM1
    CLRF    nratorL
    CLRF    denomH
    CLRF    denomM4
    CLRF    denomM3
    CLRF    denomM2
    CLRF    denomM1
    CLRF    denomL
    
    ; Clear BIN registers
    CLRF    BIN0
    CLRF    BIN1
    CLRF    BIN2
    CLRF    BIN3
    CLRF    BIN4
    CLRF    BIN5
    
    ; Clear temporary calculation registers
    CLRF    TMP1xL
    CLRF    TMP1xM1
    CLRF    TMP1xM2
    CLRF    TMP1xM3
    CLRF    TMP1xM4
    CLRF    TMP1xH
    
    ; Clear ascii buffers
    CALL    CLEAR_ASCII_BUFFERS
    
    ; Clear receive buffer 
    CALL    CLEAR_RECEIVE_BUFFER
    
    ; Reset communication state
    BANKSEL TRISC
    MOVLW   0xFF
    MOVWF   TRISC       
    BANKSEL PORTB
    BCF     PORTB, ACK_PIN 
    
    RETURN

; ===============================================
; Clear ASCII Buffers
; ===============================================
CLEAR_ASCII_BUFFERS:
    ; Clear ascii_buf
    MOVLW   LOW ascii_buf
    MOVWF   FSR
    MOVLW   D'12'
    MOVWF   tempCounter
CLEAR_ASCII_BUF_LOOP:
    CLRF    INDF
    INCF    FSR, F
    DECFSZ  tempCounter, F
    GOTO    CLEAR_ASCII_BUF_LOOP
    
    ; Clear ascii_res
    MOVLW   LOW ascii_res
    MOVWF   FSR
    MOVLW   D'12'
    MOVWF   tempCounter
CLEAR_ASCII_RES_LOOP:
    CLRF    INDF
    INCF    FSR, F
    DECFSZ  tempCounter, F
    GOTO    CLEAR_ASCII_RES_LOOP
    
    RETURN

; ===============================================
; Clear Receive Buffer 
; ===============================================
CLEAR_RECEIVE_BUFFER:
    BCF     STATUS, IRP      
    MOVLW   LOW receiveBuffer
    MOVWF   FSR
    MOVLW   D'24'             
    MOVWF   tempCounter
CLEAR_RCV_BUF_LOOP:
    CLRF    INDF
    INCF    FSR, F
    DECFSZ  tempCounter, F
    GOTO    CLEAR_RCV_BUF_LOOP
    RETURN

; =============================================================
; RECEIVE_DATA_BLOCK 
; =============================================================
RECEIVE_DATA_BLOCK:
    BANKSEL TRISC
    MOVLW   0xFF
    MOVWF   TRISC              ; PORTC as input
    BANKSEL PORTB
    BCF     PORTB, ACK_PIN       
    CALL    DELAY_200MS        
    
WAIT_INT_IDLE:
    BTFSC   PORTB, INT_PIN     ; Wait for INT to go low 
    GOTO    WAIT_INT_IDLE
    
    ; Prepare to receive 24 bytes
    BCF     STATUS, IRP        
    MOVLW   LOW receiveBuffer
    MOVWF   FSR
    MOVLW   D'24'             
    MOVWF   byteCount
    
RECEIVE_LOOP:
    CALL    RECEIVE_BYTE
    MOVF    currentByte, W
    MOVWF   INDF
    INCF    FSR, F
    DECFSZ  byteCount, F
    GOTO    RECEIVE_LOOP
    
    CALL    DELAY_100US       
    BCF     PORTB, ACK_PIN   
    
    RETURN

; ===============================================
; RECEIVE_BYTE 
; ===============================================
RECEIVE_BYTE:
    ; Wait for master to signal data ready (INT high)
WAIT_INT_HIGH_C:
    BTFSS   PORTB, INT_PIN
    GOTO    WAIT_INT_HIGH_C
    
    CALL    DELAY_100US
    
    ; Read data from PORTC
    BANKSEL PORTC
    MOVF    PORTC, W
    MOVWF   currentByte
    
    ; Send acknowledgment
    BANKSEL PORTB
    BSF     PORTB, ACK_PIN    
    
    ; Wait for master to release INT (go low)
WAIT_INT_LOW_C:
    BTFSC   PORTB, INT_PIN
    GOTO    WAIT_INT_LOW_C
    
    ; Complete handshake
    BCF     PORTB, ACK_PIN     ; ACK low (ready for next byte)
    
    RETURN

;=============================================================
; SEND_DATA_BLOCK 
;=============================================================
SEND_DATA_BLOCK:
    ; Prepare PORTC for output
    BANKSEL TRISC
    CLRF    TRISC              ; PORTC = output
    BANKSEL PORTC
    CLRF    PORTC           

    ; Signal ready to send
    BANKSEL PORTB
    BCF     PORTB, ACK_PIN     ; ACK low initially

    ; Set up for 12-byte transfer using byteCount
    MOVLW   D'12'
    MOVWF   byteCount         

    MOVLW   LOW ascii_res
    MOVWF   FSR

SD_LOOP:
    ; Load next byte and send
    MOVF    INDF, W
    MOVWF   currentByte
    CALL    SEND_BYTE

    INCF    FSR, F

    DECFSZ  byteCount, F
    GOTO    SD_LOOP

    ; Cleanup after sending
    BANKSEL PORTC
    CLRF    PORTC             
    BANKSEL TRISC
    MOVLW   0xFF
    MOVWF   TRISC              ; PORTC = input 

    BANKSEL PORTB
    BCF     PORTB, ACK_PIN     ; Final ACK low
    RETURN


; ===============================================
; SEND_BYTE 
; ===============================================
SEND_BYTE:
    ; Wait for master to request data (INT high)
WAIT_INT_HIGH_S:
    BTFSS   PORTB, INT_PIN
    GOTO    WAIT_INT_HIGH_S
    
    ; Put data on bus
    BANKSEL TRISC
    CLRF    TRISC              ; Ensure PORTC is output
    BANKSEL PORTC
    MOVF    currentByte, W
    MOVWF   PORTC              ; Data valid on bus
    
    CALL    DELAY_100US
    
    ; Signal data ready
    BANKSEL PORTB
    BSF     PORTB, ACK_PIN     ; ACK high (data ready)
    
    ; Wait for master to finish reading (INT low)
WAIT_INT_LOW_S:
    BTFSC   PORTB, INT_PIN
    GOTO    WAIT_INT_LOW_S
    
    ; Complete handshake
    BCF     PORTB, ACK_PIN     ; ACK low
    
    RETURN

; =============================================================
; DIVIDE AND PREPARE RESPONSE
; =============================================================
DIVIDE_AND_PREPARE_RESPONSE:
    BCF     STATUS, RP1
    BCF     STATUS, RP0
    
    ; Receive Numerator 
    MOVLW   LOW receiveBuffer
    MOVWF   srcPtr
    MOVLW   LOW ascii_buf
    MOVWF   dstPtr
    MOVLW   12
    MOVWF   BCount
    CALL    copy_loop

    ; Convert ascii_buf to 48-bit binary numerator
    CALL    convert_ascii_to_hex
    
    ; Copy BIN to nrator registers
    BCF     STATUS, RP1
    BCF     STATUS, RP0
    BCF     STATUS, IRP
    MOVF    BIN0,W
    MOVWF   nratorL
    MOVF    BIN1,W
    MOVWF   nratorM1
    MOVF    BIN2,W
    MOVWF   nratorM2
    MOVF    BIN3,W
    MOVWF   nratorM3
    MOVF    BIN4,W
    MOVWF   nratorM4
    MOVF    BIN5,W
    MOVWF   nratorH

    ;Receive Denominator 
    MOVLW   LOW receiveBuffer
    ADDLW   .12
    MOVWF   srcPtr
    MOVLW   LOW ascii_buf
    MOVWF   dstPtr
    MOVLW   12
    MOVWF   BCount
    CALL    copy_loop

    ; Convert ascii_buf to 48-bit binary denominator
    CALL    convert_ascii_to_hex
    
    ; Copy BIN to denom registers
    BCF     STATUS, RP1
    BCF     STATUS, RP0
    BCF     STATUS, IRP
    MOVF    BIN0, W
    MOVWF   denomL
    MOVF    BIN1, W
    MOVWF   denomM1
    MOVF    BIN2, W
    MOVWF   denomM2
    MOVF    BIN3, W
    MOVWF   denomM3
    MOVF    BIN4, W
    MOVWF   denomM4
    MOVF    BIN5, W
    MOVWF   denomH

    ; Prepare for division: copy numerator to BIN and scale by 10^4
    MOVF    nratorL, W
    MOVWF   BIN0
    MOVF    nratorM1, W
    MOVWF   BIN1
    MOVF    nratorM2, W
    MOVWF   BIN2
    MOVF    nratorM3, W
    MOVWF   BIN3
    MOVF    nratorM4, W
    MOVWF   BIN4
    MOVF    nratorH,  W
    MOVWF   BIN5

    ; Scale by 10^4 for fixed-point precision
    CALL mul48u_by10  
    CALL mul48u_by10   
    ;CALL mul48u_by10   
    ;CALL mul48u_by10

    ; Copy scaled dividend to TMP1x registers
    MOVF    BIN0, W
    MOVWF   TMP1xL
    MOVF    BIN1, W
    MOVWF   TMP1xM1
    MOVF    BIN2, W
    MOVWF   TMP1xM2
    MOVF    BIN3, W
    MOVWF   TMP1xM3
    MOVF    BIN4, W
    MOVWF   TMP1xM4
    MOVF    BIN5, W
    MOVWF   TMP1xH  

    ; Perform division
    CALL    DIV_BY_SUBTRACT_ALT
    CALL    convert_hex_to_ascii
    CALL    prepare_ascii_result

    RETURN

;——————————————————————————————————————
; BIN5:BIN0 × 10
;——————————————————————————————————————
mul48u_by10:
    BCF     STATUS, RP1
    BCF     STATUS, RP0
    BCF     STATUS, IRP

    BCF     STATUS, C
    RLF     BIN0, F
    RLF     BIN1, F
    RLF     BIN2, F
    RLF     BIN3, F
    RLF     BIN4, F
    RLF     BIN5, F

    MOVF    BIN0, W
    MOVWF   TMP0
    MOVF    BIN1, W
    MOVWF   TMP1
    MOVF    BIN2, W
    MOVWF   TMP2
    MOVF    BIN3, W
    MOVWF   TMP3
    MOVF    BIN4, W
    MOVWF   TMP4
    MOVF    BIN5, W
    MOVWF   TMP5

    BCF     STATUS, C
    RLF     TMP0, F
    RLF     TMP1, F
    RLF     TMP2, F
    RLF     TMP3, F
    RLF     TMP4, F
    RLF     TMP5, F

    BCF     STATUS, C
    RLF     TMP0, F
    RLF     TMP1, F
    RLF     TMP2, F
    RLF     TMP3, F
    RLF     TMP4, F
    RLF     TMP5, F

    MOVF    TMP0, W
    ADDWF   BIN0, F
    MOVF    TMP1, W
    BTFSC   STATUS, C
    ADDLW   1
    ADDWF   BIN1, F
    MOVF    TMP2, W
    BTFSC   STATUS, C
    ADDLW   1
    ADDWF   BIN2, F
    MOVF    TMP3, W
    BTFSC   STATUS, C
    ADDLW   1
    ADDWF   BIN3, F
    MOVF    TMP4, W
    BTFSC   STATUS, C
    ADDLW   1
    ADDWF   BIN4, F
    MOVF    TMP5, W
    BTFSC   STATUS, C
    ADDLW   1
    ADDWF   BIN5, F

    RETURN
    
;——————————————————————————————————————
; 48-bit subtraction 
;——————————————————————————————————————
DIV_BY_SUBTRACT_ALT:
    BCF     STATUS, RP1
    BCF     STATUS, RP0
    
    ; Clear quotient
    CLRF    BIN0
    CLRF    BIN1
    CLRF    BIN2
    CLRF    BIN3
    CLRF    BIN4
    CLRF    BIN5

DIV_SUB_LOOP_ALT:
    ; 48-bit subtraction using SUBLW 
    MOVF    denomL, W
    SUBWF   TMP1xL, F      
    
    MOVF    denomM1, W
    BTFSS   STATUS, C        ; If no borrow (C=1), skip increment
    INCF    denomM1, W       ; Add 1 for borrow
    SUBWF   TMP1xM1, F
    
    MOVF    denomM2, W
    BTFSS   STATUS, C
    INCF    denomM2, W
    SUBWF   TMP1xM2, F
    
    MOVF    denomM3, W
    BTFSS   STATUS, C
    INCF    denomM3, W
    SUBWF   TMP1xM3, F
    
    MOVF    denomM4, W
    BTFSS   STATUS, C
    INCF    denomM4, W
    SUBWF   TMP1xM4, F
    
    MOVF    denomH, W
    BTFSS   STATUS, C
    INCF    denomH, W
    SUBWF   TMP1xH, F

    ; Check for underflow (negative result)
    BTFSS   STATUS, C        ; If C=0, we had underflow
    GOTO    DIV_SUB_RESTORE  ; Restore and exit

    ; Increment quotient
    INCF    BIN0, F
    BTFSC   STATUS, Z
    INCF    BIN1, F
    BTFSC   STATUS, Z
    INCF    BIN2, F
    BTFSC   STATUS, Z
    INCF    BIN3, F
    BTFSC   STATUS, Z
    INCF    BIN4, F
    BTFSC   STATUS, Z
    INCF    BIN5, F

    GOTO    DIV_SUB_LOOP_ALT

DIV_SUB_RESTORE:
    ; Restore the last subtraction that caused underflow
    MOVF    denomL, W
    ADDWF   TMP1xL, F
    
    MOVF    denomM1, W
    BTFSC   STATUS, C
    INCF    denomM1, W
    ADDWF   TMP1xM1, F
    
    MOVF    denomM2, W
    BTFSC   STATUS, C
    INCF    denomM2, W
    ADDWF   TMP1xM2, F
    
    MOVF    denomM3, W
    BTFSC   STATUS, C
    INCF    denomM3, W
    ADDWF   TMP1xM3, F
    
    MOVF    denomM4, W
    BTFSC   STATUS, C
    INCF    denomM4, W
    ADDWF   TMP1xM4, F
    
    MOVF    denomH, W
    BTFSC   STATUS, C
    INCF    denomH, W
    ADDWF   TMP1xH, F

    RETURN
    
; ————————————————————————————————
; copy_loop: copy 12 bytes from receiveBuffer
; ————————————————————————————————
copy_loop:
cpy_lp:
    BCF     STATUS, IRP      
    MOVF    srcPtr, W
    MOVWF   FSR
    MOVF    INDF, W
    MOVWF   currentByte
    INCF    srcPtr, F

    BCF     STATUS, IRP       
    MOVF    dstPtr, W
    MOVWF   FSR
    MOVF    currentByte, W
    MOVWF   INDF
    INCF    dstPtr, F

    DECFSZ  BCount, F
    GOTO    cpy_lp
    RETURN
    
; ===========================
;  convert_ascii_to_hex
; ===========================
convert_ascii_to_hex:
    MOVF    STATUS, W
    MOVWF   STATUS_SAV
    BCF     STATUS, RP1
    BCF     STATUS, RP0
    BCF     STATUS, IRP
    CLRF    BIN0
    CLRF    BIN1
    CLRF    BIN2
    CLRF    BIN3
    CLRF    BIN4
    CLRF    BIN5
    MOVLW   LOW ascii_buf
    MOVWF   FSR
    MOVLW   D'12'
    MOVWF   idx
conv_loop:
    MOVF    INDF, W
    ANDLW   0x0F
    MOVWF   DIGIT
    CALL    mul48u_by10

	MOVF    DIGIT, W
    ADDWF   BIN0, F
    CLRW                    
    BTFSC   STATUS, C     
    INCF    W, F            
    ADDWF   BIN1, F        
    CLRW
    BTFSC   STATUS, C
    INCF    W, F
    ADDWF   BIN2, F
    CLRW
    BTFSC   STATUS, C
    INCF    W, F
    ADDWF   BIN3, F
    CLRW
    BTFSC   STATUS, C
    INCF    W, F
    ADDWF   BIN4, F
    CLRW
    BTFSC   STATUS, C
    INCF    W, F
    ADDWF   BIN5, F
    INCF    FSR, F
    DECFSZ  idx, F
    GOTO    conv_loop
    MOVF    STATUS_SAV, W
    MOVWF   STATUS
    RETURN
;=========================================================
; convert_hex_to_ascii
;=========================================================
convert_hex_to_ascii
    movf    STATUS, W       
    movwf   STATUS_SAV      
    bcf     STATUS, RP0    
    bcf     STATUS, RP1

    movlw   LOW ascii_buf   
    movwf   FSR            
    movlw   D'12'          
    movwf   tempCounter    
CHTA_clear_loop:
    movlw   '0'             
    movwf   INDF            
    incf    FSR, F          
    decfsz  tempCounter, F  
    goto    CHTA_clear_loop 

    movlw   D'12'         
    movwf   idx             

    MOVLW   LOW ascii_buf   
    ADDLW   d'11'          
    movwf   ASCPTR          

digit_loop
    call    div48u_by10     
    addlw   '0'            
    movwf   DIGIT         

    movf    ASCPTR, W       
    movwf   FSR            
    movf    DIGIT, W        
    movwf   INDF            

    decf    ASCPTR, F       
    decf    idx, F         
    bnz     digit_loop      

    movf    STATUS_SAV, W  
    movwf   STATUS
    return

  
DELAY_200MS:
    MOVLW   D'200'
    MOVWF   tempCounter
DELAY_LOOP:
    MOVLW   D'250'
    MOVWF   currentByte
DELAY_INNER:
    NOP
    DECFSZ  currentByte, F
    GOTO    DELAY_INNER
    DECFSZ  tempCounter, F
    GOTO    DELAY_LOOP
    RETURN
DELAY_100US:
    MOVLW D'33'
    MOVWF tempCounter
LOOP100:
    NOP
    NOP
    DECFSZ tempCounter, F
    GOTO LOOP100
    RETURN

;=====================================
; div16u_by10
;=====================================
div16u_by10:
    clrf    QUOTB        
div16_check_loop:
    movf    BIGHI, W
    btfss   STATUS, Z       
    goto    div16_perform_sub 
    movlw   D'10'
    subwf   BIGHLO, W      
                            
    btfss   STATUS, C     
    goto    div16_done    

div16_perform_sub:
    movlw   D'10'
    subwf   BIGHLO, F    
    btfsc   STATUS, C      
    goto    div16_no_borrow_needed

    decf    BIGHI, F     

div16_no_borrow_needed:
    incf    QUOTB, F       
    goto    div16_check_loop 

div16_done:
    movf    BIGHLO, W
    return

;=======================
; div48u_by10
;=======================
div48u_by10
    clrf    REM
    movlw   LOW BIN5
    movwf   PTR
    movlw   D'6'
    movwf   BYTECTR

div48_loop
    movf    PTR, W
    movwf   FSR

    movf    INDF, W
    movwf   BIGHLO
    movf    REM, W
    movwf   BIGHI

    call    div16u_by10     

    movwf   REM             
    movf    QUOTB, W      
    movwf   INDF           

    decf    PTR, F
    decfsz  BYTECTR, F
    goto    div48_loop

    movf    REM, W         
    return

;==============================
; prepare_ascii_result
;==============================
prepare_ascii_result:
    bcf     STATUS, RP0
    bcf     STATUS, RP1

    movlw   LOW ascii_res     
    movwf   FSR                
    movlw   d'12'             
    movwf   tempCounter

init_loop:
    movlw   '0'
    movwf   INDF              
    incf    FSR, f            
    decfsz  tempCounter, f
    goto    init_loop

    movlw   LOW ascii_buf + 4 
    movwf   srcPtr
    movlw   LOW ascii_res    
    movwf   dstPtr
    movlw   d'8'             
    movwf   tempCounter

ccopy_loop:
    movf    srcPtr, W
    movwf   FSR             
    movf    INDF, W
    movwf   temp               

    movf    dstPtr, W
    movwf   FSR                
    movf    temp, W
    movwf   INDF              

    incf    srcPtr, f
    incf    dstPtr, f
    decfsz  tempCounter, f
    goto    ccopy_loop

    return

END