//============================================================================
// Product: "Blinky" on LAUCHXL2-TMS570LS12 board, preemptive QK kernel
// Last Updated for Version: 5.8.1
// Date of the Last Update:  2016-12-14
//
//                   Q u a n t u m     L e a P s
//                   ---------------------------
//                   innovating embedded systems
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

#include "sys_common.h"
#include "sys_core.h"
#include "sys_vim.h"
#include "system.h"
#include "gio.h"
#include "rti.h"
#include "het.h"
// add other drivers if necessary...

//Q_DEFINE_THIS_FILE

#ifdef Q_SPY
    #error Simple Blinky Application does not provide Spy build configuration
#endif

// ISRs defined in this BSP ------------------------------------------------

// Local-scope objects -----------------------------------------------------
#define LED2_PIN    1
#define LED2_PORT   gioPORTB

#define LED3_PIN    2
#define LED3_PORT   gioPORTB

// NOTE: Switch-A is multiplexed on the same port/pin as LED3,
// so you can use one or the other but not both simultaneously.
//
#define SWA_PIN     2
#define SWA_PORT    gioPORTB

#define SWB_PIN     15
#define SWB_PORT    hetREG1

#define VIM_RAM     ((t_isrFuncPTR *)0xFFF82000U)

// ISRs used in this project ===============================================
QK_IRQ_BEGIN(rtiCompare0)
    rtiREG1->INTFLAG = 1U;    // clear the interrutp source
    QTIMEEVT_TICK_X(0U, (void *)0); // process time events for rate 0
QK_IRQ_END()

// BSP functions ===========================================================
void BSP_init(void) {
    // configure the LEDs
    gioInit();
    LED2_PORT->DIR |= (1U << LED2_PIN);  // set as output
    LED3_PORT->DIR |= (1U << LED3_PIN);  // set as output

    // configure the Buttons
    SWB_PORT->DIR  &= (1U << SWB_PIN);    // set as input
}

//............................................................................
void BSP_ledOff(void) {
    LED2_PORT->DCLR = (1U << LED2_PIN);
}
//............................................................................
void BSP_ledOn(void) {
    // exercise the FPU with some floating point computations
    float volatile x = 3.1415926F;
    x = x + 2.7182818F;

    LED2_PORT->DSET = (1U << LED2_PIN);
}


// QF callbacks ============================================================
void QF_onStartup(void) {
    rtiInit(); // configure RTI with UC counter of 7
    rtiSetPeriod(rtiCOUNTER_BLOCK0,
                 (uint32)((RTI_FREQ*1E6/(7+1))/BSP_TICKS_PER_SEC));
    rtiEnableNotification(rtiNOTIFICATION_COMPARE0);
    rtiStartCounter(rtiCOUNTER_BLOCK0);

    VIM_RAM[2 + 1] = (t_isrFuncPTR)&rtiCompare0; // install the IRQ
    vimREG->FIRQPR0 &= ~(1U << 2);   // designate interrupt as IRQ, NOTE00
    vimREG->REQMASKSET0 = (1U << 2); // enable RTI interrupt

    QF_INT_ENABLE_ALL(); // enable all interrupts (IRQ and FIQ)
}
//............................................................................
void QF_onCleanup(void) {
}
//............................................................................
void QK_onIdle(void) {
    // toggle LED1 on and then off, see NOTE01
    QF_INT_DISABLE();
    LED3_PORT->DSET = (1U << LED3_PIN);
    LED3_PORT->DCLR = (1U << LED3_PIN);
    QF_INT_ENABLE();

#ifdef NDEBUG
    // Put the CPU and peripherals to the low-power mode.
    // you might need to customize the clock management for your application,
    // see the datasheet for your particular Cortex-R MCU.
    //
    _gotoCPUIdle_(); // wait for interrupt
#endif
}

//............................................................................
Q_NORETURN Q_onError(char const * const module, int_t const id) {
    //
    // NOTE: add here your application-specific error handling
    //
    Q_UNUSED_PAR(module);
    Q_UNUSED_PAR(id);

    QS_ASSERTION(module, id, 10000U); // report assertion to QS
    systemREG1->SYSECR = 0; // perform system reset
    for (;;) { // explicitly no-return
    }
}
//............................................................................
void assert_failed(char const * const module, int_t const id); // prototype
void assert_failed(char const * const module, int_t const id) {
    Q_onError(module, id);
}


//============================================================================
// NOTE00:
// The FIQ-type interrupts are never disabled in this QP port, therefore
// they can always preempt any code, including the IRQ-handlers (ISRs).
// Therefore, FIQ-type interrupts are "kernel-unaware" and must NEVER call
// any QP services, such as posting events.
//
// NOTE01:
// One of the LEDs is used to visualize the idle loop activity. The brightness
// of the LED is proportional to the frequency of invcations of the idle loop.
// Please note that the LED is toggled with interrupts locked, so no interrupt
// execution time contributes to the brightness of the User LED.
//
