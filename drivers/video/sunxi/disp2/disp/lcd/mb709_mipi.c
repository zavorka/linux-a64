#include "sl698ph_720p.h"
#include "panels.h"

static void LCD_power_on(__u32 sel);
static void LCD_power_off(__u32 sel);
static void LCD_bl_open(__u32 sel);
static void LCD_bl_close(__u32 sel);

static void LCD_panel_init(__u32 sel);
static void LCD_panel_exit(__u32 sel);

static void LCD_cfg_panel_info(panel_extend_para * info)
{
	__u32 i = 0, j=0;
	__u32 items;
	__u8 lcd_gamma_tbl[][2] =
	{
		//{input value, corrected value}
		{0, 0},
		{15, 15},
		{30, 30},
		{45, 45},
		{60, 60},
		{75, 75},
		{90, 90},
		{105, 105},
		{120, 120},
		{135, 135},
		{150, 150},
		{165, 165},
		{180, 180},
		{195, 195},
		{210, 210},
		{225, 225},
		{240, 240},
		{255, 255},
	};

	__u8 lcd_bright_curve_tbl[][2] =
	{
		//{input value, corrected value}
		{0    ,0 },//0
		{1   ,50 },//0
		{30   ,50  },//0
		{45   ,50 },// 1
		{60   ,50  },// 2
		{75   ,50  },// 5
		{90   ,50  },//9
		{105   ,50 }, //15
		{120  ,50 },//23
		{135  ,50 },//33
		{150  ,54 },
		{165  ,67 },
		{180  ,84 },
		{195  ,108},
		{210  ,137},
		{225 ,171},
		{240 ,210},
		{255 ,255},
	};

	__u32 lcd_cmap_tbl[2][3][4] = {
	{
		{LCD_CMAP_G0,LCD_CMAP_B1,LCD_CMAP_G2,LCD_CMAP_B3},
		{LCD_CMAP_B0,LCD_CMAP_R1,LCD_CMAP_B2,LCD_CMAP_R3},
		{LCD_CMAP_R0,LCD_CMAP_G1,LCD_CMAP_R2,LCD_CMAP_G3},
		},
		{
		{LCD_CMAP_B3,LCD_CMAP_G2,LCD_CMAP_B1,LCD_CMAP_G0},
		{LCD_CMAP_R3,LCD_CMAP_B2,LCD_CMAP_R1,LCD_CMAP_B0},
		{LCD_CMAP_G3,LCD_CMAP_R2,LCD_CMAP_G1,LCD_CMAP_R0},
		},
	};

	memset(info,0,sizeof(panel_extend_para));

	items = sizeof(lcd_gamma_tbl)/2;
	for(i=0; i<items-1; i++) {
		__u32 num = lcd_gamma_tbl[i+1][0] - lcd_gamma_tbl[i][0];

		for(j=0; j<num; j++) {
			__u32 value = 0;

			value = lcd_gamma_tbl[i][1] + ((lcd_gamma_tbl[i+1][1] - lcd_gamma_tbl[i][1]) * j)/num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] = (value<<16) + (value<<8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items-1][1]<<16) + (lcd_gamma_tbl[items-1][1]<<8) + lcd_gamma_tbl[items-1][1];

	items = sizeof(lcd_bright_curve_tbl)/2;
	for(i=0; i<items-1; i++) {
		__u32 num = lcd_bright_curve_tbl[i+1][0] - lcd_bright_curve_tbl[i][0];

		for(j=0; j<num; j++) {
			__u32 value = 0;

			value = lcd_bright_curve_tbl[i][1] + ((lcd_bright_curve_tbl[i+1][1] - lcd_bright_curve_tbl[i][1]) * j)/num;
			info->lcd_bright_curve_tbl[lcd_bright_curve_tbl[i][0] + j] = value;
		}
	}
	info->lcd_bright_curve_tbl[255] = lcd_bright_curve_tbl[items-1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

static __s32 LCD_open_flow(__u32 sel)
{
	LCD_OPEN_FUNC(sel, LCD_power_on, 30);   //open lcd power, and delay 50ms
	LCD_OPEN_FUNC(sel, LCD_panel_init, 10);   //open lcd power, than delay 200ms
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 100);     //open lcd controller, and delay 100ms
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);     //open lcd backlight, and delay 0ms

	return 0;
}

static __s32 LCD_close_flow(__u32 sel)
{
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 0);       //close lcd backlight, and delay 0ms
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 0);         //close lcd controller, and delay 0ms
	LCD_CLOSE_FUNC(sel, LCD_panel_exit,	20);   //open lcd power, than delay 200ms
	LCD_CLOSE_FUNC(sel, LCD_power_off, 50);   //close lcd power, and delay 500ms

	return 0;
}

static void LCD_power_on(__u32 sel)
{
    printk("mb709_mipi: %s", __func__);
	sunxi_lcd_power_enable(sel, 0);//config lcd_power pin to open lcd power0
}

static void LCD_power_off(__u32 sel)
{
    printk("mb709_mipi: %s", __func__);
	sunxi_lcd_pin_cfg(sel, 0);
	sunxi_lcd_power_disable(sel, 0);//config lcd_power pin to close lcd power0
}

static void LCD_bl_open(__u32 sel)
{
    printk("mb709_mipi: %s", __func__);
	sunxi_lcd_pwm_enable(sel);//open pwm module
	sunxi_lcd_backlight_enable(sel);//config lcd_bl_en pin to open lcd backlight
}

static void LCD_bl_close(__u32 sel)
{
    printk("mb709_mipi: %s", __func__);
	sunxi_lcd_backlight_disable(sel);//config lcd_bl_en pin to close lcd backlight
	sunxi_lcd_pwm_disable(sel);//close pwm module
}

#define REGFLAG_DELAY             							0XFF
#define REGFLAG_END_OF_TABLE      						0xFE   // END OF REGISTERS MARKER

struct LCM_setting_table {
    __u8 cmd;
    __u32 count;
    __u8 para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = {
	{0x80,1,{0x58}},
	{0x81,1,{0x47}},
	{0x82,1,{0xD4}},
	{0x83,1,{0x88}},
	{0x84,1,{0xA9}},
	{0x85,1,{0xC3}},
	{0x86,1,{0x82}},
 {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static void LCD_panel_init(__u32 sel)
{
	__u32 i;
    printk("mb709_mipi: %s", __func__);
	sunxi_lcd_pin_cfg(sel, 1);
	sunxi_lcd_delay_ms(10);

	panel_rst(1);	 //add by lyp@20140423
	sunxi_lcd_delay_ms(50);//add by lyp@20140423
	panel_rst(0);
	sunxi_lcd_delay_ms(20);
	panel_rst(1);
	sunxi_lcd_delay_ms(200);

	for(i=0;;i++)
	{
		if(lcm_initialization_setting[i].cmd == REGFLAG_END_OF_TABLE)
			break;
		else if (lcm_initialization_setting[i].cmd == REGFLAG_DELAY)
			sunxi_lcd_delay_ms(lcm_initialization_setting[i].count);
		else
		{
			printk("[mb709_mipi] send cmd=0x%x\n", lcm_initialization_setting[i].cmd);
			dsi_dcs_wr(0,lcm_initialization_setting[i].cmd,lcm_initialization_setting[i].para_list,lcm_initialization_setting[i].count);
		}
	}

	sunxi_lcd_dsi_clk_enable(sel);

	return;
}

static void LCD_panel_exit(__u32 sel)
{
    printk("mb709_mipi: %s", __func__);
	sunxi_lcd_dsi_clk_disable(sel);
	panel_rst(0);
	return ;
}

//sel: 0:lcd0; 1:lcd1
static __s32 LCD_user_defined_func(__u32 sel, __u32 para1, __u32 para2, __u32 para3)
{
	return 0;
}

__lcd_panel_t mb709_mipi_panel = {
	/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
	.name = "mb709_mipi",
	.func = {
		.cfg_panel_info = LCD_cfg_panel_info,
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		.lcd_user_defined_func = LCD_user_defined_func,
	},
};
