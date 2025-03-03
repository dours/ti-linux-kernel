/*
 * DaVinci interrupt controller definitions
 *
 *  Copyright (C) 2006 Texas Instruments.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#ifndef __ASM_ARCH_IRQS_H
#define __ASM_ARCH_IRQS_H

/* Base address */
#define DAVINCI_ARM_INTC_BASE 0x01C48000

#define DAVINCI_INTC_TYPE_AINTC		0
#define DAVINCI_INTC_TYPE_CP_INTC	1

/* Interrupt lines */
#define IRQ_VDINT0       0
#define IRQ_VDINT1       1
#define IRQ_VDINT2       2
#define IRQ_HISTINT      3
#define IRQ_H3AINT       4
#define IRQ_PRVUINT      5
#define IRQ_RSZINT       6
#define IRQ_VFOCINT      7
#define IRQ_VENCINT      8
#define IRQ_ASQINT       9
#define IRQ_IMXINT       10
#define IRQ_VLCDINT      11
#define IRQ_USBINT       12
#define IRQ_EMACINT      13

#define IRQ_CCINT0       16
#define IRQ_CCERRINT     17
#define IRQ_TCERRINT0    18
#define IRQ_TCERRINT     19
#define IRQ_PSCIN        20

#define IRQ_IDE          22
#define IRQ_HPIINT       23
#define IRQ_MBXINT       24
#define IRQ_MBRINT       25
#define IRQ_MMCINT       26
#define IRQ_SDIOINT      27
#define IRQ_MSINT        28
#define IRQ_DDRINT       29
#define IRQ_AEMIFINT     30
#define IRQ_VLQINT       31
#define IRQ_TINT0_TINT12 32
#define IRQ_TINT0_TINT34 33
#define IRQ_TINT1_TINT12 34
#define IRQ_TINT1_TINT34 35
#define IRQ_PWMINT0      36
#define IRQ_PWMINT1      37
#define IRQ_PWMINT2      38
#define IRQ_I2C          39
#define IRQ_UARTINT0     40
#define IRQ_UARTINT1     41
#define IRQ_UARTINT2     42
#define IRQ_SPINT0       43
#define IRQ_SPINT1       44

#define IRQ_DSP2ARM0     46
#define IRQ_DSP2ARM1     47
#define IRQ_GPIO0        48
#define IRQ_GPIO1        49
#define IRQ_GPIO2        50
#define IRQ_GPIO3        51
#define IRQ_GPIO4        52
#define IRQ_GPIO5        53
#define IRQ_GPIO6        54
#define IRQ_GPIO7        55
#define IRQ_GPIOBNK0     56
#define IRQ_GPIOBNK1     57
#define IRQ_GPIOBNK2     58
#define IRQ_GPIOBNK3     59
#define IRQ_GPIOBNK4     60
#define IRQ_COMMTX       61
#define IRQ_COMMRX       62
#define IRQ_EMUINT       63

#define DAVINCI_N_AINTC_IRQ	64

#define ARCH_TIMER_IRQ IRQ_TINT1_TINT34

/* DaVinci DM6467-specific Interrupts */
#define IRQ_DM646X_VP_VERTINT0  0
#define IRQ_DM646X_VP_VERTINT1  1
#define IRQ_DM646X_VP_VERTINT2  2
#define IRQ_DM646X_VP_VERTINT3  3
#define IRQ_DM646X_VP_ERRINT    4
#define IRQ_DM646X_RESERVED_1   5
#define IRQ_DM646X_RESERVED_2   6
#define IRQ_DM646X_WDINT        7
#define IRQ_DM646X_CRGENINT0    8
#define IRQ_DM646X_CRGENINT1    9
#define IRQ_DM646X_TSIFINT0     10
#define IRQ_DM646X_TSIFINT1     11
#define IRQ_DM646X_VDCEINT      12
#define IRQ_DM646X_USBINT       13
#define IRQ_DM646X_USBDMAINT    14
#define IRQ_DM646X_PCIINT       15
#define IRQ_DM646X_TCERRINT2    20
#define IRQ_DM646X_TCERRINT3    21
#define IRQ_DM646X_IDE          22
#define IRQ_DM646X_HPIINT       23
#define IRQ_DM646X_EMACRXTHINT  24
#define IRQ_DM646X_EMACRXINT    25
#define IRQ_DM646X_EMACTXINT    26
#define IRQ_DM646X_EMACMISCINT  27
#define IRQ_DM646X_MCASP0TXINT  28
#define IRQ_DM646X_MCASP0RXINT  29
#define IRQ_DM646X_MCASP1TXINT  30
#define IRQ_DM646X_RESERVED_3   31
#define IRQ_DM646X_VLQINT       38
#define IRQ_DM646X_UARTINT2     42
#define IRQ_DM646X_SPINT0       43
#define IRQ_DM646X_SPINT1       44
#define IRQ_DM646X_DSP2ARMINT   45
#define IRQ_DM646X_RESERVED_4   46
#define IRQ_DM646X_PSCINT       47
#define IRQ_DM646X_GPIO0        48
#define IRQ_DM646X_GPIO1        49
#define IRQ_DM646X_GPIO2        50
#define IRQ_DM646X_GPIO3        51
#define IRQ_DM646X_GPIO4        52
#define IRQ_DM646X_GPIO5        53
#define IRQ_DM646X_GPIO6        54
#define IRQ_DM646X_GPIO7        55
#define IRQ_DM646X_GPIOBNK0     56
#define IRQ_DM646X_GPIOBNK1     57
#define IRQ_DM646X_GPIOBNK2     58
#define IRQ_DM646X_DDRINT       59
#define IRQ_DM646X_AEMIFINT     60

/* DaVinci DM355-specific Interrupts */
#define IRQ_DM355_CCDC_VDINT0	0
#define IRQ_DM355_CCDC_VDINT1	1
#define IRQ_DM355_CCDC_VDINT2	2
#define IRQ_DM355_IPIPE_HST	3
#define IRQ_DM355_H3AINT	4
#define IRQ_DM355_IPIPE_SDR	5
#define IRQ_DM355_IPIPEIFINT	6
#define IRQ_DM355_OSDINT	7
#define IRQ_DM355_VENCINT	8
#define IRQ_DM355_IMCOPINT	11
#define IRQ_DM355_RTOINT	13
#define IRQ_DM355_TINT4		13
#define IRQ_DM355_TINT2_TINT12	13
#define IRQ_DM355_UARTINT2	14
#define IRQ_DM355_TINT5		14
#define IRQ_DM355_TINT2_TINT34	14
#define IRQ_DM355_TINT6		15
#define IRQ_DM355_TINT3_TINT12	15
#define IRQ_DM355_SPINT1_0	17
#define IRQ_DM355_SPINT1_1	18
#define IRQ_DM355_SPINT2_0	19
#define IRQ_DM355_SPINT2_1	21
#define IRQ_DM355_TINT7		22
#define IRQ_DM355_TINT3_TINT34	22
#define IRQ_DM355_SDIOINT0	23
#define IRQ_DM355_MMCINT0	26
#define IRQ_DM355_MSINT		26
#define IRQ_DM355_MMCINT1	27
#define IRQ_DM355_PWMINT3	28
#define IRQ_DM355_SDIOINT1	31
#define IRQ_DM355_SPINT0_0	42
#define IRQ_DM355_SPINT0_1	43
#define IRQ_DM355_GPIO0		44
#define IRQ_DM355_GPIO1		45
#define IRQ_DM355_GPIO2		46
#define IRQ_DM355_GPIO3		47
#define IRQ_DM355_GPIO4		48
#define IRQ_DM355_GPIO5		49
#define IRQ_DM355_GPIO6		50
#define IRQ_DM355_GPIO7		51
#define IRQ_DM355_GPIO8		52
#define IRQ_DM355_GPIO9		53
#define IRQ_DM355_GPIOBNK0	54
#define IRQ_DM355_GPIOBNK1	55
#define IRQ_DM355_GPIOBNK2	56
#define IRQ_DM355_GPIOBNK3	57
#define IRQ_DM355_GPIOBNK4	58
#define IRQ_DM355_GPIOBNK5	59
#define IRQ_DM355_GPIOBNK6	60

/* DaVinci DM365-specific Interrupts */
#define IRQ_DM365_INSFINT	7
#define IRQ_DM365_IMXINT1	8
#define IRQ_DM365_IMXINT0	10
#define IRQ_DM365_KLD_ARMINT	10
#define IRQ_DM365_IMCOPINT	11
#define IRQ_DM365_RTOINT	13
#define IRQ_DM365_TINT5		14
#define IRQ_DM365_TINT6		15
#define IRQ_DM365_SPINT2_1	21
#define IRQ_DM365_TINT7		22
#define IRQ_DM365_SDIOINT0	23
#define IRQ_DM365_MMCINT1	27
#define IRQ_DM365_PWMINT3	28
#define IRQ_DM365_RTCINT	29
#define IRQ_DM365_SDIOINT1	31
#define IRQ_DM365_SPIINT0_0	42
#define IRQ_DM365_SPIINT3_0	43
#define IRQ_DM365_GPIO0		44
#define IRQ_DM365_GPIO1		45
#define IRQ_DM365_GPIO2		46
#define IRQ_DM365_GPIO3		47
#define IRQ_DM365_GPIO4		48
#define IRQ_DM365_GPIO5		49
#define IRQ_DM365_GPIO6		50
#define IRQ_DM365_GPIO7		51
#define IRQ_DM365_EMAC_RXTHRESH	52
#define IRQ_DM365_EMAC_RXPULSE	53
#define IRQ_DM365_EMAC_TXPULSE	54
#define IRQ_DM365_EMAC_MISCPULSE 55
#define IRQ_DM365_GPIO12	56
#define IRQ_DM365_GPIO13	57
#define IRQ_DM365_GPIO14	58
#define IRQ_DM365_GPIO15	59
#define IRQ_DM365_ADCINT	59
#define IRQ_DM365_KEYINT	60
#define IRQ_DM365_TCERRINT2	61
#define IRQ_DM365_TCERRINT3	62
#define IRQ_DM365_EMUINT	63

/* DA8XX interrupts */
#define IRQ_DA8XX_COMMTX		0
#define IRQ_DA8XX_COMMRX		1
#define IRQ_DA8XX_NINT			2
#define IRQ_DA8XX_EVTOUT0		3
#define IRQ_DA8XX_EVTOUT1		4
#define IRQ_DA8XX_EVTOUT2		5
#define IRQ_DA8XX_EVTOUT3		6
#define IRQ_DA8XX_EVTOUT4		7
#define IRQ_DA8XX_EVTOUT5		8
#define IRQ_DA8XX_EVTOUT6		9
#define IRQ_DA8XX_EVTOUT7		10
#define IRQ_DA8XX_CCINT0		11
#define IRQ_DA8XX_CCERRINT		12
#define IRQ_DA8XX_TCERRINT0		13
#define IRQ_DA8XX_AEMIFINT		14
#define IRQ_DA8XX_I2CINT0		15
#define IRQ_DA8XX_MMCSDINT0		16
#define IRQ_DA8XX_MMCSDINT1		17
#define IRQ_DA8XX_ALLINT0		18
#define IRQ_DA8XX_RTC			19
#define IRQ_DA8XX_SPINT0		20
#define IRQ_DA8XX_TINT12_0		21
#define IRQ_DA8XX_TINT34_0		22
#define IRQ_DA8XX_TINT12_1		23
#define IRQ_DA8XX_TINT34_1		24
#define IRQ_DA8XX_UARTINT0		25
#define IRQ_DA8XX_KEYMGRINT		26
#define IRQ_DA8XX_SECINT		26
#define IRQ_DA8XX_SECKEYERR		26
#define IRQ_DA8XX_CHIPINT0		28
#define IRQ_DA8XX_CHIPINT1		29
#define IRQ_DA8XX_CHIPINT2		30
#define IRQ_DA8XX_CHIPINT3		31
#define IRQ_DA8XX_TCERRINT1		32
#define IRQ_DA8XX_C0_RX_THRESH_PULSE	33
#define IRQ_DA8XX_C0_RX_PULSE		34
#define IRQ_DA8XX_C0_TX_PULSE		35
#define IRQ_DA8XX_C0_MISC_PULSE		36
#define IRQ_DA8XX_C1_RX_THRESH_PULSE	37
#define IRQ_DA8XX_C1_RX_PULSE		38
#define IRQ_DA8XX_C1_TX_PULSE		39
#define IRQ_DA8XX_C1_MISC_PULSE		40
#define IRQ_DA8XX_MEMERR		41
#define IRQ_DA8XX_GPIO0			42
#define IRQ_DA8XX_GPIO1			43
#define IRQ_DA8XX_GPIO2			44
#define IRQ_DA8XX_GPIO3			45
#define IRQ_DA8XX_GPIO4			46
#define IRQ_DA8XX_GPIO5			47
#define IRQ_DA8XX_GPIO6			48
#define IRQ_DA8XX_GPIO7			49
#define IRQ_DA8XX_GPIO8			50
#define IRQ_DA8XX_I2CINT1		51
#define IRQ_DA8XX_LCDINT		52
#define IRQ_DA8XX_UARTINT1		53
#define IRQ_DA8XX_MCASPINT		54
#define IRQ_DA8XX_ALLINT1		55
#define IRQ_DA8XX_SPINT1		56
#define IRQ_DA8XX_UHPI_INT1		57
#define IRQ_DA8XX_USB_INT		58
#define IRQ_DA8XX_IRQN			59
#define IRQ_DA8XX_RWAKEUP		60
#define IRQ_DA8XX_UARTINT2		61
#define IRQ_DA8XX_DFTSSINT		62
#define IRQ_DA8XX_EHRPWM0		63
#define IRQ_DA8XX_EHRPWM0TZ		64
#define IRQ_DA8XX_EHRPWM1		65
#define IRQ_DA8XX_EHRPWM1TZ		66
#define IRQ_DA8XX_ECAP0			69
#define IRQ_DA8XX_ECAP1			70
#define IRQ_DA8XX_ECAP2			71
#define IRQ_DA8XX_ARMCLKSTOPREQ		90

/* DA830 specific interrupts */
#define IRQ_DA830_MPUERR		27
#define IRQ_DA830_IOPUERR		27
#define IRQ_DA830_BOOTCFGERR		27
#define IRQ_DA830_EHRPWM2		67
#define IRQ_DA830_EHRPWM2TZ		68
#define IRQ_DA830_EQEP0			72
#define IRQ_DA830_EQEP1			73
#define IRQ_DA830_T12CMPINT0_0		74
#define IRQ_DA830_T12CMPINT1_0		75
#define IRQ_DA830_T12CMPINT2_0		76
#define IRQ_DA830_T12CMPINT3_0		77
#define IRQ_DA830_T12CMPINT4_0		78
#define IRQ_DA830_T12CMPINT5_0		79
#define IRQ_DA830_T12CMPINT6_0		80
#define IRQ_DA830_T12CMPINT7_0		81
#define IRQ_DA830_T12CMPINT0_1		82
#define IRQ_DA830_T12CMPINT1_1		83
#define IRQ_DA830_T12CMPINT2_1		84
#define IRQ_DA830_T12CMPINT3_1		85
#define IRQ_DA830_T12CMPINT4_1		86
#define IRQ_DA830_T12CMPINT5_1		87
#define IRQ_DA830_T12CMPINT6_1		88
#define IRQ_DA830_T12CMPINT7_1		89

#define DA830_N_CP_INTC_IRQ		96

/* DA850 speicific interrupts */
#define IRQ_DA850_MPUADDRERR0		27
#define IRQ_DA850_MPUPROTERR0		27
#define IRQ_DA850_IOPUADDRERR0		27
#define IRQ_DA850_IOPUPROTERR0		27
#define IRQ_DA850_IOPUADDRERR1		27
#define IRQ_DA850_IOPUPROTERR1		27
#define IRQ_DA850_IOPUADDRERR2		27
#define IRQ_DA850_IOPUPROTERR2		27
#define IRQ_DA850_BOOTCFG_ADDR_ERR	27
#define IRQ_DA850_BOOTCFG_PROT_ERR	27
#define IRQ_DA850_MPUADDRERR1		27
#define IRQ_DA850_MPUPROTERR1		27
#define IRQ_DA850_IOPUADDRERR3		27
#define IRQ_DA850_IOPUPROTERR3		27
#define IRQ_DA850_IOPUADDRERR4		27
#define IRQ_DA850_IOPUPROTERR4		27
#define IRQ_DA850_IOPUADDRERR5		27
#define IRQ_DA850_IOPUPROTERR5		27
#define IRQ_DA850_MIOPU_BOOTCFG_ERR	27
#define IRQ_DA850_SATAINT		67
#define IRQ_DA850_TINT12_2		68
#define IRQ_DA850_TINT34_2		68
#define IRQ_DA850_TINTALL_2		68
#define IRQ_DA850_MMCSDINT0_1		72
#define IRQ_DA850_MMCSDINT1_1		73
#define IRQ_DA850_T12CMPINT0_2		74
#define IRQ_DA850_T12CMPINT1_2		75
#define IRQ_DA850_T12CMPINT2_2		76
#define IRQ_DA850_T12CMPINT3_2		77
#define IRQ_DA850_T12CMPINT4_2		78
#define IRQ_DA850_T12CMPINT5_2		79
#define IRQ_DA850_T12CMPINT6_2		80
#define IRQ_DA850_T12CMPINT7_2		81
#define IRQ_DA850_T12CMPINT0_3		82
#define IRQ_DA850_T12CMPINT1_3		83
#define IRQ_DA850_T12CMPINT2_3		84
#define IRQ_DA850_T12CMPINT3_3		85
#define IRQ_DA850_T12CMPINT4_3		86
#define IRQ_DA850_T12CMPINT5_3		87
#define IRQ_DA850_T12CMPINT6_3		88
#define IRQ_DA850_T12CMPINT7_3		89
#define IRQ_DA850_RPIINT		91
#define IRQ_DA850_VPIFINT		92
#define IRQ_DA850_CCINT1		93
#define IRQ_DA850_CCERRINT1		94
#define IRQ_DA850_TCERRINT2		95
#define IRQ_DA850_TINT12_3		96
#define IRQ_DA850_TINT34_3		96
#define IRQ_DA850_TINTALL_3		96
#define IRQ_DA850_MCBSP0RINT		97
#define IRQ_DA850_MCBSP0XINT		98
#define IRQ_DA850_MCBSP1RINT		99
#define IRQ_DA850_MCBSP1XINT		100

#define DA850_N_CP_INTC_IRQ		101

/* da850 currently has the most gpio pins (144) */
#define DAVINCI_N_GPIO			144
/* da850 currently has the most irqs so use DA850_N_CP_INTC_IRQ */
/* Add 10 more irqs for virtual irq management */
#define NR_IRQS				(DA850_N_CP_INTC_IRQ + DAVINCI_N_GPIO + 10)

#endif /* __ASM_ARCH_IRQS_H */
