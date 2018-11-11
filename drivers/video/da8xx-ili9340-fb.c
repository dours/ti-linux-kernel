#define DEBUG
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fb.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/ioport.h>
#include <linux/dma-mapping.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h> 
#include <linux/da8xx-ili9340-fb.h>

#define DRIVER_NAME "da8xx_lcdc"
#define NICE_NAME "DA8xx ILI9340"

#define __devinitexit


#if 1
#include <linux/gpio.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/system_info.h>

//#include <mach/cp_intc.h>
#include <mach/da8xx.h>
#include <mach/mux.h>
//#include <mach/aemif.h>
//#include <mach/spi.h>

#include <linux/platform_data/gpio-davinci.h>

/*
 * ILI9340-based LCD
 */


int display_orientation = 0; // '1' - landscape; '0' - portrait

EXPORT_SYMBOL (display_orientation);
static int  set_orientation(char *str)
{
	if (!strcasecmp(str,"landscape"))
		display_orientation = 1;
	else if (!strcasecmp(str,"portrait"))
		display_orientation = 0;
	else 
		return 1;
	return 0;
}


__setup("trik.display_orientation=", set_orientation);


#ifdef EXTRA_PINS
static const short da850_trik_lcd_extra_pins[]  = {
	DA850_GPIO6_12, // LCD backlight
	DA850_GPIO8_10, // LCD reset
	-1
};
#endif

static void da850_trik_lcd_backlight_ctrl(bool _backlight)
{
	gpio_set_value(GPIO_TO_PIN(6, 12), _backlight);
}

static void da850_trik_lcd_power_ctrl(bool _power_up)
{
	if (_power_up) {
		gpio_set_value(GPIO_TO_PIN(8, 10), 0);
		udelay(10);
		gpio_set_value(GPIO_TO_PIN(8, 10), 1);
	} else {
		gpio_set_value(GPIO_TO_PIN(8, 10), 0);
	}
}

#define DA8XX_LCD_CNTRL_BASE		0x01e13000
static struct resource da850_trik_lcdc_resources[] = {
	[0] = { /* registers */
		.start  = DA8XX_LCD_CNTRL_BASE,
		.end    = DA8XX_LCD_CNTRL_BASE + SZ_4K - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = { /* interrupt */
		.start  = IRQ_DA8XX_LCDINT,
		.end    = IRQ_DA8XX_LCDINT,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct da8xx_ili9340_pdata da850_trik_lcdc_pdata = {
	.visual_mode		= DA8XX_LCDC_VISUAL_565,
	.fps			= 20, //50ms delay between memory write and redrawing

	.lcdc_lidd_mode		= DA8XX_LCDC_LIDD_MODE_8080ASYNC,
	.lcdc_lidd_cs		= DA8XX_LCDC_LIDD_CS0,
	.lcdc_lidd_ale_pol	= DA8XX_LCDC_LIDD_POL_ACTIVE_LOW,
	.lcdc_lidd_rs_en_pol	= DA8XX_LCDC_LIDD_POL_ACTIVE_LOW,
	.lcdc_lidd_ws_dir_pol	= DA8XX_LCDC_LIDD_POL_ACTIVE_LOW,
	.lcdc_lidd_cs_pol	= DA8XX_LCDC_LIDD_POL_ACTIVE_LOW,
	.lcdc_lidd_edma_burst	= 16,

        // Data taken from ch 19.3.2
	.lcdc_lidd_mclk_ns	= 0,	// Although not actually exported, it is used as granularity for timings below
					// Run at LCD_CLK for best granularity
	.lcdc_t_ta_ns		= 0,	// Tchw=0
	.lcdc_t_rhold_ns	= 90,	// Taht=10, Trdh=90/90
	.lcdc_t_rstrobe_ns	= 355,	// Trdl=45/355, Trcs=45/355
	.lcdc_t_rsu_ns		= 0,	// Tast=0, no CS->RD_low timeout
	.lcdc_t_whold_ns	= 33,   // Taht=10, Twrh=33, Tdht=10
	.lcdc_t_wstrobe_ns	= 33,	// Twrl=33, Tcs=15
	.lcdc_t_wsu_ns		= 0,	// Tast=0, no CS->WR_low timeout

	.display_t_reset_to_ready_ms	= 120,
	.display_t_sleep_in_out_ms	= 120,

	.display_idle			= false,
	.display_backlight		= true,
	.display_brightness		= 0x100,
	.display_inversion		= false,
	.display_gamma			= DA8XX_LCDC_DISPLAY_GAMMA_2_2,

	.cb_power_ctrl		= &da850_trik_lcd_power_ctrl,
	.cb_backlight_ctrl	= &da850_trik_lcd_backlight_ctrl,

};

static struct platform_device da850_trik_lcdc_device = {
	.name		= "da8xx_lcdc_ili9340",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(da850_trik_lcdc_resources),
	.resource	= da850_trik_lcdc_resources,
	.dev = {
		.platform_data 		= &da850_trik_lcdc_pdata,
	},
};

static int da850_trik_lcd_init(void){
	int ret;
#ifdef EXTRA_PINS 
	ret = davinci_cfg_reg_list(da850_trik_lcd_extra_pins);
	if (ret) {
		pr_err("%s: LCD extra pinmux setup failed: %d\n", __func__, ret);
		return ret;
	}
#endif
	ret = gpio_request_one(GPIO_TO_PIN(6, 12), GPIOF_OUT_INIT_LOW, "LCD backlight");
	if (ret){
		pr_err("%s: LCD backlight gpio request failed: %d\n", __func__, ret);
		return ret;
	}
	ret = gpio_request_one(GPIO_TO_PIN(8, 10), GPIOF_OUT_INIT_LOW, "LCD reset");
	if (ret) {
		pr_warning("%s: LCD reset gpio request failed: %d\n", __func__, ret);
		goto exit_request_one;
	}
	if (display_orientation){
		da850_trik_lcdc_pdata.xres			= 320;
		da850_trik_lcdc_pdata.yres			= 240;
		da850_trik_lcdc_pdata.xflip			= false;
		da850_trik_lcdc_pdata.yflip			= false;
		da850_trik_lcdc_pdata.xyswap			= true;
		da850_trik_lcdc_pdata.screen_height		= 37; //36,72mm
		da850_trik_lcdc_pdata.screen_width		= 49; //48,96mm
	}
	else 
	{
		da850_trik_lcdc_pdata.xres			= 240;
		da850_trik_lcdc_pdata.yres			= 320;
		da850_trik_lcdc_pdata.xflip			= true;
		da850_trik_lcdc_pdata.yflip			= false;
		da850_trik_lcdc_pdata.xyswap			= false;
		da850_trik_lcdc_pdata.screen_height		= 49; //48,96mm
		da850_trik_lcdc_pdata.screen_width		= 37; //36,72mm
	}
#if 0 
	ret = platform_device_register(&da850_trik_lcdc_device);
	if (ret) {
		pr_err("%s: LCD platform device register failed: %d\n", __func__, ret);
		goto exit_register_device;
	}
#endif 
	return 0;
exit_register_device:
	gpio_free(GPIO_TO_PIN(8, 10));
exit_request_one:
	gpio_free(GPIO_TO_PIN(6, 12));
	return ret;
}


#endif



#define REGDEFSPLIT_MASK(reg_ofs, reg_sz)			(((1ull<<(reg_sz))-1ull) << (reg_ofs))
#define REGDEFSPLIT_SET_VALUE(reg_ofs, reg_sz, val)		(((val) << (reg_ofs)) & REGDEFSPLIT_MASK(reg_ofs, reg_sz))
#define REGDEFSPLIT_GET_VALUE(reg_ofs, reg_sz, reg_val)		(((reg_val) & REGDEFSPLIT_MASK(reg_ofs, reg_sz)) >> (reg_ofs))
#define REGDEFSPLIT_VALUE_MAX(reg_ofs, reg_sz)			((1ull<<(reg_sz))-1ull)
#define REGDEFSPLIT_VALUE_OVF(reg_ofs, reg_sz, val, ovf)	(((val) > REGDEFSPLIT_VALUE_MAX(reg_ofs, reg_sz)) ? (++ovf, (REGDEFSPLIT_VALUE_MAX(reg_ofs, reg_sz))) : (val))
#define REGDEFSPLIT_SET_VALUE_OVF(reg_ofs, reg_sz, val, ovf)	((REGDEFSPLIT_VALUE_OVF(reg_ofs, reg_sz, val, ovf) << (reg_ofs)) & REGDEFSPLIT_MASK(reg_ofs, reg_sz))

#define REGDEF_MASK(reg_def)			REGDEFSPLIT_MASK(reg_def)
#define REGDEF_SET_VALUE(reg_def, val)		REGDEFSPLIT_SET_VALUE(reg_def, val)
#define REGDEF_SET_VALUE_OVF(reg_def, val, ovf)	REGDEFSPLIT_SET_VALUE_OVF(reg_def, val, ovf)
#define REGDEF_GET_VALUE(reg_def, reg_val)	REGDEFSPLIT_GET_VALUE(reg_def, reg_val)
#define REGDEF_MAX_VALUE(reg_def)		REGDEFSPLIT_VALUE_MAX(reg_def)




#define DA8XX_LCDCREG_REVID			0x00
#define DA8XX_LCDCREG_LCD_CTRL			0x04
#define DA8XX_LCDCREG_LCD_STAT			0x08
#define DA8XX_LCDCREG_LIDD_CTRL			0x0c
#define DA8XX_LCDCREG_LIDD_CS0_CONF		0x10
#define DA8XX_LCDCREG_LIDD_CS0_ADDR		0x14
#define DA8XX_LCDCREG_LIDD_CS0_DATA		0x18
#define DA8XX_LCDCREG_LIDD_CS1_CONF		0x1c
#define DA8XX_LCDCREG_LIDD_CS1_ADDR		0x20
#define DA8XX_LCDCREG_LIDD_CS1_DATA		0x24

#define DA8XX_LCDCREG_DMA_CTRL			0x40
#define DA8XX_LCDCREG_DMA_FB0_BASE		0x44
#define DA8XX_LCDCREG_DMA_FB0_CEILING		0x48
#define DA8XX_LCDCREG_DMA_FB1_BASE		0x4c
#define DA8XX_LCDCREG_DMA_FB1_CEILING		0x50


#define DA8XX_LCDCREG_REVID__REV			4,  (28)

#define DA8XX_LCDCREG_LCD_CTRL__MODESEL			0,  (1)
#define DA8XX_LCDCREG_LCD_CTRL__CLKDIV			8,  (8)

#define DA8XX_LCDCREG_LCD_STAT__DONE			0,  (1)
#define DA8XX_LCDCREG_LCD_STAT__EOF0			8,  (1)
#define DA8XX_LCDCREG_LCD_STAT__EOF1			9,  (1)

#define DA8XX_LCDCREG_LIDD_CTRL__MODE_SEL		0,  (3)
#define DA8XX_LCDCREG_LIDD_CTRL__ALEPOL			3,  (1)
#define DA8XX_LCDCREG_LIDD_CTRL__RS_EN_POL		4,  (1)
#define DA8XX_LCDCREG_LIDD_CTRL__WS_DIR_POL		5,  (1)
#define DA8XX_LCDCREG_LIDD_CTRL__CS0_E0_POL		6,  (1)
#define DA8XX_LCDCREG_LIDD_CTRL__CS1_E1_POL		7,  (1)
#define DA8XX_LCDCREG_LIDD_CTRL__LIDD_DMA_EN		8,  (1)
#define DA8XX_LCDCREG_LIDD_CTRL__DMA_CS0_CS1		9,  (1)
#define DA8XX_LCDCREG_LIDD_CTRL__DONE_INT_EN		10, (1)

#define DA8XX_LCDCREG_LIDD_CSn_CONF__TA			0,  (2)
#define DA8XX_LCDCREG_LIDD_CSn_CONF__R_HOLD		2,  (4)
#define DA8XX_LCDCREG_LIDD_CSn_CONF__R_STROBE		6,  (6)
#define DA8XX_LCDCREG_LIDD_CSn_CONF__R_SU		12, (5)
#define DA8XX_LCDCREG_LIDD_CSn_CONF__W_HOLD		17, (4)
#define DA8XX_LCDCREG_LIDD_CSn_CONF__W_STROBE		21, (6)
#define DA8XX_LCDCREG_LIDD_CSn_CONF__W_SU		27, (5)
#define DA8XX_LCDCREG_LIDD_CSn_ADDR__ADR_INDX		0,  (16)
#define DA8XX_LCDCREG_LIDD_CSn_DATA__DATA		0,  (16)

#define DA8XX_LCDCREG_DMA_CTRL__FRAME_MODE		0,  (1)
#define DA8XX_LCDCREG_DMA_CTRL__BIGENDIAN		1,  (1)
#define DA8XX_LCDCREG_DMA_CTRL__EOF_INT_EN		2,  (1)
#define DA8XX_LCDCREG_DMA_CTRL__BURST_SIZE		4,  (3)

#define DA8XX_LCDCREG_DMA_FBn_BASE__FBn_BASE		0,  (32)
#define DA8XX_LCDCREG_DMA_FBn_CEILING__FBn_CEIL		0,  (32)


#define DA8XX_LCDCREG_REVID__REV__id				0x4c10010 //Expected LCD controller ID: 4c10010x

#define DA8XX_LCDCREG_LCD_CTRL__MODESEL__lidd			0x0 //LCD controller in LIDD mode
#define DA8XX_LCDCREG_LCD_CTRL__CLKDIV__default			0x1 //MCLK to LCD_CLK by default

#define DA8XX_LCDCREG_LIDD_CTRL__MODE_SEL__6800sync		0x0 //MPU6800 sync
#define DA8XX_LCDCREG_LIDD_CTRL__MODE_SEL__6800async		0x1 //MPU6800 async
#define DA8XX_LCDCREG_LIDD_CTRL__MODE_SEL__8080sync		0x2 //8080 sync
#define DA8XX_LCDCREG_LIDD_CTRL__MODE_SEL__8080async		0x3 //8080 async
#define DA8XX_LCDCREG_LIDD_CTRL__xxxPOL__norm			0x0 //do not invert ALE / RS/EN / WS/DIR /CSn/En
#define DA8XX_LCDCREG_LIDD_CTRL__xxxPOL__inv			0x1 //invert ALE / RS/EN / WS/DIR /CSn/En
#define DA8XX_LCDCREG_LIDD_CTRL__LIDD_DMA_EN__deactivate	0x0 //deactivate DMA
#define DA8XX_LCDCREG_LIDD_CTRL__LIDD_DMA_EN__activate		0x1 //activate DMA
#define DA8XX_LCDCREG_LIDD_CTRL__DMA_CS0_CS1__cs0		0x0 //DMA writes to LIDD CS0
#define DA8XX_LCDCREG_LIDD_CTRL__DMA_CS0_CS1__cs1		0x1 //DMA writes to LIDD CS1
#define DA8XX_LCDCREG_LIDD_CTRL__DONE_INT_EN__enable		0x1 //Enable LIDD frame done interrupt
#define DA8XX_LCDCREG_LIDD_CTRL__DONE_INT_EN__disable		0x0 //Disable LIDD frame done interrupt

#define DA8XX_LCDCREG_DMA_CTRL__FRAME_MODE__single		0x0 //Single frame buffer (fb0)
#define DA8XX_LCDCREG_DMA_CTRL__BIGENDIAN__disable		0x0 //Big endian data reordering disabled
#define DA8XX_LCDCREG_DMA_CTRL__EOF_INT_EN__enable		0x1 //End of frame (CS0/CS1) interrupt enabled
#define DA8XX_LCDCREG_DMA_CTRL__EOF_INT_EN__disable		0x0 //End of frame (CS0/CS1) interrupt disabled
#define DA8XX_LCDCREG_DMA_CTRL__BURST_SIZE__1			0x0 //Burst size of 1
#define DA8XX_LCDCREG_DMA_CTRL__BURST_SIZE__2			0x1 //Burst size of 2
#define DA8XX_LCDCREG_DMA_CTRL__BURST_SIZE__4			0x2 //Burst size of 4
#define DA8XX_LCDCREG_DMA_CTRL__BURST_SIZE__8			0x3 //Burst size of 8
#define DA8XX_LCDCREG_DMA_CTRL__BURST_SIZE__16			0x4 //Burst size of 16

#define DA8XX_LCDCREG_DMA_FBn_BASE__ALIGNMENT			0x4 //DMA address alignment




#define ILI9340_CMD_NOP				0x00
#define ILI9340_CMD_RESET			0x01
#define ILI9340_CMD_READID			0x04
#define ILI9340_CMD_READ_SELFDIAG		0x0f
#define ILI9340_CMD_SLEEP_IN			0x10
#define ILI9340_CMD_SLEEP_OUT			0x11
#define ILI9340_CMD_INVERSION_OFF		0x20
#define ILI9340_CMD_INVERSION_ON		0x21
#define ILI9340_CMD_GAMMA			0x26
#define ILI9340_CMD_DISPLAY_OFF			0x28
#define ILI9340_CMD_DISPLAY_ON			0x29
#define ILI9340_CMD_COLUMN_ADDR			0x2a
#define ILI9340_CMD_ROW_ADDR			0x2b
#define ILI9340_CMD_MEMORY_WRITE		0x2c
#define ILI9340_CMD_MEMORY_ACCESS_CTRL		0x36
#define ILI9340_CMD_IDLE_OFF			0x38
#define ILI9340_CMD_IDLE_ON			0x39
#define ILI9340_CMD_PIXEL_FORMAT		0x3a
#define ILI9340_CMD_BRIGHTNESS			0x51
#define ILI9340_CMD_DISPLAY_CTRL		0x53
#define ILI9340_CMD_IFACE_CTRL			0xf6


#define ILI9340_CMD_READ_SELFDIAG__RLD		7, (1)
#define ILI9340_CMD_READ_SELFDIAG__FD		6, (1)
#define ILI9340_CMD_COLUMN_ADDR__LOWBYTE	0, (8)
#define ILI9340_CMD_COLUMN_ADDR__HIGHBYTE	0, (8)
#define ILI9340_CMD_ROW_ADDR__LOWBYTE		0, (8)
#define ILI9340_CMD_ROW_ADDR__HIGHBYTE		0, (8)
#define ILI9340_CMD_PIXEL_FORMAT__DBI		0, (3)
#define ILI9340_CMD_MEMORY_ACCESS_CTRL__BGR	3, (1)
#define ILI9340_CMD_MEMORY_ACCESS_CTRL__MV	5, (1)
#define ILI9340_CMD_MEMORY_ACCESS_CTRL__MX	6, (1)
#define ILI9340_CMD_MEMORY_ACCESS_CTRL__MY	7, (1)
#define ILI9340_CMD_DISPLAY_CTRL__BCTRL		5, (1)
#define ILI9340_CMD_DISPLAY_CTRL__DD		3, (1)
#define ILI9340_CMD_IFACE_CTRL__WEMODE		0, (1)
#define ILI9340_CMD_IFACE_CTRL__MDT		0, (2)
#define ILI9340_CMD_IFACE_CTRL__EPF		4, (2)




#define ILI9340_DISPLAY_CFG_BRIGHTNESS				0, (8)
#define ILI9340_DISPLAY_CFG_GAMMA				0, (4)
#define ILI9340_DISPLAY_CFG_GAMMA__1_0				0x08
#define ILI9340_DISPLAY_CFG_GAMMA__1_8				0x02
#define ILI9340_DISPLAY_CFG_GAMMA__2_2				0x01
#define ILI9340_DISPLAY_CFG_GAMMA__2_5				0x04
#define ILI9340_DISPLAY_CFG_FLIP_X				0, (1)
#define ILI9340_DISPLAY_CFG_FLIP_Y				1, (1)




/* Notes on timings for ILI9340-based display
   Page 216:
   Hardware reset -> Level2 command: 120ms
   Sleep out -> Display on sequence: 60ms

   Page 211:
   Sleep in -> power off: 120ms

   Page 90, 100, 101:
   Software reset -> any command: 5ms
   Software reset -> sleep out: 120ms
   Sleep in/out -> sleep out/in: 120ms
   Sleep in/out -> any command: 5ms
   Sleep out -> self diagnostics: 5ms
   Sleep out -> supplier's value loaded to registers: 120ms
*/



struct da8xx_ili9340_par_display_settings {
	atomic_t			changed;

	atomic_t			disp_on;
	atomic_t			disp_idle;
	atomic_t			disp_inversion;
	atomic_t			disp_gamma;
	atomic_t			disp_flip;
	atomic_t			disp_brightness;
	atomic_t			disp_backlight;
	atomic_t			disp_vcom;
	atomic_t			disp_vcom_offset;
	atomic_t			disp_vrh;
	atomic_t			disp_vdv;
	atomic_t			disp_gctrl;
};

struct da8xx_ili9340_par {
	__u32				disp_id;
	__u32				pseudo_palette[16];

	struct fb_info*					fb_info;
	enum da8xx_ili9340_pdata_lcdc_visual_mode	fb_visual_mode;

	dma_addr_t			fb_dma_phaddr;
	size_t				fb_dma_phsize;

	struct semaphore		lcdc_semaphore;

	resource_size_t			lcdc_reg_start;
	resource_size_t			lcdc_reg_size;
	void __iomem*			lcdc_reg_base;
	int				lcdc_irq;
	struct clk*			lcdc_clk;

	struct delayed_work		display_redraw_work;
	atomic_t			display_redraw_requested;
	atomic_t			display_redraw_ongoing;
	wait_queue_head_t		display_redraw_completion;

	bool						display_swapxy;
	struct da8xx_ili9340_par_display_settings	display_settings;

	loff_t				lidd_reg_cs_conf;
	loff_t				lidd_reg_cs_addr;
	loff_t				lidd_reg_cs_data;

	unsigned			ili9340_t_reset_to_ready_ms;
	unsigned			ili9340_t_sleep_in_out_ms;

	void		(*cb_power_ctrl)(bool _power_up);
	void		(*cb_backlight_ctrl)(bool _backlight);

	unsigned long			perf_count;
};

static const struct fb_fix_screeninfo da8xx_ili9340_fix_init = {
	.id		= NICE_NAME,
	//.smem_start, .smem_len
	.type		= FB_TYPE_PACKED_PIXELS,
	.type_aux	= 0,
	.visual		= FB_VISUAL_TRUECOLOR,
	.xpanstep	= 0,
	.ypanstep	= 1, // allow vertical panning
	.ywrapstep	= 0,
	//.line_length
	.mmio_start	= 0,
	.mmio_len	= 0,
	.accel		= FB_ACCEL_NONE,
	.capabilities	= 0,
};

static const struct fb_var_screeninfo da8xx_ili9340_var_init  = {
	//.xres, .yres, .xres_virtual, .yres_virtual
	.xoffset	= 0,
	.yoffset	= 0,
	//.bits_per_pixel
	.grayscale	= 0,
	.red		= { .msb_right = 0 },
	.green		= { .msb_right = 0 },
	.blue		= { .msb_right = 0 },
	.transp		= { .msb_right = 0, .offset = 0, .length = 0 },
	.nonstd		= 0,
	.activate	= FB_ACTIVATE_NOW,
	//.height, .width
	.accel_flags	= 0,
	//.pixclock, .left_margin, .right_margin, .upper_margin, .lower_margin, .hsync_len, .vsync_len
	.sync		= 0,
	.vmode		= FB_VMODE_NONINTERLACED,
	.rotate		= FB_ROTATE_UR,
	.colorspace	= 0,
};

static void	defio_redraw(struct fb_info* _info, struct list_head* _pagelist);

static struct fb_deferred_io da8xx_ili9340_defio = {
	//.delay
	.deferred_io		= defio_redraw,
};

static ssize_t	fbops_write(struct fb_info* _info, const char __user* _buf, size_t _count, loff_t* _ppos);
static void	fbops_fillrect(struct fb_info* _info, const struct fb_fillrect* _rect);
static void	fbops_copyarea(struct fb_info* _info, const struct fb_copyarea* _region);
static void	fbops_imageblit(struct fb_info* _info, const struct fb_image* _image);
static int	fbops_pan_display(struct fb_var_screeninfo* _var, struct fb_info* _info);
static int	fbops_blank(int _blank, struct fb_info* _info);
static int	fbops_setcolreg(unsigned _regno, unsigned _red, unsigned _green, unsigned _blue, unsigned _transp, struct fb_info* _info);

static struct fb_ops da8xx_ili9340_fbops = {
	.owner		= THIS_MODULE,
	.fb_read	= &fb_sys_read,
	.fb_write	= &fbops_write,
	.fb_fillrect	= &fbops_fillrect,
	.fb_copyarea	= &fbops_copyarea,
	.fb_imageblit	= &fbops_imageblit,
	.fb_pan_display	= &fbops_pan_display,
	.fb_blank	= &fbops_blank,
	.fb_setcolreg	= &fbops_setcolreg,
};

static void		display_schedule_redraw(struct device* _dev, struct da8xx_ili9340_par* _par, bool _disp_settings_changed);
static int		display_start_redraw_locked(struct device* _dev, struct da8xx_ili9340_par* _par);
static int		display_wait_redraw_completion(struct device* _dev, struct da8xx_ili9340_par* _par);
static void		display_visibility_settings_update(struct device* _dev, struct da8xx_ili9340_par* _par);
static void		display_alignment_settings_update(struct device* _dev, struct da8xx_ili9340_par* _par);

static void		display_redraw_work(struct work_struct* _work);
static void 		lcdc_edma_start(struct device* _dev, struct da8xx_ili9340_par* _par);
static irqreturn_t	lcdc_edma_done(int _irq, void* _dev);
static void		display_redraw_work_done(struct device* _dev, struct da8xx_ili9340_par* _par);




static ssize_t		sysfs_id_show(struct device* _fbdev, struct device_attribute* _attr, char* _buf);
static ssize_t		sysfs_idle_show(struct device* _fbdev, struct device_attribute* _attr, char* _buf);
static ssize_t		sysfs_idle_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count);
static ssize_t		sysfs_inversion_show(struct device* _fbdev, struct device_attribute* _attr, char* _buf);
static ssize_t		sysfs_inversion_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count);
static ssize_t		sysfs_gamma_show(struct device* _fbdev, struct device_attribute* _attr, char* _buf);
static ssize_t		sysfs_gamma_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count);
static ssize_t		sysfs_flip_show(struct device* _fbdev, struct device_attribute* _attr, char* _buf);
static ssize_t		sysfs_flip_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count);
static ssize_t		sysfs_brightness_show(struct device* _fbdev, struct device_attribute* _attr, char* _buf);
static ssize_t		sysfs_brightness_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count);
static ssize_t		sysfs_backlight_show(struct device* _fbdev, struct device_attribute* _attr, char* _buf);
static ssize_t		sysfs_backlight_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count);
//static ssize_t		sysfs_perf_count_show(struct device* _fbdev, struct device_attribute* _attr, char* _buf);
static ssize_t		sysfs_perf_count_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count);
static ssize_t		sysfs_color_fill_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count);

static ssize_t		sysfs_vcom_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count);
static ssize_t		sysfs_vcom_offset_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count);
static ssize_t		sysfs_vrh_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count);
static ssize_t		sysfs_vdv_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count);
static ssize_t		sysfs_gctrl_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count);

static struct device_attribute da8xx_ili9340_sysfs_attrs[] = {
	__ATTR(id,		S_IRUGO,	&sysfs_id_show, NULL),
	__ATTR(idle,		S_IRUGO|S_IWUSR,	&sysfs_idle_show,		&sysfs_idle_store),
	__ATTR(inversion,	S_IRUGO|S_IWUSR,	&sysfs_inversion_show,		&sysfs_inversion_store),
	__ATTR(gamma,		S_IRUGO|S_IWUSR,	&sysfs_gamma_show,		&sysfs_gamma_store),
	__ATTR(flip,		S_IRUGO|S_IWUSR,	&sysfs_flip_show,		&sysfs_flip_store),
/*	__ATTR(brightness,	S_IRUGO|S_IWUSR,	&sysfs_brightness_show,		&sysfs_brightness_store), */ 
	__ATTR(backlight,	S_IRUGO|S_IWUSR,	&sysfs_backlight_show,		&sysfs_backlight_store),
//	__ATTR(perf_count,	S_IRUSR|S_IWUSR,	&sysfs_perf_count_show,		&sysfs_perf_count_store),
	__ATTR(color_fill,	S_IWUSR,		NULL,				&sysfs_color_fill_store),
	__ATTR(vcom,		S_IWUSR,		NULL,				&sysfs_vcom_store),
	__ATTR(vcom_offset,		S_IWUSR,		NULL,				&sysfs_vcom_offset_store),
	__ATTR(vrh,		S_IWUSR,		NULL,				&sysfs_vrh_store),
	__ATTR(vdv,		S_IWUSR,		NULL,				&sysfs_vdv_store),
	__ATTR(gctrl,		S_IWUSR,		NULL,				&sysfs_gctrl_store),
};




static inline int lcdc_lock(struct da8xx_ili9340_par* _par)
{
	return down_interruptible(&_par->lcdc_semaphore);
}

static inline void lcdc_unlock(struct da8xx_ili9340_par* _par)
{
	up(&_par->lcdc_semaphore);
}

static inline void lcdc_assert_locked(struct da8xx_ili9340_par* _par)
{
	BUG_ON(_par->lcdc_semaphore.count != 0);
}

static inline __u32 lcdc_reg_read(struct device* _dev, struct da8xx_ili9340_par* _par, loff_t _reg)
{
	void __iomem*	reg_ptr		= _par->lcdc_reg_base + _reg;

	BUG_ON(_par->lcdc_reg_base == NULL);
	lcdc_assert_locked(_par);

	return __raw_readl(reg_ptr);
}

static inline void lcdc_reg_write(struct device* _dev, struct da8xx_ili9340_par* _par, loff_t _reg, __u32 _value)
{
	void __iomem*	reg_ptr		= _par->lcdc_reg_base + _reg;

	BUG_ON(_par->lcdc_reg_base == NULL);
	lcdc_assert_locked(_par);

	__raw_writel(_value, reg_ptr);
}

static inline void lcdc_reg_change(struct device* _dev, struct da8xx_ili9340_par* _par, loff_t _reg, __u32 _mask, __u32 _value)
{
	void __iomem*	reg_ptr		= _par->lcdc_reg_base + _reg;

	BUG_ON(_par->lcdc_reg_base == NULL);
	lcdc_assert_locked(_par);

	__raw_writel((__raw_readl(reg_ptr) & ~_mask) | _value, reg_ptr);
}

static inline __u16 display_read_data(struct device* _dev, struct da8xx_ili9340_par* _par)
{
	return lcdc_reg_read(_dev, _par, _par->lidd_reg_cs_data);
}

static inline void display_write_cmd(struct device* _dev, struct da8xx_ili9340_par* _par, __u16 _cmd)
{
	lcdc_reg_write(_dev, _par, _par->lidd_reg_cs_addr, _cmd);
}

static inline void display_write_data(struct device* _dev, struct da8xx_ili9340_par* _par, __u16 _data)
{
	lcdc_reg_write(_dev, _par, _par->lidd_reg_cs_data, _data);
}




static ssize_t fbops_write(struct fb_info* _info, const char __user* _buf, size_t _count, loff_t* _ppos)
{
	ssize_t ret;
	struct device* dev		= _info->device;
	struct da8xx_ili9340_par* par	= _info->par;

	dev_dbg(dev, "%s: called\n", __func__);
	ret = fb_sys_write(_info, _buf, _count, _ppos);
	if (ret < 0)
		return ret;

	display_schedule_redraw(dev, par, false);
	dev_dbg(dev, "%s: done\n", __func__);

	return ret;
}

static void fbops_fillrect(struct fb_info* _info, const struct fb_fillrect* _rect)
{
	struct device* dev		= _info->device;
	struct da8xx_ili9340_par* par	= _info->par;

	dev_dbg(dev, "%s: called\n", __func__);
	sys_fillrect(_info, _rect);
	display_schedule_redraw(dev, par, false);
	dev_dbg(dev, "%s: done\n", __func__);
}

static void fbops_copyarea(struct fb_info* _info, const struct fb_copyarea* _region)
{
	struct device* dev		= _info->device;
	struct da8xx_ili9340_par* par	= _info->par;

	dev_dbg(dev, "%s: called\n", __func__);
	sys_copyarea(_info, _region);
	display_schedule_redraw(dev, par, false);
	dev_dbg(dev, "%s: done\n", __func__);
}

static void fbops_imageblit(struct fb_info* _info, const struct fb_image* _image)
{
	struct device* dev		= _info->device;
	struct da8xx_ili9340_par* par	= _info->par;

//	dev_dbg(dev, "%s: called\n", __func__);
	sys_imageblit(_info, _image);
	display_schedule_redraw(dev, par, false);
//	dev_dbg(dev, "%s: done\n", __func__);
}

static int fbops_pan_display(struct fb_var_screeninfo* _var, struct fb_info* _info)
{
	struct device* dev		= _info->device;
	struct da8xx_ili9340_par* par	= _info->par;

//	dev_dbg(dev, "%s: called\n", __func__);
	if (   _info->fix.ypanstep == 0
	    || _var->yoffset % _info->fix.ypanstep != 0
	    || _var->yoffset+_info->var.yres > _info->var.yres_virtual) {
		dev_err(dev, "%s: failed \n", __func__);
		return -EINVAL;
	}

	_info->var.yoffset = _var->yoffset;

	display_schedule_redraw(dev, par, true);
//	dev_dbg(dev, "%s: done\n", __func__);

	return 0;
}

static int fbops_blank(int _blank, struct fb_info* _info)
{
	struct device* dev		= _info->device;
	struct da8xx_ili9340_par* par	= _info->par;

	dev_dbg(dev, "%s: called\n", __func__);

	switch (_blank) {
		case FB_BLANK_UNBLANK:
			atomic_set(&par->display_settings.disp_on, true);
			break;
		case FB_BLANK_POWERDOWN:
			atomic_set(&par->display_settings.disp_on, false);
			break;
		default:
			dev_err(dev, "%s: failed for 0x%04x\n", __func__, _blank);
			return -EINVAL;
	}

	display_schedule_redraw(dev, par, true);
	dev_dbg(dev, "%s: done\n", __func__);

	return 0;
}

static inline unsigned colreg(unsigned _value, struct fb_bitfield* _colordef)
{
	return (_value >> (16 - _colordef->length)) << _colordef->offset;
}

static int fbops_setcolreg(unsigned _regno, unsigned _red, unsigned _green, unsigned _blue, unsigned _transp, struct fb_info* _info)
{
	struct device* dev		= _info->device;
	struct da8xx_ili9340_par* par	= _info->par;


	if (_regno >= ARRAY_SIZE(par->pseudo_palette)) {
//		dev_err(dev, "%s: failed, 0x%04x <- #%04x;%04x;%04x\n", __func__, _regno, _red, _green, _blue);
		return -EINVAL; 
	}

	par->pseudo_palette[_regno]	= colreg(_red,	&_info->var.red)
					| colreg(_green,&_info->var.green)
					| colreg(_blue,	&_info->var.blue);

//	dev_dbg(dev, "%s: done\n", __func__);

	return 0;
}




static void defio_redraw(struct fb_info* _info, struct list_head* _pagelist)
{
	struct device* dev		= _info->device;
	struct da8xx_ili9340_par* par	= _info->par;

	display_schedule_redraw(dev, par, false);
}




static inline void _display_redraw_work_done(struct device* _dev, struct da8xx_ili9340_par* _par)
{
	atomic_set(&_par->display_redraw_ongoing, 0);
	wake_up_all(&_par->display_redraw_completion);

	if (atomic_read(&_par->display_redraw_requested) == 0)
		return;

	if (atomic_inc_return(&_par->display_redraw_ongoing) == 1) { // i.e. it was 0 and no work were ongoing
		dev_dbg(_dev, "%s: re-scheduling redraw work\n", __func__);
		atomic_set(&_par->display_redraw_requested, 0);
		schedule_delayed_work(&_par->display_redraw_work, 1);
	}
}

static void display_schedule_redraw(struct device* _dev, struct da8xx_ili9340_par* _par, bool _disp_settings_changed)
{
//	dev_dbg(_dev, "%s: called\n", __func__);

	atomic_inc(&_par->display_redraw_requested);
	if (_disp_settings_changed)
		atomic_inc(&_par->display_settings.changed);

	if (atomic_inc_return(&_par->display_redraw_ongoing) == 1) { // i.e. it was 0 and no work were ongoing
//		dev_dbg(_dev, "%s: scheduling redraw work\n", __func__);
		atomic_set(&_par->display_redraw_requested, 0);
		schedule_delayed_work(&_par->display_redraw_work, 0);
	} // otherwise, we already incremented redraw_requested and work should be re-scheduled at completion
//	dev_dbg(_dev, "%s: done\n", __func__);
}

static int display_start_redraw_locked(struct device* _dev, struct da8xx_ili9340_par* _par)
{
	if (atomic_inc_return(&_par->display_redraw_ongoing) != 1) // i.e. it was already running
		return -EBUSY;

	dev_dbg(_dev, "%s: starting redraw work\n", __func__);
	lcdc_assert_locked(_par);
	lcdc_edma_start(_dev, _par);
	return 0;
}

static int display_wait_redraw_completion(struct device* _dev, struct da8xx_ili9340_par* _par)
{
	int ret;

	dev_dbg(_dev, "%s: called\n", __func__);
	ret = wait_event_interruptible(_par->display_redraw_completion, (atomic_read(&_par->display_redraw_ongoing) == 0));
	dev_dbg(_dev, "%s: done\n", __func__);

	return ret;
}

static void display_visibility_settings_update(struct device* _dev, struct da8xx_ili9340_par* _par)
{
	dev_dbg(_dev, "%s: called\n", __func__);
	lcdc_assert_locked(_par);
	bool idle;
	__u16 gamma;
	__u16 brightness;
	// Turn display off before re-configuring
	display_write_cmd(_dev, _par, ILI9340_CMD_DISPLAY_OFF);
	idle = atomic_read(&_par->display_settings.disp_idle);
	display_write_cmd(_dev, _par, idle?ILI9340_CMD_IDLE_ON:ILI9340_CMD_IDLE_OFF);

	display_write_cmd(_dev, _par, atomic_read(&_par->display_settings.disp_inversion)?ILI9340_CMD_INVERSION_ON:ILI9340_CMD_INVERSION_OFF);

	gamma		= REGDEF_GET_VALUE(ILI9340_DISPLAY_CFG_GAMMA, atomic_read(&_par->display_settings.disp_gamma));
	display_write_cmd(_dev, _par, ILI9340_CMD_GAMMA);
	display_write_data(_dev, _par, gamma);
	display_write_cmd(_dev, _par, 0xbb);
	display_write_data(_dev, _par, atomic_read(&_par->display_settings.disp_vcom));
	display_write_cmd(_dev, _par, 0xc5);
	display_write_data(_dev, _par, atomic_read(&_par->display_settings.disp_vcom_offset));
	display_write_cmd(_dev, _par, 0xc3);
	display_write_data(_dev, _par, atomic_read(&_par->display_settings.disp_vrh));
	display_write_cmd(_dev, _par, 0xc4);
	display_write_data(_dev, _par, atomic_read(&_par->display_settings.disp_vdv));
	display_write_cmd(_dev, _par, 0xb7);
	display_write_data(_dev, _par, atomic_read(&_par->display_settings.disp_gctrl));


#if 0 
TODO: trying to control brightness breaks those newhaven displays, which are based on the ST7789S LCD controller
	brightness	= REGDEF_GET_VALUE(ILI9340_DISPLAY_CFG_BRIGHTNESS, atomic_read(&_par->display_settings.disp_brightness));
	display_write_cmd(_dev, _par, ILI9340_CMD_BRIGHTNESS);
	display_write_data(_dev, _par, brightness);

	display_write_cmd(_dev, _par, ILI9340_CMD_DISPLAY_CTRL);
	display_write_data(_dev, _par,
			0
			| REGDEF_SET_VALUE(ILI9340_CMD_DISPLAY_CTRL__BCTRL, 1)
			| REGDEF_SET_VALUE(ILI9340_CMD_DISPLAY_CTRL__DD, idle?1:0));

#endif
	int isOn = atomic_read(&_par->display_settings.disp_on);
	display_write_cmd(_dev, _par, isOn ? ILI9340_CMD_DISPLAY_ON : ILI9340_CMD_DISPLAY_OFF);
	if (_par->cb_backlight_ctrl)
			_par->cb_backlight_ctrl(isOn && atomic_read(&_par->display_settings.disp_backlight));
	
	dev_dbg(_dev, "%s: done\n", __func__);
}

static void display_alignment_settings_update(struct device* _dev, struct da8xx_ili9340_par* _par)
{
	unsigned flip;
	__u16 flipx;
	__u16 flipy;

	dev_dbg(_dev, "%s: called\n", __func__);
	lcdc_assert_locked(_par);

	flip	= atomic_read(&_par->display_settings.disp_flip);
	if (_par->display_swapxy) {
		flipx	= REGDEF_GET_VALUE(ILI9340_DISPLAY_CFG_FLIP_Y, flip);
		flipy	= REGDEF_GET_VALUE(ILI9340_DISPLAY_CFG_FLIP_X, flip);
	} else {
		flipx	= REGDEF_GET_VALUE(ILI9340_DISPLAY_CFG_FLIP_X, flip);
		flipy	= REGDEF_GET_VALUE(ILI9340_DISPLAY_CFG_FLIP_Y, flip);
	}

	display_write_cmd(_dev, _par, ILI9340_CMD_MEMORY_ACCESS_CTRL);
	display_write_data(_dev, _par,
			0
			| REGDEF_SET_VALUE(ILI9340_CMD_MEMORY_ACCESS_CTRL__MX, flipx)
			| REGDEF_SET_VALUE(ILI9340_CMD_MEMORY_ACCESS_CTRL__MY, flipy)
			| REGDEF_SET_VALUE(ILI9340_CMD_MEMORY_ACCESS_CTRL__MV, _par->display_swapxy?1:0)
			| REGDEF_SET_VALUE(ILI9340_CMD_MEMORY_ACCESS_CTRL__BGR, 1)
	);

	display_write_cmd(_dev, _par, 0xc0); //LCMCTRL for ST7789S controller. Default is 0x2C   
	display_write_data(_dev, _par, 0x00); // Invert X for ST7789S
			
	dev_dbg(_dev, "%s: done\n", __func__);
}


static void display_redraw_work(struct work_struct* _work)
{
	int ret;
	struct da8xx_ili9340_par* par	= container_of(to_delayed_work(_work), struct da8xx_ili9340_par, display_redraw_work);
	struct fb_info* info		= par->fb_info;
	struct device* dev		= info->device;

//	dev_dbg(dev, "%s: starting redraw work\n", __func__);
	ret = lcdc_lock(par);
	if (ret) {
		dev_err(dev, "%s: cannot obtain LCD controller lock: %d\n", __func__, ret);
		_display_redraw_work_done(dev, par);
		return;
	}
	lcdc_edma_start(dev, par);
}

static void lcdc_edma_start(struct device* _dev, struct da8xx_ili9340_par* _par)
{
	struct fb_info* info		= dev_get_drvdata(_dev);
	dma_addr_t phaddr_base;
	dma_addr_t phaddr_ceil;

	lcdc_assert_locked(_par);

	if (info->var.xoffset != 0)
		dev_warn(_dev, "%s: xoffset != 0\n", __func__);
	phaddr_base = _par->fb_dma_phaddr + info->var.yoffset*info->fix.line_length;
	phaddr_ceil = phaddr_base + info->var.yres*info->fix.line_length - 1;

	if (atomic_read(&_par->display_settings.changed))
		display_alignment_settings_update(_dev, _par);

	display_write_cmd(_dev, _par, ILI9340_CMD_MEMORY_WRITE);

	// Set EDMA address
	lcdc_reg_write(_dev, _par,
			DA8XX_LCDCREG_DMA_FB0_BASE,
			REGDEF_SET_VALUE(DA8XX_LCDCREG_DMA_FBn_BASE__FBn_BASE, phaddr_base));
	lcdc_reg_write(_dev, _par,
			DA8XX_LCDCREG_DMA_FB0_CEILING,
			REGDEF_SET_VALUE(DA8XX_LCDCREG_DMA_FBn_CEILING__FBn_CEIL, phaddr_ceil));

	// Kick EDMA on
	lcdc_reg_change(_dev, _par,
			DA8XX_LCDCREG_LIDD_CTRL,
			REGDEF_MASK(DA8XX_LCDCREG_LIDD_CTRL__LIDD_DMA_EN),
			REGDEF_SET_VALUE(DA8XX_LCDCREG_LIDD_CTRL__LIDD_DMA_EN, DA8XX_LCDCREG_LIDD_CTRL__LIDD_DMA_EN__activate));
	lcdc_reg_change(_dev, _par,
			DA8XX_LCDCREG_LIDD_CTRL,
			REGDEF_MASK(DA8XX_LCDCREG_LIDD_CTRL__LIDD_DMA_EN),
			REGDEF_SET_VALUE(DA8XX_LCDCREG_LIDD_CTRL__LIDD_DMA_EN, DA8XX_LCDCREG_LIDD_CTRL__LIDD_DMA_EN__deactivate));
}

static irqreturn_t lcdc_edma_done(int _irq, void* _dev)
{
	struct device* dev		= _dev;
	struct fb_info* info		= dev_get_drvdata(dev);
	struct da8xx_ili9340_par* par	= info->par;

	lcdc_reg_change(dev, par, DA8XX_LCDCREG_LCD_STAT, 0x0, 0x0); //Write STAT register back - reset interrupt flag

	display_redraw_work_done(dev, par);

	return IRQ_HANDLED;
}

static void display_redraw_work_done(struct device* _dev, struct da8xx_ili9340_par* _par)
{
//	dev_dbg(_dev, "%s: completed redraw work\n", __func__);
	if (atomic_read(&_par->display_redraw_ongoing) == 0) {
		dev_err(_dev, "%s: redraw work done while not ongoing\n", __func__);
		return;
	}

	if (atomic_xchg(&_par->display_settings.changed, 0))
		display_visibility_settings_update(_dev, _par);

	lcdc_unlock(_par);
	_display_redraw_work_done(_dev, _par);
}




static ssize_t sysfs_id_show(struct device* _fbdev, struct device_attribute* _attr, char* _buf)
{
	struct fb_info* info		= dev_get_drvdata(_fbdev);
	struct da8xx_ili9340_par* par	= info->par;

	return snprintf(_buf, PAGE_SIZE, "%08X\n",  (unsigned)par->disp_id);
}

static ssize_t sysfs_idle_show(struct device* _fbdev, struct device_attribute* _attr, char* _buf)
{
	struct fb_info* info		= dev_get_drvdata(_fbdev);
	struct da8xx_ili9340_par* par	= info->par;

	return snprintf(_buf, PAGE_SIZE,
			"%u\n"
			"Values accepted: 0..1\n",
			(unsigned)atomic_read(&par->display_settings.disp_idle));
}

static ssize_t sysfs_idle_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count)
{
	int ret;
	struct fb_info* info		= dev_get_drvdata(_fbdev);
	struct device* dev		= info->device;
	struct da8xx_ili9340_par* par	= info->par;
	unsigned value;

	ret = kstrtouint(_buf, 0, &value);
	if (ret)
		return ret;

	atomic_set(&par->display_settings.disp_idle, value?1:0);

	display_schedule_redraw(dev, par, true);
	return _count;
}

static ssize_t sysfs_inversion_show(struct device* _fbdev, struct device_attribute* _attr, char* _buf)
{
	struct fb_info* info		= dev_get_drvdata(_fbdev);
	struct da8xx_ili9340_par* par	= info->par;

	return snprintf(_buf, PAGE_SIZE,
			"%u\n"
			"Values accepted: 0..1\n",
			(unsigned)atomic_read(&par->display_settings.disp_inversion));
}

static ssize_t sysfs_inversion_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count)
{
	int ret;
	struct fb_info* info		= dev_get_drvdata(_fbdev);
	struct device* dev		= info->device;
	struct da8xx_ili9340_par* par	= info->par;
	unsigned value;

	ret = kstrtouint(_buf, 0, &value);
	if (ret)
		return ret;

	atomic_set(&par->display_settings.disp_inversion, value?1:0);

	display_schedule_redraw(dev, par, true);
	return _count;
}

static ssize_t sysfs_gamma_show(struct device* _fbdev, struct device_attribute* _attr, char* _buf)
{
	struct fb_info* info		= dev_get_drvdata(_fbdev);
	struct da8xx_ili9340_par* par	= info->par;
	const char* gamma;

	switch (REGDEF_GET_VALUE(ILI9340_DISPLAY_CFG_GAMMA, atomic_read(&par->display_settings.disp_gamma))) {
		case ILI9340_DISPLAY_CFG_GAMMA__1_0:	gamma = "1.0";	break;
		case ILI9340_DISPLAY_CFG_GAMMA__1_8:	gamma = "1.8";	break;
		case ILI9340_DISPLAY_CFG_GAMMA__2_2:	gamma = "2.2";	break;
		case ILI9340_DISPLAY_CFG_GAMMA__2_5:	gamma = "2.5";	break;
		default:				gamma = "unknown";	break;
	}

	return snprintf(_buf, PAGE_SIZE,
			"%s\n"
			"Values accepted: 1.0 1.8 2.2 2.5\n",
			gamma);
}

static ssize_t sysfs_gamma_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count)
{
	struct fb_info* info		= dev_get_drvdata(_fbdev);
	struct device* dev		= info->device;
	struct da8xx_ili9340_par* par	= info->par;

	if (!strncmp(_buf, "1.0", 3))
		atomic_set(&par->display_settings.disp_gamma,
			REGDEF_SET_VALUE(ILI9340_DISPLAY_CFG_GAMMA, ILI9340_DISPLAY_CFG_GAMMA__1_0));
	else if (!strncmp(_buf, "1.8", 3))
		atomic_set(&par->display_settings.disp_gamma,
			REGDEF_SET_VALUE(ILI9340_DISPLAY_CFG_GAMMA, ILI9340_DISPLAY_CFG_GAMMA__1_8));
	else if (!strncmp(_buf, "2.2", 3))
		atomic_set(&par->display_settings.disp_gamma,
			REGDEF_SET_VALUE(ILI9340_DISPLAY_CFG_GAMMA, ILI9340_DISPLAY_CFG_GAMMA__2_2));
	else if (!strncmp(_buf, "2.5", 3))
		atomic_set(&par->display_settings.disp_gamma,
			REGDEF_SET_VALUE(ILI9340_DISPLAY_CFG_GAMMA, ILI9340_DISPLAY_CFG_GAMMA__2_5));
	else
		return -EINVAL;

	display_schedule_redraw(dev, par, true);
	return _count;
}

static ssize_t sysfs_flip_show(struct device* _fbdev, struct device_attribute* _attr, char* _buf)
{
	struct fb_info* info		= dev_get_drvdata(_fbdev);
	struct da8xx_ili9340_par* par	= info->par;
	unsigned flip = atomic_read(&par->display_settings.disp_flip);

	return snprintf(_buf, PAGE_SIZE,
			"%s%s\n"
			"Values accepted: <empty> x y xy\n"
			"%s%s%s",
			REGDEF_GET_VALUE(ILI9340_DISPLAY_CFG_FLIP_X, flip)?"x":"", REGDEF_GET_VALUE(ILI9340_DISPLAY_CFG_FLIP_Y, flip)?"y":"",
			REGDEF_GET_VALUE(ILI9340_DISPLAY_CFG_FLIP_X, flip)?"Horizontal flip\n":"",
			REGDEF_GET_VALUE(ILI9340_DISPLAY_CFG_FLIP_Y, flip)?"Vertical flip\n":"",
			par->display_swapxy?"Internally swapped X and Y axis\n":"");
}

static ssize_t sysfs_flip_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count)
{
	struct fb_info* info		= dev_get_drvdata(_fbdev);
	struct device* dev		= info->device;
	struct da8xx_ili9340_par* par	= info->par;
	size_t idx = 0;
	unsigned value = 0;

	for (idx = 0; idx < _count; ++idx)
		switch (_buf[idx]) {
			case 'x':
			case 'X':	value |= REGDEF_SET_VALUE(ILI9340_DISPLAY_CFG_FLIP_X, 1);	break;
			case 'y':
			case 'Y':	value |= REGDEF_SET_VALUE(ILI9340_DISPLAY_CFG_FLIP_Y, 1);	break;
		}

	atomic_set(&par->display_settings.disp_flip, value);

	display_schedule_redraw(dev, par, true);
	return _count;
}

static ssize_t sysfs_brightness_show(struct device* _fbdev, struct device_attribute* _attr, char* _buf)
{
	struct fb_info* info		= dev_get_drvdata(_fbdev);
	struct da8xx_ili9340_par* par	= info->par;

	return snprintf(_buf, PAGE_SIZE,
			"%u\n"
			"Values accepted: 0..%u\n",
			(unsigned)REGDEF_GET_VALUE(ILI9340_DISPLAY_CFG_BRIGHTNESS, atomic_read(&par->display_settings.disp_brightness)),
			(unsigned)REGDEF_MAX_VALUE(ILI9340_DISPLAY_CFG_BRIGHTNESS));
}

static ssize_t sysfs_brightness_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count)
{
	int ret;
	struct fb_info* info		= dev_get_drvdata(_fbdev);
	struct device* dev		= info->device;
	struct da8xx_ili9340_par* par	= info->par;
	unsigned value;
	unsigned ovf = 0;

	ret = kstrtouint(_buf, 0, &value);
	if (ret)
		return ret;

	atomic_set(&par->display_settings.disp_brightness, REGDEF_SET_VALUE_OVF(ILI9340_DISPLAY_CFG_BRIGHTNESS, value, ovf));

	display_schedule_redraw(dev, par, true);
	return _count;
}

static ssize_t sysfs_backlight_show(struct device* _fbdev, struct device_attribute* _attr, char* _buf)
{
	struct fb_info* info		= dev_get_drvdata(_fbdev);
	struct da8xx_ili9340_par* par	= info->par;

	return snprintf(_buf, PAGE_SIZE,
			"%u\n"
			"Values accepted: 0..1\n",
			(unsigned)atomic_read(&par->display_settings.disp_backlight));
}

static ssize_t sysfs_backlight_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count)
{
	int ret;
	struct fb_info* info		= dev_get_drvdata(_fbdev);
	struct device* dev		= info->device;
	struct da8xx_ili9340_par* par	= info->par;
	unsigned value;

	ret = kstrtouint(_buf, 0, &value);
	if (ret)
		return ret;

	atomic_set(&par->display_settings.disp_backlight, value?1:0);

	display_schedule_redraw(dev, par, true);
	return _count;
}

#if 0 
static ssize_t sysfs_perf_count_show(struct device* _fbdev, struct device_attribute* _attr, char* _buf)
{
	struct fb_info* info		= dev_get_drvdata(_fbdev);
	struct device* dev		= info->device;
	struct da8xx_ili9340_par* par	= info->par;

	unsigned retry;
	struct timespec started_at1, started_at2;
	struct timespec spent_time1, spent_time2;
	unsigned long ns_per_redraw1, ns_per_redraw2;

	started_at1	= CURRENT_TIME;
	for (retry = 0; retry < par->perf_count; ++retry) {
		int ret;
		display_schedule_redraw(dev, par, false);
		ret = display_wait_redraw_completion(dev, par);
		if (ret)
			return ret;
	}
	spent_time1	= timespec_sub(CURRENT_TIME, started_at1);

	started_at2	= CURRENT_TIME;
	for (retry = 0; retry < par->perf_count; ++retry) {
		int ret;
		display_schedule_redraw(dev, par, true);
		ret = display_wait_redraw_completion(dev, par);
		if (ret)
			return ret;
	}
	spent_time2	= timespec_sub(CURRENT_TIME, started_at2);

	ns_per_redraw1	=  spent_time1.tv_nsec / par->perf_count; // calculate this way to avoid overflow
	ns_per_redraw1	+= spent_time1.tv_sec * (NSEC_PER_SEC / par->perf_count);
	ns_per_redraw2	=  spent_time2.tv_nsec / par->perf_count; // calculate this way to avoid overflow
	ns_per_redraw2	+= spent_time2.tv_sec * (NSEC_PER_SEC / par->perf_count);

	return snprintf(_buf, PAGE_SIZE,
			"Performance tested for %u redraw\n"
			"It took %lu.%09lu for regular redraws\n"
			"Single redraw takes %luns in average\n"
			"It took %lu.%09lu for full redraws (with display settings apply)\n"
			"Single redraw takes %luns in average\n"
			"Maximum possible FPS is %lu (full redraw)\n",
			(unsigned)par->perf_count,
			(unsigned long)spent_time1.tv_sec, (unsigned long)spent_time1.tv_nsec,
			ns_per_redraw1,
			(unsigned long)spent_time2.tv_sec, (unsigned long)spent_time2.tv_nsec,
			ns_per_redraw2,
			(unsigned long)(NSEC_PER_SEC/ns_per_redraw2));
}
#endif 

static ssize_t sysfs_perf_count_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count)
{
	int ret;
	struct fb_info* info		= dev_get_drvdata(_fbdev);
	struct da8xx_ili9340_par* par	= info->par;

	ret = kstrtoul(_buf, 0, &par->perf_count);
	if (ret)
		return ret;

	return _count;
}

static ssize_t sysfs_color_fill_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count)
{
	int ret;
	unsigned long fill_value;
	unsigned char fill[sizeof(fill_value)];
	unsigned fill_size;
	struct fb_info* info		= dev_get_drvdata(_fbdev);
	int ofs;

	ret = kstrtoul(_buf, 0, &fill_value);
	if (ret)
		return ret;

	fill_size = info->var.bits_per_pixel/BITS_PER_BYTE;
	if (fill_size > sizeof(fill))
		return -ENOMEM;

	for (ofs = 0; ofs < fill_size; ++ofs)
		fill[ofs] = (fill_value >> (ofs*BITS_PER_BYTE)) & ((1ul<<BITS_PER_BYTE)-1);

	for (ofs = 0; ofs < info->screen_size; ofs += fill_size)
		memcpy(info->screen_base+ofs, fill, fill_size);

	display_schedule_redraw(info->device, info->par, false);

	return _count;
}


static ssize_t sysfs_vcom_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count)
{
	int ret;
	struct fb_info* info		= dev_get_drvdata(_fbdev);
	struct device* dev		= info->device;
	struct da8xx_ili9340_par* par	= info->par;
	unsigned value;

	ret = kstrtouint(_buf, 0, &value);
	if (ret)
		return ret;

	atomic_set(&par->display_settings.disp_vcom, value);

	display_schedule_redraw(dev, par, true);
	return _count;
}

static ssize_t sysfs_vcom_offset_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count)
{
	int ret;
	struct fb_info* info		= dev_get_drvdata(_fbdev);
	struct device* dev		= info->device;
	struct da8xx_ili9340_par* par	= info->par;
	unsigned value;

	ret = kstrtouint(_buf, 0, &value);
	if (ret)
		return ret;

	atomic_set(&par->display_settings.disp_vcom_offset, value);

	display_schedule_redraw(dev, par, true);
	return _count;
}

static ssize_t sysfs_gctrl_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count)
{
	int ret;
	struct fb_info* info		= dev_get_drvdata(_fbdev);
	struct device* dev		= info->device;
	struct da8xx_ili9340_par* par	= info->par;
	unsigned value;

	ret = kstrtouint(_buf, 0, &value);
	if (ret)
		return ret;

	atomic_set(&par->display_settings.disp_gctrl, value);

	display_schedule_redraw(dev, par, true);
	return _count;
}



static ssize_t sysfs_vrh_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count)
{
	int ret;
	struct fb_info* info		= dev_get_drvdata(_fbdev);
	struct device* dev		= info->device;
	struct da8xx_ili9340_par* par	= info->par;
	unsigned value;

	ret = kstrtouint(_buf, 0, &value);
	if (ret)
		return ret;

	atomic_set(&par->display_settings.disp_vrh, value);

	display_schedule_redraw(dev, par, true);
	return _count;
}

static ssize_t sysfs_vdv_store(struct device* _fbdev, struct device_attribute* _attr, const char* _buf, size_t _count)
{
	int ret;
	struct fb_info* info		= dev_get_drvdata(_fbdev);
	struct device* dev		= info->device;
	struct da8xx_ili9340_par* par	= info->par;
	unsigned value;

	ret = kstrtouint(_buf, 0, &value);
	if (ret)
		return ret;

	atomic_set(&par->display_settings.disp_vdv, value);

	display_schedule_redraw(dev, par, true);
	return _count;
}


static int da8xx_ili9340_fb_init(struct platform_device* _pdevice, struct da8xx_ili9340_pdata* _pdata)
{
	int ret		= -EINVAL;
	struct device* dev			= &_pdevice->dev;
	struct fb_info* info			= platform_get_drvdata(_pdevice);
	struct da8xx_ili9340_par* par		= info->par;

	dev_dbg(dev, "%s: called\n", __func__);

	info->flags		= FBINFO_DEFAULT | FBINFO_VIRTFB | FBINFO_HWACCEL_NONE;
	info->fix		= da8xx_ili9340_fix_init;
	info->var		= da8xx_ili9340_var_init;
	info->fbops		= &da8xx_ili9340_fbops;
	info->pseudo_palette	= par->pseudo_palette;

	info->var.xres		= _pdata->xres;
	info->var.xres_virtual	= info->var.xres;
	info->var.yres		= _pdata->yres;
	info->var.yres_virtual	= info->var.yres * (info->fix.ypanstep?2:1);
	info->var.height	= _pdata->screen_height;
	info->var.width		= _pdata->screen_width;
	info->var.transp.length	= 0;
	info->var.transp.offset = 0;

	switch (_pdata->visual_mode) {
		case DA8XX_LCDC_VISUAL_565: // RGB565, 16bits
			info->var.red.offset		= 11;
			info->var.red.length		= 5;
			info->var.green.offset		= 5;
			info->var.green.length		= 6;
			info->var.blue.offset		= 0;
			info->var.blue.length		= 5;
			info->var.bits_per_pixel	= 16;
			break;
		case DA8XX_LCDC_VISUAL_8880: // RGB888, 32bits
			info->var.red.offset		= 8;
			info->var.red.length		= 8;
			info->var.green.offset		= 0;
			info->var.green.length		= 8;
			info->var.blue.offset		= 24;
			info->var.blue.length		= 8;
			info->var.bits_per_pixel	= 32;
			break;
		default:
			dev_err(dev, "%s: unsupported visual mode %u\n", __func__, (unsigned)_pdata->visual_mode);
			ret = -EINVAL;
			goto exit;
	}

	par->fb_visual_mode = _pdata->visual_mode;

	info->fix.line_length		= DIV_ROUND_UP(info->var.bits_per_pixel, BITS_PER_BYTE) * info->var.xres_virtual;
	if (ALIGN(info->fix.line_length, DA8XX_LCDCREG_DMA_FBn_BASE__ALIGNMENT) != info->fix.line_length) {
		dev_warn(dev, "%s: framebuffer line is not alignment on LCD controller DMA boundary, y-axis padding disabled\n", __func__);
		info->fix.ypanstep = 0; // disable panning
	}
	info->screen_size		= info->fix.line_length * info->var.yres_virtual;
	par->fb_dma_phsize		= ALIGN(info->screen_size, DA8XX_LCDCREG_DMA_FBn_BASE__ALIGNMENT);
	if (par->fb_dma_phsize != info->screen_size)
		dev_warn(dev, "%s: whole framebuffer is not alignment on LCD controller DMA boundary\n", __func__);

	info->screen_base	= dma_alloc_coherent(NULL, par->fb_dma_phsize, &par->fb_dma_phaddr, GFP_KERNEL);
	if (info->screen_base == NULL) {
		dev_err(dev, "%s: cannot allocate EDMA screen buffer\n", __func__);
		ret = -ENOMEM;
		goto exit;
	}

	memset(info->screen_base, 0, info->screen_size);
	info->fix.smem_start	= virt_to_phys(info->screen_base);
	info->fix.smem_len	= info->screen_size;

	ret = fb_alloc_cmap(&info->cmap, 256, 0);
	if (ret) {
		dev_err(dev, "%s: cannot allocated colormap: %d\n", __func__, ret);
		goto exit_free_dma;
	}

	info->fbdefio		= &da8xx_ili9340_defio;
	info->fbdefio->delay	= (HZ / (_pdata->fps>1?_pdata->fps:1));

	fb_deferred_io_init(info);

	dev_dbg(dev, "%s: done\n", __func__);

	return 0;


 //exit_defio_cleanup:
	fb_deferred_io_cleanup(info);
 //exit_dealloc_cmap:
	fb_dealloc_cmap(&info->cmap);
 exit_free_dma:
	dma_free_coherent(dev, par->fb_dma_phsize, info->screen_base, par->fb_dma_phaddr);
 exit:
	return ret;
}




static void __devinitexit da8xx_ili9340_fb_shutdown(struct platform_device* _pdevice)
{
	struct device* dev			= &_pdevice->dev;
	struct fb_info* info			= platform_get_drvdata(_pdevice);
	struct da8xx_ili9340_par* par		= info->par;

	dev_dbg(dev, "%s: called\n", __func__);

	fb_deferred_io_cleanup(info);
	fb_dealloc_cmap(&info->cmap);
	dma_free_coherent(dev, par->fb_dma_phsize, info->screen_base, par->fb_dma_phaddr);

	dev_dbg(dev, "%s: done\n", __func__);
}




static int da8xx_ili9340_lidd_regs_init(struct platform_device* _pdevice, struct da8xx_ili9340_pdata* _pdata)
{
	int ret					= -EINVAL;
	struct device* dev			= &_pdevice->dev;
	struct fb_info* info			= platform_get_drvdata(_pdevice);
	struct da8xx_ili9340_par* par		= info->par;

	__u32 regval;
	__u32 revid;

	unsigned long lcdc_clk_khz;
	unsigned long lcdc_mclk_ns;
	__u32 lcdc_mclk_div;

	bool lidd_is_async;
	__u32 lidd_ctrl_mode;
	__u32 lidd_dma_cs;
	__u32 lidd_ctrl_ale_pol;
	__u32 lidd_ctrl_rs_en_pol;
	__u32 lidd_ctrl_ws_dir_pol;
	__u32 lidd_ctrl_cs0_e0_pol;
	__u32 lidd_ctrl_cs1_e1_pol;
	__u32 lidd_dma_burst_size;
	unsigned lidd_reg_ovf = 0;

	dev_dbg(dev, "%s: called\n", __func__);

	ret = lcdc_lock(par);
	if (ret) {
		dev_err(dev, "%s: cannot obtain LCD controller lock: %d\n", __func__, ret);
		goto exit;
	}
	regval = lcdc_reg_read(dev, par, DA8XX_LCDCREG_REVID);
	lcdc_unlock(par);

	revid = REGDEF_GET_VALUE(DA8XX_LCDCREG_REVID__REV, regval);
	if (revid != DA8XX_LCDCREG_REVID__REV__id) {
		dev_err(dev, "%s: detected wrong LCD controller REVID %x, expected %x\n", __func__, (unsigned)revid, (unsigned)DA8XX_LCDCREG_REVID__REV__id);
		ret = -EINVAL;
		goto exit;
	}

	lcdc_clk_khz	= clk_get_rate(par->lcdc_clk)/1000; // khz to avoid potential overflow
	lcdc_mclk_div	= DIV_ROUND_UP(lcdc_clk_khz * _pdata->lcdc_lidd_mclk_ns, USEC_PER_SEC);
	if (lcdc_mclk_div == 0)
		lcdc_mclk_div = 1;
	lcdc_mclk_ns = DIV_ROUND_UP(USEC_PER_SEC, lcdc_clk_khz * lcdc_mclk_div);
	if (lcdc_mclk_ns == 0)
		lcdc_mclk_ns = 1;
        dev_dbg(dev, "%s: configuring LCD controller MCLK div %lu, MCLK will be %uns with %uns configured\n",
			__func__, (unsigned long)lcdc_mclk_div, (unsigned)lcdc_mclk_ns, (unsigned)_pdata->lcdc_lidd_mclk_ns);

	switch (_pdata->lcdc_lidd_mode) {
		case DA8XX_LCDC_LIDD_MODE_6800SYNC:
			lidd_ctrl_mode = DA8XX_LCDCREG_LIDD_CTRL__MODE_SEL__6800sync;
			lidd_is_async  = false;
			break;
		case DA8XX_LCDC_LIDD_MODE_6800ASYNC:
			lidd_ctrl_mode = DA8XX_LCDCREG_LIDD_CTRL__MODE_SEL__6800async;
			lidd_is_async  = true;
			break;
		case DA8XX_LCDC_LIDD_MODE_8080SYNC:
			lidd_ctrl_mode = DA8XX_LCDCREG_LIDD_CTRL__MODE_SEL__8080sync;
			lidd_is_async  = false;
			break;
		case DA8XX_LCDC_LIDD_MODE_8080ASYNC:
			lidd_ctrl_mode = DA8XX_LCDCREG_LIDD_CTRL__MODE_SEL__8080async;
			lidd_is_async  = true;
			break;
		default:
			dev_err(dev, "%s: unsupported LCD controller LIDD mode %d\n", __func__, (int)_pdata->lcdc_lidd_mode);
			ret = -EINVAL;
			goto exit;
	}

	if (lidd_is_async) {
		switch (_pdata->lcdc_lidd_cs) {
			case DA8XX_LCDC_LIDD_CS0:
				par->lidd_reg_cs_conf	= DA8XX_LCDCREG_LIDD_CS0_CONF;
				par->lidd_reg_cs_addr	= DA8XX_LCDCREG_LIDD_CS0_ADDR;
				par->lidd_reg_cs_data	= DA8XX_LCDCREG_LIDD_CS0_DATA;
				lidd_dma_cs		= DA8XX_LCDCREG_LIDD_CTRL__DMA_CS0_CS1__cs0;
				dev_dbg(dev, "%s: using CS0\n", __func__);
				break;
			case DA8XX_LCDC_LIDD_CS1:
				par->lidd_reg_cs_conf	= DA8XX_LCDCREG_LIDD_CS1_CONF;
				par->lidd_reg_cs_addr	= DA8XX_LCDCREG_LIDD_CS1_ADDR;
				par->lidd_reg_cs_data	= DA8XX_LCDCREG_LIDD_CS1_DATA;
				lidd_dma_cs		= DA8XX_LCDCREG_LIDD_CTRL__DMA_CS0_CS1__cs1;
				dev_dbg(dev, "%s: using CS1\n", __func__);
				break;
			default:
				dev_err(dev, "%s: unsupported LCD controller chip-select %d\n", __func__, (int)_pdata->lcdc_lidd_cs);
				ret = -EINVAL;
				goto exit;
		}
	} else {
		if (_pdata->lcdc_lidd_cs != DA8XX_LCDC_LIDD_CS0)
			dev_warn(dev, "%s: ignoring CS %d in sync mode, enforcing CS0\n", __func__, (int)_pdata->lcdc_lidd_cs);
		par->lidd_reg_cs_conf	= DA8XX_LCDCREG_LIDD_CS0_CONF;
		par->lidd_reg_cs_addr	= DA8XX_LCDCREG_LIDD_CS0_ADDR;
		par->lidd_reg_cs_data	= DA8XX_LCDCREG_LIDD_CS0_DATA;
		lidd_dma_cs		= DA8XX_LCDCREG_LIDD_CTRL__DMA_CS0_CS1__cs0;
		dev_dbg(dev, "%s: enforcing CS0\n", __func__);
	}

	switch (_pdata->lcdc_lidd_ale_pol) {
		case DA8XX_LCDC_LIDD_POL_DEFAULT:
		case DA8XX_LCDC_LIDD_POL_ACTIVE_LOW:	lidd_ctrl_ale_pol = DA8XX_LCDCREG_LIDD_CTRL__xxxPOL__norm; break;
		case DA8XX_LCDC_LIDD_POL_ACTIVE_HIGH:	lidd_ctrl_ale_pol = DA8XX_LCDCREG_LIDD_CTRL__xxxPOL__inv; break;
		default:
			dev_err(dev, "%s: unsupported LCD controller ALE polarity %d\n", __func__, (int)_pdata->lcdc_lidd_ale_pol);
			ret = -EINVAL;
			goto exit;
	}

	switch (_pdata->lcdc_lidd_rs_en_pol) {
		case DA8XX_LCDC_LIDD_POL_DEFAULT:
		case DA8XX_LCDC_LIDD_POL_ACTIVE_LOW:	lidd_ctrl_rs_en_pol = DA8XX_LCDCREG_LIDD_CTRL__xxxPOL__norm; break;
		case DA8XX_LCDC_LIDD_POL_ACTIVE_HIGH:	lidd_ctrl_rs_en_pol = DA8XX_LCDCREG_LIDD_CTRL__xxxPOL__inv; break;
		default:
			dev_err(dev, "%s: unsupported LCD controller RS/EN polarity %d\n", __func__, (int)_pdata->lcdc_lidd_rs_en_pol);
			ret = -EINVAL;
			goto exit;
	}

	switch (_pdata->lcdc_lidd_ws_dir_pol) {
		case DA8XX_LCDC_LIDD_POL_DEFAULT:
		case DA8XX_LCDC_LIDD_POL_ACTIVE_LOW:	lidd_ctrl_ws_dir_pol = DA8XX_LCDCREG_LIDD_CTRL__xxxPOL__norm; break;
		case DA8XX_LCDC_LIDD_POL_ACTIVE_HIGH:	lidd_ctrl_ws_dir_pol = DA8XX_LCDCREG_LIDD_CTRL__xxxPOL__inv; break;
		default:
			dev_err(dev, "%s: unsupported LCD controller WS/DIR polarity %d\n", __func__, (int)_pdata->lcdc_lidd_ws_dir_pol);
			ret = -EINVAL;
			goto exit;
	}

	switch (_pdata->lcdc_lidd_cs_pol) {
		case DA8XX_LCDC_LIDD_POL_DEFAULT:	lidd_ctrl_cs0_e0_pol = DA8XX_LCDCREG_LIDD_CTRL__xxxPOL__norm;
							lidd_ctrl_cs1_e1_pol = DA8XX_LCDCREG_LIDD_CTRL__xxxPOL__norm;	break;
		case DA8XX_LCDC_LIDD_POL_ACTIVE_LOW:	lidd_ctrl_cs0_e0_pol = DA8XX_LCDCREG_LIDD_CTRL__xxxPOL__norm;
							lidd_ctrl_cs1_e1_pol = DA8XX_LCDCREG_LIDD_CTRL__xxxPOL__inv;	break;
		case DA8XX_LCDC_LIDD_POL_ACTIVE_HIGH:	lidd_ctrl_cs0_e0_pol = DA8XX_LCDCREG_LIDD_CTRL__xxxPOL__inv;
							lidd_ctrl_cs1_e1_pol = DA8XX_LCDCREG_LIDD_CTRL__xxxPOL__norm;	break;
		default:
			dev_err(dev, "%s: unsupported LCD controller CS polarity %d\n", __func__, (int)_pdata->lcdc_lidd_cs_pol);
			ret = -EINVAL;
			goto exit;
	}

	if (_pdata->lcdc_lidd_edma_burst < 2)
		lidd_dma_burst_size	= DA8XX_LCDCREG_DMA_CTRL__BURST_SIZE__1;
	else if (_pdata->lcdc_lidd_edma_burst < 4)
		lidd_dma_burst_size	= DA8XX_LCDCREG_DMA_CTRL__BURST_SIZE__2;
	else if (_pdata->lcdc_lidd_edma_burst < 8)
		lidd_dma_burst_size	= DA8XX_LCDCREG_DMA_CTRL__BURST_SIZE__4;
	else if (_pdata->lcdc_lidd_edma_burst < 16)
		lidd_dma_burst_size	= DA8XX_LCDCREG_DMA_CTRL__BURST_SIZE__8;
	else
		lidd_dma_burst_size	= DA8XX_LCDCREG_DMA_CTRL__BURST_SIZE__16;

	dev_dbg(dev, "%s: applying registers\n", __func__);

	ret = lcdc_lock(par);
	if (ret) {
		dev_err(dev, "%s: cannot obtain LCD controller lock: %d\n", __func__, ret);
		goto exit;
	}

	regval = 0;
	regval |= REGDEF_SET_VALUE(DA8XX_LCDCREG_LCD_CTRL__MODESEL,	DA8XX_LCDCREG_LCD_CTRL__MODESEL__lidd);
	regval |= REGDEF_SET_VALUE_OVF(DA8XX_LCDCREG_LCD_CTRL__CLKDIV,	lcdc_mclk_div, lidd_reg_ovf);
	if (lidd_reg_ovf) {
		dev_warn(dev, "%s: noticed LCD controller MCLK div overflow\n", __func__);
		lidd_reg_ovf = 0;
	}
	lcdc_reg_write(dev, par, DA8XX_LCDCREG_LCD_CTRL, regval);

	lcdc_reg_write(dev, par, DA8XX_LCDCREG_LIDD_CTRL,
			0
			| REGDEF_SET_VALUE(DA8XX_LCDCREG_LIDD_CTRL__MODE_SEL,		lidd_ctrl_mode)
			| REGDEF_SET_VALUE(DA8XX_LCDCREG_LIDD_CTRL__ALEPOL,		lidd_ctrl_ale_pol)
			| REGDEF_SET_VALUE(DA8XX_LCDCREG_LIDD_CTRL__RS_EN_POL,		lidd_ctrl_rs_en_pol)
			| REGDEF_SET_VALUE(DA8XX_LCDCREG_LIDD_CTRL__WS_DIR_POL,		lidd_ctrl_ws_dir_pol)
			| REGDEF_SET_VALUE(DA8XX_LCDCREG_LIDD_CTRL__CS0_E0_POL,		lidd_ctrl_cs0_e0_pol)
			| REGDEF_SET_VALUE(DA8XX_LCDCREG_LIDD_CTRL__CS1_E1_POL,		lidd_ctrl_cs1_e1_pol)
			| REGDEF_SET_VALUE(DA8XX_LCDCREG_LIDD_CTRL__LIDD_DMA_EN,	DA8XX_LCDCREG_LIDD_CTRL__LIDD_DMA_EN__deactivate)
			| REGDEF_SET_VALUE(DA8XX_LCDCREG_LIDD_CTRL__DMA_CS0_CS1,	lidd_dma_cs)
			| REGDEF_SET_VALUE(DA8XX_LCDCREG_LIDD_CTRL__DONE_INT_EN,	DA8XX_LCDCREG_LIDD_CTRL__DONE_INT_EN__enable));

	regval = 0;
	regval |= REGDEF_SET_VALUE_OVF(DA8XX_LCDCREG_LIDD_CSn_CONF__TA,		DIV_ROUND_UP(_pdata->lcdc_t_ta_ns,	lcdc_mclk_ns), lidd_reg_ovf);
	regval |= REGDEF_SET_VALUE_OVF(DA8XX_LCDCREG_LIDD_CSn_CONF__R_HOLD,	DIV_ROUND_UP(_pdata->lcdc_t_rhold_ns,	lcdc_mclk_ns), lidd_reg_ovf);
	regval |= REGDEF_SET_VALUE_OVF(DA8XX_LCDCREG_LIDD_CSn_CONF__R_STROBE,	DIV_ROUND_UP(_pdata->lcdc_t_rstrobe_ns,	lcdc_mclk_ns), lidd_reg_ovf);
	regval |= REGDEF_SET_VALUE_OVF(DA8XX_LCDCREG_LIDD_CSn_CONF__R_SU,	DIV_ROUND_UP(_pdata->lcdc_t_rsu_ns,	lcdc_mclk_ns), lidd_reg_ovf);
	regval |= REGDEF_SET_VALUE_OVF(DA8XX_LCDCREG_LIDD_CSn_CONF__W_HOLD,	DIV_ROUND_UP(_pdata->lcdc_t_whold_ns,	lcdc_mclk_ns), lidd_reg_ovf);
	regval |= REGDEF_SET_VALUE_OVF(DA8XX_LCDCREG_LIDD_CSn_CONF__W_STROBE,	DIV_ROUND_UP(_pdata->lcdc_t_wstrobe_ns,	lcdc_mclk_ns), lidd_reg_ovf);
	regval |= REGDEF_SET_VALUE_OVF(DA8XX_LCDCREG_LIDD_CSn_CONF__W_SU,	DIV_ROUND_UP(_pdata->lcdc_t_wsu_ns,	lcdc_mclk_ns), lidd_reg_ovf);
	if (lidd_reg_ovf) {
		dev_warn(dev, "%s: noticed LCD controller timings overflow\n", __func__);
		lidd_reg_ovf = 0;
	}
	lcdc_reg_write(dev, par, par->lidd_reg_cs_conf, regval);

	lcdc_reg_write(dev, par, DA8XX_LCDCREG_DMA_CTRL,
			0
			| REGDEF_SET_VALUE(DA8XX_LCDCREG_DMA_CTRL__FRAME_MODE,	DA8XX_LCDCREG_DMA_CTRL__FRAME_MODE__single)
			| REGDEF_SET_VALUE(DA8XX_LCDCREG_DMA_CTRL__BIGENDIAN,	DA8XX_LCDCREG_DMA_CTRL__BIGENDIAN__disable)
			| REGDEF_SET_VALUE(DA8XX_LCDCREG_DMA_CTRL__EOF_INT_EN,	DA8XX_LCDCREG_DMA_CTRL__EOF_INT_EN__disable)
			| REGDEF_SET_VALUE(DA8XX_LCDCREG_DMA_CTRL__BURST_SIZE,	lidd_dma_burst_size));

	regval = lcdc_reg_read(dev, par, DA8XX_LCDCREG_LCD_STAT);
	if (regval) {
		dev_warn(dev, "%s: non-zero LCD_STAT value %x at initialization\n", __func__, (unsigned)regval);
		lcdc_reg_write(dev, par, DA8XX_LCDCREG_LCD_STAT, regval);
	}

	lcdc_unlock(par);

	dev_dbg(dev, "%s: done\n", __func__);

	return 0;


 //exit_unlock:
	lcdc_unlock(par);
 exit:
	return ret;
}

static void __devinitexit da8xx_ili9340_lidd_regs_shutdown(struct platform_device* _pdevice)
{
	int ret					= -EINVAL;
	struct device* dev			= &_pdevice->dev;
	struct fb_info* info			= platform_get_drvdata(_pdevice);
	struct da8xx_ili9340_par* par		= info->par;

	dev_dbg(dev, "%s: called\n", __func__);

	ret = lcdc_lock(par);
	if (ret) {
		dev_err(dev, "%s: cannot obtain LCD controller lock: %d\n", __func__, ret);
		goto exit;
	}

	lcdc_reg_change(dev, par, DA8XX_LCDCREG_LIDD_CTRL,
			0
			| REGDEF_MASK(DA8XX_LCDCREG_LIDD_CTRL__LIDD_DMA_EN)
			| REGDEF_MASK(DA8XX_LCDCREG_LIDD_CTRL__DONE_INT_EN),
			0
			| REGDEF_SET_VALUE(DA8XX_LCDCREG_LIDD_CTRL__LIDD_DMA_EN,	DA8XX_LCDCREG_LIDD_CTRL__LIDD_DMA_EN__deactivate)
			| REGDEF_SET_VALUE(DA8XX_LCDCREG_LIDD_CTRL__DONE_INT_EN,	DA8XX_LCDCREG_LIDD_CTRL__DONE_INT_EN__disable));

	lcdc_unlock(par);

	dev_dbg(dev, "%s: done\n", __func__);

	return;


 //exit_unlock:
	lcdc_unlock(par);
 exit:
	return;
}




static int da8xx_ili9340_lcdc_init(struct platform_device* _pdevice, struct da8xx_ili9340_pdata* _pdata)
{
	int ret					= -EINVAL;
	struct device* dev			= &_pdevice->dev;
	struct fb_info* info			= platform_get_drvdata(_pdevice);
	struct da8xx_ili9340_par* par		= info->par;
	struct resource* res_ptr;

	dev_dbg(dev, "%s: called\n", __func__);

	res_ptr = da850_trik_lcdc_resources; //  platform_get_resource(_pdevice, IORESOURCE_MEM, 0);
	if (res_ptr == NULL) {
		dev_err(dev, "%s: no memory resource specified\n", __func__);
		ret = -ENXIO;
		goto exit;
	}

	par->lcdc_reg_size	= resource_size(res_ptr);
	par->lcdc_reg_start	= res_ptr->start;
	res_ptr = request_mem_region(par->lcdc_reg_start, par->lcdc_reg_size, _pdevice->name);
	if (res_ptr == NULL) {
		dev_err(dev, "%s: cannot reserve LCD controller registers memory\n", __func__);
		ret = -ENXIO;
		goto exit;
	}

	par->lcdc_reg_base	= ioremap(par->lcdc_reg_start, par->lcdc_reg_size);
	if (par->lcdc_reg_base == NULL) {
		dev_err(dev, "%s: cannot ioremap LCD controller registers memory region\n", __func__);
		ret = -ENXIO;
		goto exit_release_lcdc_reg;
	}

	res_ptr = da850_trik_lcdc_resources + 1; //   platform_get_resource(_pdevice, IORESOURCE_IRQ, 0);
	if (res_ptr == NULL) {
		dev_err(dev, "%s: no IRQ resource specified\n", __func__);
		ret = -ENXIO;
		goto exit_iounmap_lcdc_reg;
	}

	par->lcdc_irq = res_ptr->start;
	ret = request_irq(par->lcdc_irq, &lcdc_edma_done, 0, DRIVER_NAME, dev);
	if (ret) {
		dev_err(dev, "%s: cannot request LCD controller EDMA completion IRQ: %d\n", __func__, ret);
		goto exit_iounmap_lcdc_reg;
	}

	ret = irq_set_irq_type(par->lcdc_irq, IRQ_TYPE_EDGE_RISING);
	if (ret) {
		dev_err(dev, "%s: cannot set LCD controller EDMA completion IRQ type: %d\n", __func__, ret);
		goto exit_free_lcdc_irq;
	}

	par->lcdc_clk = clk_get(NULL, "my_lcdc_fck"); //   devm_clk_get(dev, "fck");
	if (IS_ERR(par->lcdc_clk)) {
		dev_err(dev, "%s: cannot get LCD controller clock\n", __func__);
		ret = -ENODEV;
		goto exit_free_lcdc_irq;
	}

	ret = clk_prepare_enable(par->lcdc_clk);
	if (ret) {
		dev_err(dev, "%s: cannot enable LCD controller clock\n", __func__);
		goto exit_put_lcdc_clk;
	}

	sema_init(&par->lcdc_semaphore, 1);

	ret = da8xx_ili9340_lidd_regs_init(_pdevice, _pdata);
	if (ret) {
		dev_err(dev, "%s: cannot setup LCD controller LIDD registers: %d\n", __func__, ret);
		goto exit_disable_lcdc_clk;
	}

	dev_dbg(dev, "%s: done\n", __func__);

	return 0;


 //exit_shutdown_lidd_regs:
	da8xx_ili9340_lidd_regs_shutdown(_pdevice);
 exit_disable_lcdc_clk:
	clk_disable_unprepare(par->lcdc_clk);
 exit_put_lcdc_clk:
	devm_clk_put(dev, par->lcdc_clk);
 exit_free_lcdc_irq:
	free_irq(par->lcdc_irq, dev);
 exit_iounmap_lcdc_reg:
	iounmap(par->lcdc_reg_base);
 exit_release_lcdc_reg:
	release_mem_region(par->lcdc_reg_start, par->lcdc_reg_size);
 exit:
	return ret;
}

static void __devinitexit da8xx_ili9340_lcdc_shutdown(struct platform_device* _pdevice)
{
	struct device* dev			= &_pdevice->dev;
	struct fb_info* info			= platform_get_drvdata(_pdevice);
	struct da8xx_ili9340_par* par		= info->par;

	dev_dbg(dev, "%s: called\n", __func__);

	da8xx_ili9340_lidd_regs_shutdown(_pdevice);
	BUG_ON(down_trylock(&par->lcdc_semaphore) != 0);
	clk_disable_unprepare(par->lcdc_clk);
	devm_clk_put(dev, par->lcdc_clk);
	free_irq(par->lcdc_irq, dev);
	iounmap(par->lcdc_reg_base);
	release_mem_region(par->lcdc_reg_start, par->lcdc_reg_size);

	dev_dbg(dev, "%s: done\n", __func__);
}




static int da8xx_ili9340_display_init(struct platform_device* _pdevice, struct da8xx_ili9340_pdata* _pdata)
{
	int ret					= -EINVAL;
	struct device* dev			= &_pdevice->dev;
	struct fb_info* info			= platform_get_drvdata(_pdevice);
	struct da8xx_ili9340_par* par		= info->par;
	unsigned ovf = 0;
	__u16 disp_mfc, disp_ver, disp_id;
	__u16 disp_self_diag1, disp_self_diag2;
	__u16 disp_dbi, disp_mdt;
	__u32 disp_columns, disp_rows;

	dev_dbg(dev, "%s: called\n", __func__);

	par->cb_power_ctrl	= _pdata->cb_power_ctrl;
	par->cb_backlight_ctrl	= _pdata->cb_backlight_ctrl;

	INIT_DELAYED_WORK(&par->display_redraw_work, &display_redraw_work);
	atomic_set(&par->display_redraw_requested,	0);
	atomic_set(&par->display_redraw_ongoing,	0);
	init_waitqueue_head(&par->display_redraw_completion);

	par->display_swapxy	= _pdata->xyswap;

	atomic_set(&par->display_settings.changed,		1);
	atomic_set(&par->display_settings.disp_on,		1);
	atomic_set(&par->display_settings.disp_idle,		_pdata->display_idle?1:0);
	atomic_set(&par->display_settings.disp_inversion,	_pdata->display_inversion?1:0);
	switch (_pdata->display_gamma) {
		case DA8XX_LCDC_DISPLAY_GAMMA_1_0:
			atomic_set(&par->display_settings.disp_gamma,
					REGDEF_SET_VALUE(ILI9340_DISPLAY_CFG_GAMMA, ILI9340_DISPLAY_CFG_GAMMA__1_0));
			break;
		case DA8XX_LCDC_DISPLAY_GAMMA_1_8:
			atomic_set(&par->display_settings.disp_gamma,
					REGDEF_SET_VALUE(ILI9340_DISPLAY_CFG_GAMMA, ILI9340_DISPLAY_CFG_GAMMA__1_8));
			break;
		case DA8XX_LCDC_DISPLAY_GAMMA_DEFAULT:
		case DA8XX_LCDC_DISPLAY_GAMMA_2_2:
			atomic_set(&par->display_settings.disp_gamma,
					REGDEF_SET_VALUE(ILI9340_DISPLAY_CFG_GAMMA, ILI9340_DISPLAY_CFG_GAMMA__2_2));
			break;
		case DA8XX_LCDC_DISPLAY_GAMMA_2_5:
			atomic_set(&par->display_settings.disp_gamma,
					REGDEF_SET_VALUE(ILI9340_DISPLAY_CFG_GAMMA, ILI9340_DISPLAY_CFG_GAMMA__2_5));
			break;
		default:
			dev_err(dev, "%s: unknown gamma value %u\n", __func__, (unsigned)_pdata->display_gamma);
			atomic_set(&par->display_settings.disp_gamma,
					REGDEF_SET_VALUE(ILI9340_DISPLAY_CFG_GAMMA, ILI9340_DISPLAY_CFG_GAMMA__2_2));
			break;
	}
	atomic_set(&par->display_settings.disp_flip,		_pdata->xflip?REGDEF_SET_VALUE(ILI9340_DISPLAY_CFG_FLIP_X, 1):0x0
							       |_pdata->yflip?REGDEF_SET_VALUE(ILI9340_DISPLAY_CFG_FLIP_Y, 1):0x0);
	atomic_set(&par->display_settings.disp_brightness,	REGDEF_SET_VALUE_OVF(ILI9340_DISPLAY_CFG_BRIGHTNESS, _pdata->display_brightness, ovf));
	atomic_set(&par->display_settings.disp_backlight,	_pdata->display_backlight?1:0);

	par->ili9340_t_reset_to_ready_ms	= _pdata->display_t_reset_to_ready_ms;
	par->ili9340_t_sleep_in_out_ms		= _pdata->display_t_sleep_in_out_ms;

	ret = lcdc_lock(par);
	if (ret) {
		dev_err(dev, "%s: cannot obtain LCD controller lock: %d\n", __func__, ret);
		goto exit;
	}


#warning TODO consider making this code asynchronous. Read note below
	/*
	 * Code below may be started as a long-running work (queue_work(system_long_wq)) for faster startup.
	 * It takes about 300ms for a normal startup due to 2*120ms delays.
	 * Code below initializes display power and registers and then start redraw work (display_start_redraw_locked).
	 *
	 * At the same time this code checks if startup succeed and only registers framebuffer in this case.
	 * Unfortunately, there is no good mechanism to 'abort' startup - if we wait for a startup completion,
	 * then there is no speed up; if we don't, we have to 'unroll' framebuffer registration later on.
	 *
	 * To make this code asynchronous, a new work_struct should be added to par; initialization and flushing code
	 * should be added to near to initialization and flushing code for redraw work. Note that flushing should
	 * flush startup work and then redraw work.
	 * To block framebuffer access while startup is running, startup work must run from locked context (i.e. here
	 * is a nice place) with display_settings.changed and display_redraw_ongoing set (to block any redraw work).
	 * At the end of startup work it might be reasonable to run lcdc_edma_start from startup work itself, without
	 * scheduling redraw work - this way it will use redraw work's finalization code to re-run redraw if necessary.
	 */

	par->cb_power_ctrl(1);
	msleep(par->ili9340_t_reset_to_ready_ms); // Reset/power on to registers access delay, ch13.3.1, pg216

	// Checking display ID
	display_write_cmd(dev, par, ILI9340_CMD_READID);
	/*ignore*/	display_read_data(dev, par);
	disp_mfc	= display_read_data(dev, par);
	disp_ver	= display_read_data(dev, par);
	disp_id		= display_read_data(dev, par);
	pr_info(NICE_NAME ": detected display: manufacturer 0x%02x, version 0x%02x, id 0x%02x\n",
			(unsigned)disp_mfc, (unsigned)disp_ver, (unsigned)disp_id);
	par->disp_id = (unsigned)disp_mfc << 16 | (unsigned) disp_ver << 8 | disp_id;

	// Sleep-out
	display_write_cmd(dev, par, ILI9340_CMD_READ_SELFDIAG);
	/*ignore*/	display_read_data(dev, par);
	disp_self_diag1	= display_read_data(dev, par);

	display_write_cmd(dev, par, ILI9340_CMD_SLEEP_OUT);
	msleep(par->ili9340_t_sleep_in_out_ms); // Sleep-out to self diagnostics delay, ch11.2, pg210

	display_write_cmd(dev, par, ILI9340_CMD_READ_SELFDIAG);
	/*ignore*/	display_read_data(dev, par);
	disp_self_diag2	= display_read_data(dev, par);

	if (REGDEF_GET_VALUE(ILI9340_CMD_READ_SELFDIAG__RLD, disp_self_diag1) ==
		REGDEF_GET_VALUE(ILI9340_CMD_READ_SELFDIAG__RLD, disp_self_diag2)) {
		dev_err(dev, "%s: ILI9340 reports register loading failure\n", __func__);
		ret = -EBUSY;
		goto exit_sleep_in;
	}
	if (REGDEF_GET_VALUE(ILI9340_CMD_READ_SELFDIAG__FD, disp_self_diag1) ==
		REGDEF_GET_VALUE(ILI9340_CMD_READ_SELFDIAG__FD, disp_self_diag2)) {
		dev_err(dev, "%s: ILI9340 reports functionality failure\n", __func__);
		ret = -EBUSY;
		goto exit_sleep_in;
	}

	disp_columns	= info->var.xres;
	disp_rows	= info->var.yres;

	display_write_cmd(dev, par, ILI9340_CMD_COLUMN_ADDR);
	display_write_data(dev, par, REGDEF_SET_VALUE(ILI9340_CMD_COLUMN_ADDR__HIGHBYTE,	0));
	display_write_data(dev, par, REGDEF_SET_VALUE(ILI9340_CMD_COLUMN_ADDR__LOWBYTE,		0));
	display_write_data(dev, par, REGDEF_SET_VALUE(ILI9340_CMD_COLUMN_ADDR__HIGHBYTE,	disp_columns>>8));
	display_write_data(dev, par, REGDEF_SET_VALUE(ILI9340_CMD_COLUMN_ADDR__LOWBYTE,		disp_columns));

	display_write_cmd(dev, par, ILI9340_CMD_ROW_ADDR);
	display_write_data(dev, par, REGDEF_SET_VALUE(ILI9340_CMD_ROW_ADDR__HIGHBYTE,		0));
	display_write_data(dev, par, REGDEF_SET_VALUE(ILI9340_CMD_ROW_ADDR__LOWBYTE,		0));
	display_write_data(dev, par, REGDEF_SET_VALUE(ILI9340_CMD_ROW_ADDR__HIGHBYTE,		disp_rows>>8));
	display_write_data(dev, par, REGDEF_SET_VALUE(ILI9340_CMD_ROW_ADDR__LOWBYTE,		disp_rows));

	switch (par->fb_visual_mode) {
		case DA8XX_LCDC_VISUAL_565:	disp_dbi = 0x5;	disp_mdt = 0x0;	break;
		case DA8XX_LCDC_VISUAL_8880:	disp_dbi = 0x6;	disp_mdt = 0x1;	break;
		default:
			dev_err(dev, "%s: unsupported visual mode\n", __func__);
			ret = -EINVAL;
			goto exit_sleep_in;
	}
	display_write_cmd(dev, par, ILI9340_CMD_PIXEL_FORMAT);
	display_write_data(dev, par, REGDEF_SET_VALUE(ILI9340_CMD_PIXEL_FORMAT__DBI, disp_dbi));
        display_write_cmd(dev, par, ILI9340_CMD_IFACE_CTRL);
	
	atomic_set(&par->display_settings.disp_vcom, 0x2b); //0xbb
	atomic_set(&par->display_settings.disp_vcom_offset, 0x20); //0xc5
	
	atomic_set(&par->display_settings.disp_vrh, 0x11);  //0xc3, default is 0x0b, but experiments show 0x20 looks nice
	atomic_set(&par->display_settings.disp_vdv, 0x20);  //0xc4, default is 0x20
	atomic_set(&par->display_settings.disp_gctrl, 0x77);  //0xb7 default is 0x35

	display_write_cmd(dev, par, 0xb0); 
	display_write_data(dev, par, REGDEF_SET_VALUE(ILI9340_CMD_IFACE_CTRL__WEMODE, 0x0)); // ignore extra data
	display_write_data(dev, par, 0xc0 | REGDEF_SET_VALUE(ILI9340_CMD_IFACE_CTRL__MDT, disp_mdt)
					| REGDEF_SET_VALUE(ILI9340_CMD_IFACE_CTRL__EPF, 0x0)); // in 565 mode, lowest bit is populated with topmost


	display_start_redraw_locked(dev, par);
	// forget about lock from this point on

	dev_dbg(dev, "%s: done\n", __func__);

	return 0;


 //exit_display_off:
	atomic_set(&par->display_settings.disp_on, 0);
	display_visibility_settings_update(dev, par);
 exit_sleep_in:
	display_write_cmd(dev, par, ILI9340_CMD_SLEEP_IN);
	msleep(par->ili9340_t_sleep_in_out_ms); // Sleep-in to power off delay, ch12, pg211
 //exit_power_off:
	par->cb_power_ctrl(0);
 //exit_unlock:
	lcdc_unlock(par);
 exit:
	return ret;
}

static void __devinitexit da8xx_ili9340_display_shutdown(struct platform_device* _pdevice)
{
	int ret					= -EINVAL;
	struct device* dev			= &_pdevice->dev;
	struct fb_info* info			= platform_get_drvdata(_pdevice);
	struct da8xx_ili9340_par* par		= info->par;

	dev_dbg(dev, "%s: called\n", __func__);

	// Shutdown routine may be synchronous since it must wait for real shutdown completion
	display_wait_redraw_completion(dev, par);
	ret = lcdc_lock(par);
	if (ret) {
		dev_err(dev, "%s: cannot obtain LCD controller lock: %d\n", __func__, ret);
		goto exit;
	}

	atomic_set(&par->display_settings.disp_on, 0);
	display_visibility_settings_update(dev, par);

	display_write_cmd(dev, par, ILI9340_CMD_SLEEP_IN);
	msleep(par->ili9340_t_sleep_in_out_ms); // Sleep-in to power off delay, ch12, pg211

	par->cb_power_ctrl(0);
	lcdc_unlock(par);

	cancel_delayed_work_sync(&par->display_redraw_work);
	BUG_ON(atomic_read(&par->display_redraw_requested) != 0);
	BUG_ON(atomic_read(&par->display_redraw_ongoing) != 0);

	dev_dbg(dev, "%s: done\n", __func__);

	return;

 //exit_unlock:
	lcdc_unlock(par);
 exit:
	return;
}




static int da8xx_ili9340_sysfs_register(struct platform_device* _pdevice, struct da8xx_ili9340_pdata* _pdata)
{
	int ret					= -EINVAL;
	struct device* dev			= &_pdevice->dev;
	struct fb_info* info			= platform_get_drvdata(_pdevice);
	struct da8xx_ili9340_par* par		= info->par;
	int sysfs_entry = 0; // not unsigned to allow simplier unrolling

	dev_dbg(dev, "%s: called\n", __func__);

	par->perf_count = 100;

	for (sysfs_entry = 0; sysfs_entry < ARRAY_SIZE(da8xx_ili9340_sysfs_attrs); ++sysfs_entry) {
		ret = device_create_file(info->dev, &da8xx_ili9340_sysfs_attrs[sysfs_entry]);
		if (ret) {
			dev_err(dev, "%s: cannot register sysfs entry %s: %d\n",
				__func__, da8xx_ili9340_sysfs_attrs[sysfs_entry].attr.name, ret);
			goto exit_remove_file;
		}
	}

	dev_dbg(dev, "%s: done\n", __func__);

	return 0;


 exit_remove_file:
	while (--sysfs_entry >= 0)
		device_remove_file(info->dev, &da8xx_ili9340_sysfs_attrs[sysfs_entry]);
 //exit:
	return ret;
}

static void __devinitexit da8xx_ili9340_sysfs_unregister(struct platform_device* _pdevice)
{
	struct device* dev			= &_pdevice->dev;
	struct fb_info* info			= platform_get_drvdata(_pdevice);
	int sysfs_entry = 0; // not unsigned to allow simplier unrolling

	dev_dbg(dev, "%s: called\n", __func__);

	for (sysfs_entry = ARRAY_SIZE(da8xx_ili9340_sysfs_attrs)-1; sysfs_entry >= 0; --sysfs_entry)
		device_remove_file(info->dev, &da8xx_ili9340_sysfs_attrs[sysfs_entry]);

	dev_dbg(dev, "%s: done\n", __func__);
}




static int da8xx_ili9340_probe(struct platform_device* _pdevice)
{
	int ret				= -EINVAL;
	struct device* dev		= &_pdevice->dev;
	struct da8xx_ili9340_pdata* pdata = &da850_trik_lcdc_pdata; //   _pdevice->dev.platform_data;
	struct da8xx_ili9340_par* par	= NULL;
	struct fb_info* info		= NULL;

	dev_dbg(dev, "%s: called, pdevice %s.%d (%p), dev %s (%p)\n",
		__func__,
		_pdevice->name, _pdevice->id, _pdevice,
		dev_name(dev), dev);

da850_trik_lcd_init(); 

	info = framebuffer_alloc(sizeof(struct da8xx_ili9340_par), dev);
	if (info == NULL) {
		dev_err(dev, "%s: cannot allocate framebuffer\n", __func__);
		ret = -ENOMEM;
		goto exit;
	}

	platform_set_drvdata(_pdevice, info);
	dev_set_drvdata(dev, info);
	par = info->par;
	par->fb_info = info;

	ret = da8xx_ili9340_fb_init(_pdevice, pdata);
	if (ret) {
		dev_err(dev, "%s: cannot initialize framebuffer: %d\n", __func__, ret);
		goto exit_release_framebuffer;
	}

	ret = da8xx_ili9340_lcdc_init(_pdevice, pdata);
	if (ret) {
		dev_err(dev, "%s: cannot initialize da8xx LCD controller: %d\n", __func__, ret);
		goto exit_fb_shutdown;
	}

	ret = da8xx_ili9340_display_init(_pdevice, pdata);
	if (ret) {
		dev_err(dev, "%s: cannot initialize da8xx display data: %d\n", __func__, ret);
		goto exit_lcdc_shutdown;
	}


	ret = register_framebuffer(info);
	if (ret) {
		dev_err(dev, "%s: cannot register framebuffer: %d\n", __func__, ret);
		goto exit_display_shutdown;
	}


	ret = da8xx_ili9340_sysfs_register(_pdevice, pdata);
	if (ret) {
		dev_err(dev, "%s: cannot register sysfs entries: %d\n", __func__, ret);
		goto exit_unregister_framebuffer;
	}


	dev_dbg(dev, "%s: done\n", __func__);

	return 0;


 //exit_unregister_sysfs:
	da8xx_ili9340_sysfs_unregister(_pdevice);
 exit_unregister_framebuffer:
	unregister_framebuffer(info);
 exit_display_shutdown:
	da8xx_ili9340_display_shutdown(_pdevice);
 exit_lcdc_shutdown:
	da8xx_ili9340_lcdc_shutdown(_pdevice);
 exit_fb_shutdown:
	da8xx_ili9340_fb_shutdown(_pdevice);
 exit_release_framebuffer:
	framebuffer_release(info);
 exit:
	return ret;
}

static int da8xx_ili9340_remove(struct platform_device* _pdevice)
{
	struct device* dev	= &_pdevice->dev;
	struct fb_info* info	= platform_get_drvdata(_pdevice);

	dev_dbg(dev, "%s: called\n", __func__);

	da8xx_ili9340_sysfs_unregister(_pdevice);

	unregister_framebuffer(info);

	da8xx_ili9340_display_shutdown(_pdevice);
	da8xx_ili9340_lcdc_shutdown(_pdevice);
	da8xx_ili9340_fb_shutdown(_pdevice);

	framebuffer_release(info);

	dev_dbg(dev, "%s: done\n", __func__);

	return 0;
}



static struct of_device_id xillybus_of_match[] = {
  { .compatible = "hack,ili9340", },
  {}
};



static struct platform_driver da8xx_ili9340_driver = {
	.probe		= da8xx_ili9340_probe,
	.remove		= da8xx_ili9340_remove,
	.driver		= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
                .of_match_table = xillybus_of_match,
	},
};

static int __init da8xx_ili9340_init_module(void)
{
	int ret		= -EINVAL;

	ret = platform_driver_register(&da8xx_ili9340_driver);

	if (ret) {
		pr_err("%s: cannot register platform driver %s: %d\n", __func__, da8xx_ili9340_driver.driver.name, ret);
		goto exit;
	}

	pr_debug("%s: registered platform driver %s\n", __func__, da8xx_ili9340_driver.driver.name);

	return 0;


 //exit_driver_unregister:
	platform_driver_unregister(&da8xx_ili9340_driver);
 exit:
	return ret;
}

module_init(da8xx_ili9340_init_module);

#ifdef MODULE
static void __exit da8xx_ili9340_exit_module(void)
{
	platform_driver_unregister(&da8xx_ili9340_driver);
}

module_exit(da8xx_ili9340_exit_module);
#endif /* MODULE */

MODULE_LICENSE("GPL");

