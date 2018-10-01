#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/input.h>
#include <linux/gpio.h>
//#include <linux/leds.h>
#include <linux/of_gpio.h>
#include <linux/fs.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>

#include <linux/fb.h>
#include <linux/uaccess.h>

#include "st7735s.h"


#define DRIVER_NAME			"lcd_st7735s_drv"


static struct device *dev;

static const struct of_device_id my_drvr_match[];


typedef enum {
	LCD_Landscape,
	LCD_Portrait
} LCD_Orientation;


struct lcd_data {
	struct spi_device *spi_device;

	struct fb_info *info;

	struct class *sys_class;
	struct gpio_desc *RST_gpiod;
	struct gpio_desc *DC_gpiod;
	//struct gpio_desc *CS_gpiod;

	struct work_struct display_update_ws;

	LCD_Orientation orientation;

	u16 width;
	u16 height;
	u8 RST_state;
	u8 DC_state;
	//u8 CS_state;
};


static struct lcd_data *lcd;

static u16 *lcd_vmem;
static u16 *lcd_buff;

static uint32_t pseudo_palette[16];

static struct fb_fix_screeninfo st7735s_fix = {
	.id 			= "Sitronix st7735s",
	.type       	= FB_TYPE_PACKED_PIXELS,
	.visual     	= FB_VISUAL_TRUECOLOR,
	.xpanstep   	= 0,
	.ypanstep   	= 0,
	.ywrapstep  	= 0,
	.line_length    = (LCD_WIDTH * BPP) / 8,
	.accel      	= FB_ACCEL_NONE,
};


static struct fb_var_screeninfo st7735s_var = {
		.xres           = LCD_WIDTH,
		.yres           = LCD_HEIGHT,
		.xres_virtual   = LCD_WIDTH,
		.yres_virtual   = LCD_HEIGHT,
		.height         = -1,
		.width          = -1,
		.bits_per_pixel = BPP,
		
		.red            = { 11, 5, 0 },
		.green          = { 5, 6, 0 },
		.blue           = { 0, 5, 0 },
		.nonstd         = 0,
};



static void UtilWaitDelay(u32 delay)
{

}


static void RST_Force(u8 state)
{
	gpiod_set_value(lcd->RST_gpiod, state);

	UtilWaitDelay(0x0000000F);
	//dev_info(dev, "%s (%d)\n", __FUNCTION__, state);
}


static void DC_Force(u8 state)
{
	gpiod_set_value(lcd->DC_gpiod, state);

	UtilWaitDelay(0x0000000F);
	//dev_info(dev, "%s (%d)\n", __FUNCTION__, state);
}


#define TFT_DC_SET		DC_Force(1);
#define TFT_DC_RESET	DC_Force(0);

#define TFT_RST_SET		RST_Force(1);
#define TFT_RST_RESET	RST_Force(0);


/* Init script function */
struct st7735_function {
	u16 cmd;
	u16 data;
};

/* Init script commands */
enum st7735_cmd {
	ST7735_START,
	ST7735_END,
	ST7735_CMD,
	ST7735_DATA,
	ST7735_DELAY
};

/* lcd configuration */
static struct st7735_function st7735_cfg_script[] = {
	{ ST7735_START, ST7735_START},
	{ ST7735_CMD, ST7735_SWRESET},
	{ ST7735_DELAY, 150},
	{ ST7735_CMD, ST7735_SLPOUT},
	{ ST7735_DELAY, 500},
	{ ST7735_CMD, ST7735_FRMCTR1},
	{ ST7735_DATA, 0x01},
	{ ST7735_DATA, 0x2c},
	{ ST7735_DATA, 0x2d},
	{ ST7735_CMD, ST7735_FRMCTR2},
	{ ST7735_DATA, 0x01},
	{ ST7735_DATA, 0x2c},
	{ ST7735_DATA, 0x2d},
	{ ST7735_CMD, ST7735_FRMCTR3},
	{ ST7735_DATA, 0x01},
	{ ST7735_DATA, 0x2c},
	{ ST7735_DATA, 0x2d},
	{ ST7735_DATA, 0x01},
	{ ST7735_DATA, 0x2c},
	{ ST7735_DATA, 0x2d},
	{ ST7735_CMD, ST7735_INVCTR},
	{ ST7735_DATA, 0x07},
	{ ST7735_CMD, ST7735_PWCTR1},
	{ ST7735_DATA, 0xa2},
	{ ST7735_DATA, 0x02},
	{ ST7735_DATA, 0x84},
	{ ST7735_CMD, ST7735_PWCTR2},
	{ ST7735_DATA, 0xc5},
	{ ST7735_CMD, ST7735_PWCTR3},
	{ ST7735_DATA, 0x0a},
	{ ST7735_DATA, 0x00},
	{ ST7735_CMD, ST7735_PWCTR4},
	{ ST7735_DATA, 0x8a},
	{ ST7735_DATA, 0x2a},
	{ ST7735_CMD, ST7735_PWCTR5},
	{ ST7735_DATA, 0x8a},
	{ ST7735_DATA, 0xee},
	{ ST7735_CMD, ST7735_VMCTR1},
	{ ST7735_DATA, 0x0e},
	{ ST7735_CMD, ST7735_INVOFF},
	{ ST7735_CMD, ST7735_MADCTL}, // Orientation
	//{ ST7735_DATA, 0xc8},
	{ ST7735_DATA, 0x00},
	{ ST7735_CMD, ST7735_COLMOD},
	{ ST7735_DATA, 0x05},
	{ ST7735_CMD, ST7735_CASET},
	{ ST7735_DATA, 0x00},
	{ ST7735_DATA, 0x00},
	{ ST7735_DATA, 0x00},
	{ ST7735_DATA, 0x00},
	{ ST7735_DATA, 0x7f},
	{ ST7735_CMD, ST7735_RASET},
	{ ST7735_DATA, 0x00},
	{ ST7735_DATA, 0x00},
	{ ST7735_DATA, 0x00},
	{ ST7735_DATA, 0x00},
	{ ST7735_DATA, 0x9f},
	{ ST7735_CMD, ST7735_GMCTRP1},
	{ ST7735_DATA, 0x02},
	{ ST7735_DATA, 0x1c},
	{ ST7735_DATA, 0x07},
	{ ST7735_DATA, 0x12},
	{ ST7735_DATA, 0x37},
	{ ST7735_DATA, 0x32},
	{ ST7735_DATA, 0x29},
	{ ST7735_DATA, 0x2d},
	{ ST7735_DATA, 0x29},
	{ ST7735_DATA, 0x25},
	{ ST7735_DATA, 0x2b},
	{ ST7735_DATA, 0x39},
	{ ST7735_DATA, 0x00},
	{ ST7735_DATA, 0x01},
	{ ST7735_DATA, 0x03},
	{ ST7735_DATA, 0x10},
	{ ST7735_CMD, ST7735_GMCTRN1},
	{ ST7735_DATA, 0x03},
	{ ST7735_DATA, 0x1d},
	{ ST7735_DATA, 0x07},
	{ ST7735_DATA, 0x06},
	{ ST7735_DATA, 0x2e},
	{ ST7735_DATA, 0x2c},
	{ ST7735_DATA, 0x29},
	{ ST7735_DATA, 0x2d},
	{ ST7735_DATA, 0x2e},
	{ ST7735_DATA, 0x2e},
	{ ST7735_DATA, 0x37},
	{ ST7735_DATA, 0x3f},
	{ ST7735_DATA, 0x00},
	{ ST7735_DATA, 0x00},
	{ ST7735_DATA, 0x02},
	{ ST7735_DATA, 0x10},
	{ ST7735_CMD, ST7735_DISPON},
	{ ST7735_DELAY, 100},
	{ ST7735_CMD, ST7735_NORON},
	{ ST7735_DELAY, 10},
	{ ST7735_END, ST7735_END},
};

static int st7735_write(struct lcd_data *lcd, u8 data)
{
	u8 txbuf[2]; /* allocation from stack must go */

	txbuf[0] = data;

	return spi_write(lcd->spi_device, &txbuf[0], 1);
}

static void st7735_write_data(struct lcd_data *lcd, u8 data)
{
	int ret = 0;

	/* Set data mode */
	TFT_DC_SET;

	ret = st7735_write(lcd, data);
	if (ret < 0)
		pr_err("write data %02x failed with status %d\n", data, ret);
}

static int st7735_write_data_buf(struct lcd_data *lcd,
					u8 *txbuf, int size)
{
	/* Set data mode */
	TFT_DC_SET;

	/* Write entire buffer */
	return spi_write(lcd->spi_device, txbuf, size);
}

static void st7735_write_cmd(struct lcd_data *lcd, u8 data)
{
	int ret = 0;

	/* Set command mode */
	TFT_DC_RESET;

	ret = st7735_write(lcd, data);
	if (ret < 0)
		pr_err("write command %02x failed with status %d\n", data, ret);
}

static void st7735_run_cfg_script(struct lcd_data *lcd)
{
	int i = 0;
	int end_script = 0;

	do {
		switch (st7735_cfg_script[i].cmd)
		{
		case ST7735_START:
			break;
		case ST7735_CMD:
			st7735_write_cmd(lcd, st7735_cfg_script[i].data & 0xff);
			break;
		case ST7735_DATA:
			st7735_write_data(lcd, st7735_cfg_script[i].data & 0xff);
			break;
		case ST7735_DELAY:
			mdelay(st7735_cfg_script[i].data);
			break;
		case ST7735_END:
			end_script = 1;
		}
		i++;
	} while (!end_script);
}

static void st7735_set_addr_win(struct lcd_data *lcd, int xs, int ys, int xe, int ye)
{
	st7735_write_cmd(lcd, ST7735_CASET);
	st7735_write_data(lcd, 0x00);
	st7735_write_data(lcd, xs+0);
	st7735_write_data(lcd, 0x00);
	st7735_write_data(lcd, xe+2);
	st7735_write_cmd(lcd, ST7735_RASET);
	st7735_write_data(lcd, 0x00);
	st7735_write_data(lcd, ys+0);
	st7735_write_data(lcd, 0x00);
	st7735_write_data(lcd, ye+1);
}

static void st7735_reset(struct lcd_data *lcd)
{
	/* Reset controller */
	TFT_RST_RESET;
	udelay(10);
	TFT_RST_SET;
	mdelay(120);
}

static void LCD_Init(void){

	st7735_reset(lcd);

	st7735_run_cfg_script(lcd);
}



/* For now, just write the full 40KiB on each update */
static void st7735_UpdateScreen(struct lcd_data *lcd){
	int ret = 0;
	int i = 0;
	u8 *vmem = lcd->info->screen_base;
	u32 vmem_size;
	u16 *vmem16 = (u16 *)vmem;

	/* Set row/column data window */
	st7735_set_addr_win(lcd, 0, 0, lcd->width-1, lcd->height-1);

	/* Internal RAM write command */
	st7735_write_cmd(lcd, ST7735_RAMWR);

	vmem_size = (lcd->width * lcd->height * BPP) / 8;
	
		// swap colors 
		for (i = 0; i < vmem_size; i++)
			lcd_buff[i] = swab16(vmem16[i]);
	
	/* Blast framebuffer to ST7735 internal display RAM */
	ret = st7735_write_data_buf(lcd, (u8 *)lcd_buff, vmem_size);	
}




static void update_display_work(struct work_struct *work)
{
	struct lcd_data *lcd =
		container_of(work, struct lcd_data, display_update_ws);
	st7735_UpdateScreen(lcd);
}


static ssize_t clear_show(struct class *class,
	struct class_attribute *attr, char *buf)
{
	ssize_t i = 0;

	i += sprintf(buf, "sys_lcd_clear\n");

	memset(lcd_vmem, 0x0000, lcd->height * lcd->width*2);

	st7735_UpdateScreen(lcd);

	return i;
}


static ssize_t paint_show(struct class *class,
	struct class_attribute *attr, char *buf)
{
	ssize_t i = 0;
	long indx = 0;
	u16 *pmem = 0;
	u16 color = 0x007F;

	i += sprintf(buf, "sys_lcd_paint\n");
	// http://www.barth-dev.de/online/rgb565-color-picker/

	pmem = lcd_vmem;

	color = 0xF800;	// RED
	for(indx = 0; indx < (lcd->height * lcd->width / 3); indx++){
		//color++;
		*pmem++ = color;
	}

	color = 0x07E0; // GREEN
	for(indx = 0; indx < (lcd->height * lcd->width / 3); indx++){
		//color++;
		*pmem++ = color;
	}

	color = 0x001F; // BLUE
	for(indx = 0; indx < (lcd->height * lcd->width / 3); indx++){
		//color++;
		*pmem++ = color;
	}


	st7735_UpdateScreen(lcd);

	return i;
}



CLASS_ATTR_RO(clear);
CLASS_ATTR_RO(paint);


static void make_sysfs_entry(struct spi_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	const char *name;
	int res;

	struct class *sys_class;

	if (np) {

		if (!of_property_read_string(np, "label", &name))
			dev_info(dev, "label = %s\n", name);


		sys_class = class_create(THIS_MODULE, name);

		if (IS_ERR(sys_class)){
			dev_err(dev, "bad class create\n");
		}
		else{
			res = class_create_file(sys_class, &class_attr_clear);
			res = class_create_file(sys_class, &class_attr_paint);

			lcd->sys_class = sys_class;


			dev_info(dev, "sys class created = %s\n", name);
		}
	}

}




static int get_platform_info(struct spi_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	const char *name;

	int gpiod_cnt;

	if (np) {

		if (!of_property_read_string(np, "label", &name))
			dev_info(dev, "label = %s\n", name);

		if (np->name)
			dev_info(dev, "np->name = %s\n", np->name);

		if (np->parent)
			dev_info(dev, "np->parent->name = %s\n", np->parent->name);

		if (np->child)
			dev_info(dev, "np->child->name = %s\n", np->child->name);


		gpiod_cnt = gpiod_count(dev, "reset");
		dev_info(dev, "gpiod_cnt = %d\n", gpiod_cnt);

		//lcd->CS_gpiod = devm_gpiod_get(dev, "cs", GPIOD_OUT_HIGH);
		//if (IS_ERR(lcd->CS_gpiod)) {
		//	dev_err(dev, "fail to get reset-gpios()\n");
		//	return EINVAL;
		//}
		//if(!gpiod_direction_output(lcd->CS_gpiod, 1))
		//	dev_info(dev, "cs-gpios set as OUT\n");

		lcd->RST_gpiod = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
		if (IS_ERR(lcd->RST_gpiod)) {
			dev_err(dev, "fail to get reset-gpios()\n");
			return EINVAL;
		}
		if(!gpiod_direction_output(lcd->RST_gpiod, 1))
			dev_info(dev, "reset-gpios set as OUT\n");


		lcd->DC_gpiod = devm_gpiod_get(dev, "dc", GPIOD_OUT_HIGH);
		if (IS_ERR(lcd->DC_gpiod)) {
			dev_err(dev, "fail to get dc-gpios()\n");
			return EINVAL;
		}
		if(!gpiod_direction_output(lcd->DC_gpiod, 1))
			dev_info(dev, "dc-gpios set as OUT\n");
		
		return 1;
	}
	else{
		dev_err(dev, "failed to get device_node\n");
		return -EINVAL;
	}

	return 0;
}


static int st7735s_setcolreg (u_int regno,
        u_int red, u_int green, u_int blue,
        u_int transp, struct fb_info *info)
{
	u32 *palette = info->pseudo_palette;

	if (regno >= 16)
			return -EINVAL;

	/* only FB_VISUAL_TRUECOLOR supported */

	red >>= 16 - info->var.red.length;
	green >>= 16 - info->var.green.length;
	blue >>= 16 - info->var.blue.length;
	transp >>= 16 - info->var.transp.length;
	
	palette[regno] = (red << info->var.red.offset) 		|
					(green << info->var.green.offset) 	|
					(blue << info->var.blue.offset) 	|
					(transp << info->var.transp.offset);
	return 0;
}


static ssize_t st7735s_write(struct fb_info *info, const char __user *buf,
		size_t count, loff_t *ppos)
{
	struct lcd_data *lcd = info->par;
	unsigned long total_size;
	unsigned long p = *ppos;
	u8 __iomem *dst;

	total_size = info->fix.smem_len;

	if (p > total_size)
		return -EINVAL;

	if (count + p > total_size)
		count = total_size - p;

	if (!count)
		return -EINVAL;

	dst = (void __force *) (info->screen_base + p);

	if (copy_from_user(dst, buf, count))
		return -EFAULT;

	schedule_work(&lcd->display_update_ws);

	*ppos += count;

	return count;
}


static int st7735s_blank(int blank_mode, struct fb_info *info)
{
	return 0;
}

static void st7735s_fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{
	struct lcd_data *lcd = info->par;
	sys_fillrect(info, rect);
	schedule_work(&lcd->display_update_ws);

	//dev_info(dev, "%s\n", __FUNCTION__);
}

static void st7735s_copyarea(struct fb_info *info, const struct fb_copyarea *area)
{
	struct lcd_data *lcd = info->par;
	sys_copyarea(info, area);
	schedule_work(&lcd->display_update_ws);

	//dev_info(dev, "%s\n", __FUNCTION__);
}

static void st7735s_imageblit(struct fb_info *info, const struct fb_image *image)
{
	sys_imageblit(info, image);
	schedule_work(&lcd->display_update_ws);

	//dev_info(dev, "%s\n", __FUNCTION__);
}

static struct fb_ops st7735s_ops = {
	.owner          = THIS_MODULE,
	.fb_setcolreg   = st7735s_setcolreg,
	.fb_read        = fb_sys_read,
	.fb_write       = st7735s_write,
	.fb_blank       = st7735s_blank,
	.fb_fillrect    = st7735s_fillrect,
	.fb_copyarea    = st7735s_copyarea,
	.fb_imageblit   = st7735s_imageblit,
};


static int st7735s_probe(struct spi_device *spi)
{
	const struct of_device_id *match;
	struct fb_info *info;
	u32 vmem_size;
	int ret;

	dev = &spi->dev;

	match = of_match_device(of_match_ptr(my_drvr_match), dev);
	if (!match) {
		dev_err(dev, "failed of_match_device()\n");
		return -EINVAL;
	}


	spi->mode = SPI_MODE_0;
	spi->bits_per_word = 8;
	spi_setup(spi);


	info = framebuffer_alloc(sizeof(struct lcd_data), &spi->dev);

	if (!info){
		dev_err(dev, "framebuffer_alloc has failed\n");
		return -ENOMEM;
	}


	lcd 				= info->par;
	lcd->spi_device 	= spi;

	lcd->width  		= LCD_WIDTH;
	lcd->height 		= LCD_HEIGHT;


	vmem_size = (lcd->width * lcd->height * BPP) / 8;


	lcd_vmem = devm_kzalloc(dev, vmem_size, GFP_KERNEL);
	lcd_buff = devm_kzalloc(dev, vmem_size, GFP_KERNEL);

	if (!lcd_vmem) {
		dev_err(dev, "Couldn't allocate graphical memory.\n");
		return -ENOMEM;
	}


	info->fbops     		= &st7735s_ops;
	info->fix       		= st7735s_fix;
	info->fix.line_length 	= (lcd->width * BPP) / 8;

	info->var 				= st7735s_var;

	info->var.xres 			= lcd->width;
	info->var.xres_virtual 	= lcd->width;
	info->var.yres 			= lcd->height;
	info->var.yres_virtual 	= lcd->height;

	info->screen_base = (u8 __force __iomem *)lcd_vmem;

	info->fix.smem_start = __pa(lcd_vmem);
	info->fix.smem_len = vmem_size;
	info->flags = FBINFO_FLAG_DEFAULT;

	info->pseudo_palette = pseudo_palette;

	lcd->info 			= info;


	dev_info(dev, "Hello, world!\n");
	dev_info(dev, "spiclk %u KHz.\n",	(spi->max_speed_hz + 500) / 1000);
	dev_info(dev, "LCD mem addr: 0x%X", lcd_vmem);

	if(!get_platform_info(spi)){
		dev_err(dev, "failed to get platform info\n");
		return -EINVAL;
	}


	LCD_Init();
	INIT_WORK(&lcd->display_update_ws, update_display_work);

	spi_set_drvdata(spi, lcd);

	make_sysfs_entry(spi);


	ret = register_framebuffer(info);
	if (ret) {
		dev_err(dev, "Couldn't register the framebuffer\n");
		return ret;
	}

	dev_info(dev, "module initialized\n");
	return 0;
}



static int st7735s_remove(struct spi_device *spi)
{
	struct device *dev = &spi->dev;
	struct class *sys_class;
	struct fb_info *info = lcd->info;

	sys_class = lcd->sys_class;

	cancel_work_sync(&lcd->display_update_ws);

    kfree(lcd_vmem);
    kfree(lcd_buff);
	
	unregister_framebuffer(info);

	class_remove_file(sys_class, &class_attr_clear);
	class_remove_file(sys_class, &class_attr_paint);
	class_destroy(sys_class);

	dev_info(dev, "Goodbye, world!\n");
	return 0;
}



static const struct of_device_id my_drvr_match[] = {
	{ .compatible = "Domin,lcd_st7735s_drv", },
	{ },
};
MODULE_DEVICE_TABLE(of, my_drvr_match);



static const struct spi_device_id st7735s_spi_ids[] = {
	{ "lcd_st7735s_drv", 0 },
	{}
};
MODULE_DEVICE_TABLE(spi, st7735s_spi_ids);
 

static struct spi_driver my_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = my_drvr_match,
	},
	.probe 		= st7735s_probe,
	.remove 	= st7735s_remove,
	.id_table 	= st7735s_spi_ids,
};
module_spi_driver(my_driver);



MODULE_LICENSE("GPL");
MODULE_AUTHOR("Denis Domin <unkindx@gmail.com>");