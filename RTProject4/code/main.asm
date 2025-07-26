;===============================
; Interactive LCD Digit Entry with Lock
;===============================

    LIST P=16F877A
    #include <P16F877A.INC>
    __CONFIG 0x3731  ; HS oscillator, WDT off, PWRTE on, LVP off

;===============================
; Variables
;===============================
    cblock 0x20
        BlinkCount
        DigitValue
        InactivityCounter
        CurrentPos        ; Holds LCD DDRAM address (0xC0, 0xC2, 0xC4, ...)
    endc

;===============================
; Reset Vector
;===============================
    ORG 0x00
    GOTO START

;===============================
; Main Program
;===============================
START:
    ; Set RD as output
    BANKSEL TRISD
    CLRF TRISD

    ; Set RB0 as input (button)
    BANKSEL TRISB
    BSF TRISB, 0

    ; Clear ports
    BANKSEL PORTD
    CLRF PORTD
    BANKSEL PORTB
    CLRF PORTB

    ; Init LCD
    CALL inid

;===============================
; Blink "Welcome to / Division" 3x
;===============================
    MOVLW 3
    MOVWF BlinkCount

welcome_loop:
    CALL display_welcome
    CALL half_second_delay
    CALL clear_display
    CALL half_second_delay
    DECFSZ BlinkCount, F
    GOTO welcome_loop

    CALL display_welcome
    CALL two_second_delay

;===============================
; Show "Number 1"
;===============================
    CALL clear_display
    BCF Select, RS
    MOVLW 0x80
    CALL send

    MOVLW 'N'
    CALL display_char
    MOVLW 'u'
    CALL display_char
    MOVLW 'm'
    CALL display_char
    MOVLW 'b'
    CALL display_char
    MOVLW 'e'
    CALL display_char
    MOVLW 'r'
    CALL display_char
    MOVLW ' '
    CALL display_char
    MOVLW '1'
    CALL display_char

    ; Move to second row
    BCF Select, RS
    MOVLW 0xC0
    CALL send

    ; Enable cursor blinking
    MOVLW 0x0F
    CALL send

    ; Init
    CLRF DigitValue
    CLRF InactivityCounter
    MOVLW 0xC0
    MOVWF CurrentPos

;===============================
; Digit Entry (4 digits max)
;===============================
NextDigit:
    CLRF InactivityCounter

DigitInputLoop:
    ; Set cursor
    MOVF CurrentPos, W
    CALL set_cursor

    ; Show digit
    MOVF DigitValue, W
    ADDLW '0'
    CALL display_char

    CALL quarter_second_delay

    ; Check RB0 press
    BTFSS PORTB, 0
    GOTO button_pressed

    ; Not pressed
    INCF InactivityCounter, F
    MOVF InactivityCounter, W
    SUBLW D'4'
    BTFSS STATUS, Z
    GOTO DigitInputLoop

    ; Lock digit
    MOVLW 0x0C
    CALL send

    ; Move to next position (C0, C2, C4, C6)
    INCF CurrentPos, F
    INCF CurrentPos, F

    ; Stop after 4 digits
    MOVLW 0xC8        ; max pos = 0xC6
    SUBWF CurrentPos, W
    BTFSS STATUS, C
    GOTO NextDigit

    GOTO IDLE

button_pressed:
    CLRF InactivityCounter
    INCF DigitValue, F
    MOVF DigitValue, W
    SUBLW D'10'
    BTFSS STATUS, C
    CLRF DigitValue
    GOTO DigitInputLoop

;===============================
; set_cursor: Set LCD cursor
;===============================
set_cursor:
    BCF Select, RS
    CALL send
    RETURN

;===============================
; display_welcome
;===============================
display_welcome:
    BCF Select, RS
    MOVLW 0x80
    CALL send
    MOVLW 'W'
    CALL display_char
    MOVLW 'e'
    CALL display_char
    MOVLW 'l'
    CALL display_char
    MOVLW 'c'
    CALL display_char
    MOVLW 'o'
    CALL display_char
    MOVLW 'm'
    CALL display_char
    MOVLW 'e'
    CALL display_char
    MOVLW ' '
    CALL display_char
    MOVLW 't'
    CALL display_char
    MOVLW 'o'
    CALL display_char

    BCF Select, RS
    MOVLW 0xC0
    CALL send
    MOVLW 'D'
    CALL display_char
    MOVLW 'i'
    CALL display_char
    MOVLW 'v'
    CALL display_char
    MOVLW 'i'
    CALL display_char
    MOVLW 's'
    CALL display_char
    MOVLW 'i'
    CALL display_char
    MOVLW 'o'
    CALL display_char
    MOVLW 'n'
    CALL display_char
    RETURN

;===============================
; display_char: WREG ? LCD
;===============================
display_char:
    BSF Select, RS
    CALL send
    RETURN

;===============================
; clear_display
;===============================
clear_display:
    BCF Select, RS
    MOVLW 0x01
    CALL send
    MOVLW D'5'
    CALL xms
    RETURN

;===============================
; Delays
;===============================
quarter_second_delay:
    MOVLW D'62'
    CALL xms
    MOVLW D'63'
    CALL xms
    MOVLW D'63'
    CALL xms
    MOVLW D'62'
    CALL xms
    RETURN

half_second_delay:
    CALL quarter_second_delay
    CALL quarter_second_delay
    RETURN

two_second_delay:
    CALL half_second_delay
    CALL half_second_delay
    CALL half_second_delay
    CALL half_second_delay
    RETURN

;===============================
; IDLE (End)
;===============================
IDLE:
    GOTO IDLE

;===============================
; Include LCD Driver
;===============================
    INCLUDE "LCDIS.INC"

    END
