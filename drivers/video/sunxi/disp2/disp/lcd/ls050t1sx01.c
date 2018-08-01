#include "ls050t1sx01.h"

extern s32 bsp_disp_get_panel_info(u32 screen_id, disp_panel_para *info);

#define DBG()         printk("ls050t1sx01[%i]: %s",         sel, __func__      )
#define DBGi1(a)      printk("ls050t1sx01[%i]: %s %i",      sel, __func__,a    )
#define DBGi3(a,b,c)  printk("ls050t1sx01[%i]: %s %i %i %i",sel, __func__,a,b,c)

static void LCD_power_on(u32 sel);
static void LCD_power_off(u32 sel);
static void LCD_bl_open(u32 sel);
static void LCD_bl_close(u32 sel);

static void LCD_panel_init(u32 sel);
static void LCD_panel_exit(u32 sel);

#define LCD_GPIO_PWREN  0
#define LCD_GPIO_RESET  1

#define gpio_panel_reset(val) sunxi_lcd_gpio_set_value(sel, LCD_GPIO_RESET, val)
#define gpio_power_en(val)    sunxi_lcd_gpio_set_value(sel, LCD_GPIO_PWREN, val)

static void LCD_cfg_panel_info(panel_extend_para * info)
{
	u32 i = 0, j=0;
	u32 items;
	u8 lcd_gamma_tbl[][2] =
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

	u32 lcd_cmap_tbl[2][3][4] = {
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

	items = sizeof(lcd_gamma_tbl)/2;
	for (i=0; i<items-1; i++) {
		u32 num = lcd_gamma_tbl[i+1][0] - lcd_gamma_tbl[i][0];

		for (j=0; j<num; j++) {
			u32 value = 0;

			value = lcd_gamma_tbl[i][1] + ((lcd_gamma_tbl[i+1][1] - lcd_gamma_tbl[i][1]) * j)/num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] = (value<<16) + (value<<8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items-1][1]<<16) + (lcd_gamma_tbl[items-1][1]<<8) + lcd_gamma_tbl[items-1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

static s32 LCD_open_flow(u32 sel) {
        DBG();
	LCD_OPEN_FUNC(sel, LCD_power_on, 20);   //open lcd power, and delay 50ms
	LCD_OPEN_FUNC(sel, LCD_panel_init, 10);   //open lcd power, than delay 200ms
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 20);     //open lcd controller, and delay 100ms
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);     //open lcd backlight, and delay 0ms
	return 0;
}

static s32 LCD_close_flow(u32 sel) {
        DBG();
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 0);       //close lcd backlight, and delay 0ms
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 0);         //close lcd controller, and delay 0ms
	LCD_CLOSE_FUNC(sel, LCD_panel_exit,	200);   //open lcd power, than delay 200ms
	LCD_CLOSE_FUNC(sel, LCD_power_off, 500);   //close lcd power, and delay 500ms
	return 0;
}

static void LCD_power_on(u32 sel) {
        DBG();

        // defaults
	gpio_panel_reset(0);
        sunxi_lcd_gpio_set_direction(0, LCD_GPIO_RESET, 1); // 1=out
	gpio_power_en(0);
        sunxi_lcd_gpio_set_direction(0, LCD_GPIO_PWREN, 1); // 1=out

        // sequence
	sunxi_lcd_power_enable(sel, 0);//config lcd_power pin to open lcd power
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_enable(sel, 1);//config lcd_power pin to open lcd power0
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_enable(sel, 2);//config lcd_power pin to open lcd power2
	sunxi_lcd_delay_ms(5);
	gpio_power_en(1);
	sunxi_lcd_delay_ms(20);
	gpio_panel_reset(1);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_pin_cfg(sel, 1);
}

static void LCD_power_off(u32 sel) {
        DBG();
	sunxi_lcd_pin_cfg(sel, 0);
	gpio_panel_reset(0);
	sunxi_lcd_delay_ms(20);
	gpio_power_en(0);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 2);//config lcd_power pin to close lcd power2
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 1);//config lcd_power pin to close lcd power1
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 0);//config lcd_power pin to close lcd power
}

static void LCD_bl_open(u32 sel) {
        DBG();
	sunxi_lcd_pwm_enable(sel);
	sunxi_lcd_backlight_enable(sel);//config lcd_bl_en pin to open lcd backlight
}

static void LCD_bl_close(u32 sel) {
        DBG();
	sunxi_lcd_backlight_disable(sel);//config lcd_bl_en pin to close lcd backlight
	sunxi_lcd_pwm_disable(sel);
}

static void LCD_panel_init(u32 sel) {
	disp_panel_para *panel_info = kmalloc(sizeof(disp_panel_para), GFP_KERNEL | __GFP_ZERO);
	u32 bright;

        bsp_disp_get_panel_info(sel, panel_info);
	bright = bsp_disp_lcd_get_bright(sel);

        DBGi1(bright);

	sunxi_lcd_dsi_clk_enable(sel);

	sunxi_lcd_delay_ms(20);
	sunxi_lcd_dsi_dcs_write_0para(sel,DSI_DCS_SOFT_RESET);
	sunxi_lcd_delay_ms(10);

	// sunxi_lcd_dsi_gen_write_1para(sel,0xb0,0x04);
	// sunxi_lcd_dsi_dcs_write_0para(sel,0x00);
	// sunxi_lcd_dsi_dcs_write_0para(sel,0x00);
	// sunxi_lcd_dsi_gen_write_1para(sel,0xd6,0x01);
//	sunxi_lcd_dsi_dcs_write_2para(sel,0x51,bright>>8,bright&0xFF);
	// sunxi_lcd_dsi_dcs_write_1para(sel,0x53,0x04);

	sunxi_lcd_dsi_dcs_write_0para(sel,DSI_DCS_SET_DISPLAY_ON);
	sunxi_lcd_dsi_dcs_write_0para(sel,DSI_DCS_EXIT_SLEEP_MODE);

	sunxi_lcd_delay_ms(120); // min 6 frame

	kfree(panel_info);

	return;
}

static void LCD_panel_exit(u32 sel) {
        DBG();
	sunxi_lcd_dsi_dcs_write_0para(sel,DSI_DCS_SET_DISPLAY_OFF);
	sunxi_lcd_delay_ms(20);  // min 1 frame
	sunxi_lcd_dsi_dcs_write_0para(sel,DSI_DCS_ENTER_SLEEP_MODE);
	sunxi_lcd_delay_ms(80);  // min 4 frames
	return ;
}

//sel: 0:lcd0; 1:lcd1
static s32 LCD_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3) {
        DBGi3(para1,para2,para3);
	return 0;
}

//sel: 0:lcd0; 1:lcd1
static s32 LCD_set_bright(u32 sel, u32 bright) {
        // values seen: 50,51,51,0,0,51
        DBGi1(bright);
        // 0x0000 ~ 0x0FFF
//	sunxi_lcd_dsi_dcs_write_2para(sel,0x51,bright>>8,bright&0xFF);
	return 0;
}

__lcd_panel_t ls050t1sx01_panel = {
	/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
	.name = "ls050t1sx01",
	.func = {
		.cfg_panel_info = LCD_cfg_panel_info,
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		.lcd_user_defined_func = LCD_user_defined_func,
		.set_bright = LCD_set_bright,
	},
};
