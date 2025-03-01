//============================================================================
// Product: Blinky on MSP-EXP430G2, cooperative QV kernel
// Last updated for version 7.3.0
// Last updated on  2023-05-23
//
//                   Q u a n t u m  L e a P s
//                   ------------------------
//                   Modern Embedded Software
//
// Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
//
// This program is open source software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Alternatively, this program may be distributed and modified under the
// terms of Quantum Leaps commercial licenses, which expressly supersede
// the GNU General Public License and are specifically designed for
// licensees interested in retaining the proprietary status of their code.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <www.gnu.org/licenses/>.
//
// Contact information:
// <www.state-machine.com/licensing>
// <info@state-machine.com>
//============================================================================
#include "qpc.h"
#include "blinky.h"
#include "bsp.h"

#include <msp430g2553.h>  // MSP430 variant used
// add other drivers if necessary...

//Q_DEFINE_THIS_FILE

// Local-scope objects -----------------------------------------------------
// 1MHz clock setting, see BSP_init()
#define BSP_MCK     1000000U
#define BSP_SMCLK   1000000U

#define LED1        (1U << 0)
#define LED2        (1U << 6)

// ISRs used in this project ===============================================
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
    __interrupt void TIMER0_A0_ISR(void); // prototype
    #pragma vector=TIMER0_A0_VECTOR
    __interrupt void TIMER0_A0_ISR(void)
#elif defined(__GNUC__)
    void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) TIMER0_A0_ISR (void)
#else
    #error Compiler not supported!
#endif
{
    TACTL &= ~TAIFG;   // clear the interrupt pending flag
#ifdef NDEBUG
    __low_power_mode_off_on_exit(); // see NOTE1
#endif

    QTIMEEVT_TICK_X(0U, 0); // time events for rate 0
}

// BSP functions ===========================================================
void BSP_init(void) {
    WDTCTL = WDTPW | WDTHOLD; // stop watchdog timer

    // configure the Basic Clock Module
    DCOCTL = 0;             // Select lowest DCOx and MODx settings
    BCSCTL1 = CALBC1_1MHZ;  // Set DCO
    DCOCTL = CALDCO_1MHZ;

    P1DIR |= (LED1 | LED2);  // set LED1 and LED2 pins to output
}
//............................................................................
void BSP_ledOff(void) {
    P1OUT &= ~LED1;        // turn LED1 off
}
//............................................................................
void BSP_ledOn(void) {
    P1OUT |= LED1;         // turn LED1 on
}

// QF callbacks ============================================================
void QF_onStartup(void) {
    TACTL  = (ID_3 | TASSEL_2 | MC_1);  // SMCLK, /8 divider, upmode
    TACCR0 = (((BSP_SMCLK / 8U) + BSP_TICKS_PER_SEC/2U) / BSP_TICKS_PER_SEC);
    CCTL0 = CCIE;  // CCR0 interrupt enabled
}
//............................................................................
void QV_onIdle(void) { // NOTE: called with interrutps DISABLED, see NOTE1
    // toggle LED2 on and then off, see NOTE2
    P1OUT |=  LED2;        // turn LED2 on
    P1OUT &= ~LED2;        // turn LED2 off

#ifdef NDEBUG
    // Put the CPU and peripherals to the low-power mode.
    // you might need to customize the clock management for your application,
    // see the datasheet for your particular MSP430 MCU.
    //
    __low_power_mode_1(); // Enter LPM1; also ENABLES interrupts
#else
    QF_INT_ENABLE(); // just enable interrupts
#endif
}
//............................................................................
Q_NORETURN Q_onError(char const * const module, int_t const id) {
    // implement the error-handling policy for your application!!!
    Q_UNUSED_PAR(module);
    Q_UNUSED_PAR(id);

    // cause the reset of the CPU...
    WDTCTL = WDTPW | WDTHOLD;
    __asm("    push &0xFFFE");
    for (;;) {} // explicitly no-return
}
//............................................................................
void assert_failed(char const * const module, int_t const id); // prototype
void assert_failed(char const * const module, int_t const id) {
    Q_onError(module, id);
}

//============================================================================
// NOTE1:
// With the cooperative QV kernel for MSP430, it is necessary to explicitly
// turn the low-power mode OFF in the interrupt, because the return
// from the interrupt will restore the CPU status register, which will
// re-enter the low-power mode. This, in turn, will prevent the QV event-loop
// from running.
//
// NOTE2:
// One of the LEDs is used to visualize the idle loop activity. The brightness
// of the LED is proportional to the frequency of invocations of the idle loop.
// Please note that the LED is toggled with interrupts locked, so no interrupt
// execution time contributes to the brightness of the User LED.
//

