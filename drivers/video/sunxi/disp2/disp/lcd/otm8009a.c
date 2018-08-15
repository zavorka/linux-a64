#include "otm8009a.h"

static void LCD_power_on(u32 sel);
static void LCD_power_off(u32 sel);
static void LCD_bl_open(u32 sel);
static void LCD_bl_close(u32 sel);

static void LCD_panel_init(u32 sel);
static void LCD_panel_exit(u32 sel);

extern s32 dsi_dcs_wr_0para(u32 sel,u8 cmd);

#define LCD_GPIO_RESET	0
#define LCD_GPIO_PWREN	1

#define KM040TMP02_X	480
#define KM040TMP02_Y	800

#define MCS_ADRSFT	0x0000	/* Address Shift Function */
#define MCS_PANSET	0xB3A6	/* Panel Type Setting */
#define MCS_SD_CTRL	0xC0A2	/* Source Driver Timing Setting */
#define MCS_P_DRV_M	0xC0B4	/* Panel Driving Mode */
#define MCS_OSC_ADJ	0xC181	/* Oscillator Adjustment for Idle/Normal mode */
#define MCS_RGB_VID_SET	0xC1A1	/* RGB Video Mode Setting */
#define MCS_SD_PCH_CTRL	0xC480	/* Source Driver Precharge Control */
#define MCS_NO_DOC1	0xC48A	/* Command not documented */
#define MCS_PWR_CTRL1	0xC580	/* Power Control Setting 1 */
#define MCS_PWR_CTRL2	0xC590	/* Power Control Setting 2 for Normal Mode */
#define MCS_PWR_CTRL4	0xC5B0	/* Power Control Setting 4 for DC Voltage */
#define MCS_PANCTRLSET1	0xCB80	/* Panel Control Setting 1 */
#define MCS_PANCTRLSET2	0xCB90	/* Panel Control Setting 2 */
#define MCS_PANCTRLSET3	0xCBA0	/* Panel Control Setting 3 */
#define MCS_PANCTRLSET4	0xCBB0	/* Panel Control Setting 4 */
#define MCS_PANCTRLSET5	0xCBC0	/* Panel Control Setting 5 */
#define MCS_PANCTRLSET6	0xCBD0	/* Panel Control Setting 6 */
#define MCS_PANCTRLSET7	0xCBE0	/* Panel Control Setting 7 */
#define MCS_PANCTRLSET8	0xCBF0	/* Panel Control Setting 8 */
#define MCS_PANU2D1	0xCC80	/* Panel U2D Setting 1 */
#define MCS_PANU2D2	0xCC90	/* Panel U2D Setting 2 */
#define MCS_PANU2D3	0xCCA0	/* Panel U2D Setting 3 */
#define MCS_PAND2U1	0xCCB0	/* Panel D2U Setting 1 */
#define MCS_PAND2U2	0xCCC0	/* Panel D2U Setting 2 */
#define MCS_PAND2U3	0xCCD0	/* Panel D2U Setting 3 */
#define MCS_GOAVST	0xCE80	/* GOA VST Setting */
#define MCS_GOACLKA1	0xCEA0	/* GOA CLKA1 Setting */
#define MCS_GOACLKA3	0xCEB0	/* GOA CLKA3 Setting */
#define MCS_GOAECLK	0xCFC0	/* GOA ECLK Setting */
#define MCS_NO_DOC2	0xCFD0	/* Command not documented */
#define MCS_GVDDSET	0xD800	/* GVDD/NGVDD */
#define MCS_VCOMDC	0xD900	/* VCOM Voltage Setting */
#define MCS_GMCT2_2P	0xE100	/* Gamma Correction 2.2+ Setting */
#define MCS_GMCT2_2N	0xE200	/* Gamma Correction 2.2- Setting */
#define MCS_NO_DOC3	0xF5B6	/* Command not documented */
#define MCS_CMD2_ENA1	0xFF00	/* Enable Access Command2 "CMD2" */
#define MCS_CMD2_ENA2	0xFF80	/* Enable Access Orise Command2 */

#define mipi_dsi_dcs_nop(ctx)			dsi_dcs_wr_0para(ctx, DSI_DCS_NOP)
#define mipi_dsi_dcs_set_display_on(ctx)	dsi_dcs_wr_0para(ctx, DSI_DCS_SET_DISPLAY_ON)
#define mipi_dsi_dcs_set_display_off(ctx)	dsi_dcs_wr_0para(ctx, DSI_DCS_SET_DISPLAY_OFF)
#define mipi_dsi_dcs_exit_sleep_mode(ctx)	dsi_dcs_wr_0para(ctx, DSI_DCS_EXIT_SLEEP_MODE)
#define mipi_dsi_dcs_enter_sleep_mode(ctx)	dsi_dcs_wr_0para(ctx, DSI_DCS_ENTER_SLEEP_MODE)

#define mipi_dsi_dcs_set_column_address(ctx, start, end)	\
	dsi_dcs_wr_4para(ctx, DSI_DCS_SET_COLUMN_ADDRESS,	\
				(start) >> 8, (start) & 0xff,	\
				(end) >> 8, (end) & 0xff)

#define mipi_dsi_dcs_set_page_address(ctx, start, end)		\
	dsi_dcs_wr_4para(ctx, DSI_DCS_SET_PAGE_ADDRESS,		\
				(start) >> 8, (start) & 0xff,	\
				(end) >> 8, (end) & 0xff)

#define dcs_write_seq(ctx, cmd, seq...)			\
({							\
	static u8 d[] = { seq };			\
	dsi_dcs_wr(ctx, (u8)cmd, d, ARRAY_SIZE(d));	\
})

#define dcs_write_cmd_at(ctx, cmd, seq...)		\
({							\
	dcs_write_seq(ctx, MCS_ADRSFT, (cmd) & 0xff);	\
	dcs_write_seq(ctx, (cmd) >> 8, seq);		\
})


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

			value = lcd_gamma_tbl[i][1]
				+ ((lcd_gamma_tbl[i+1][1] - lcd_gamma_tbl[i][1]) * j)/num;

			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] =
							(value<<16) + (value<<8) + value;
		}
	}
	info->lcd_gamma_tbl[255] =
		  (lcd_gamma_tbl[items-1][1]<<16)
		+ (lcd_gamma_tbl[items-1][1]<<8)
		+ lcd_gamma_tbl[items-1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

static s32 LCD_open_flow(u32 sel)
{
	printk("[otm8009a] LCD_open_flow\n");

	LCD_OPEN_FUNC(sel, LCD_power_on, 50);   //open lcd power, and delay 50ms
	LCD_OPEN_FUNC(sel, LCD_panel_init, 10);   //open lcd power, then delay 10ms
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 20);     //open lcd controller, and delay 100ms
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);     //open lcd backlight, and delay 0ms

	return 0;
}

static s32 LCD_close_flow(u32 sel)
{
	printk("[otm8009a] LCD_close_flow\n");
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 0);       //close lcd backlight, and delay 0ms
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 0);         //close lcd controller, and delay 0ms
	LCD_CLOSE_FUNC(sel, LCD_panel_exit,	200);   //open lcd power, than delay 200ms
	LCD_CLOSE_FUNC(sel, LCD_power_off, 500);   //close lcd power, and delay 500ms

	return 0;
}

static void LCD_power_on(u32 sel)
{
	printk("[otm8009a] LCD_power_on\n");
	sunxi_lcd_power_enable(sel, 0);//config lcd_power pin to open lcd power
	sunxi_lcd_delay_ms(5);
	/* jdi, power on */
	sunxi_lcd_gpio_set_value(sel, LCD_GPIO_PWREN, 1);
}

static void LCD_power_off(u32 sel)
{
	printk("[otm8009a] LCD_power_off\n");
	sunxi_lcd_pin_cfg(sel, 0);
	sunxi_lcd_gpio_set_value(sel, LCD_GPIO_PWREN, 0);
	sunxi_lcd_delay_ms(20);
	sunxi_lcd_gpio_set_value(sel, LCD_GPIO_RESET, 0);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 0);//config lcd_power pin to close lcd power
}

static void LCD_bl_open(u32 sel)
{
	printk("[otm8009a] LCD_bl_open\n");
	//sunxi_lcd_pwm_enable(sel);
	//sunxi_lcd_backlight_enable(sel);//config lcd_bl_en pin to open lcd backlight
}

static void LCD_bl_close(u32 sel)
{
	printk("[otm8009a] LCD_bl_close\n");
	//sunxi_lcd_backlight_disable(sel);//config lcd_bl_en pin to close lcd backlight
	//sunxi_lcd_pwm_disable(sel);
}

static void LCD_panel_init(u32 ctx)
{
	printk("[OTM8009a] LCD_panel_init\n");

	/* RST is active low */
	sunxi_lcd_gpio_set_value(ctx, LCD_GPIO_RESET, 1);
	sunxi_lcd_delay_ms(50);
	sunxi_lcd_gpio_set_value(ctx, LCD_GPIO_RESET, 0);
	sunxi_lcd_delay_ms(20);
	sunxi_lcd_gpio_set_value(ctx, LCD_GPIO_RESET, 1);
	sunxi_lcd_delay_ms(100);

	/* Enter CMD2 */
	dcs_write_cmd_at(ctx, MCS_CMD2_ENA1, 0x80, 0x09, 0x01);

	/* Enter Orise Command2 */
	dcs_write_cmd_at(ctx, MCS_CMD2_ENA2, 0x80, 0x09);

	dcs_write_cmd_at(ctx, MCS_SD_PCH_CTRL, 0x30);
	sunxi_lcd_delay_ms(10);

	dcs_write_cmd_at(ctx, MCS_NO_DOC1, 0x40);
	sunxi_lcd_delay_ms(10);

	/* Enter Orise Command2 */
	dcs_write_cmd_at(ctx, MCS_CMD2_ENA2, 0x80, 0x09);

	dcs_write_cmd_at(ctx, MCS_SD_PCH_CTRL, 0x30);
	sunxi_lcd_delay_ms(10);

	dcs_write_cmd_at(ctx, MCS_NO_DOC1, 0x40);
	sunxi_lcd_delay_ms(10);

	dcs_write_cmd_at(ctx, MCS_PWR_CTRL4 + 1, 0xA9);
	dcs_write_cmd_at(ctx, MCS_PWR_CTRL2 + 1, 0x34);

	/* P_DRV_M - 0xC0B4h - 181th parameter - Default 0x00 */
	/* -> Column inversion                                */
	dcs_write_cmd_at(ctx, MCS_P_DRV_M, 0x50);
	dcs_write_cmd_at(ctx, MCS_VCOMDC, 0x4E);
	dcs_write_cmd_at(ctx, MCS_OSC_ADJ, 0x66); /* 65Hz */
	dcs_write_cmd_at(ctx, MCS_PWR_CTRL2 + 2, 0x01);
	dcs_write_cmd_at(ctx, MCS_PWR_CTRL2 + 5, 0x34);
	dcs_write_cmd_at(ctx, MCS_PWR_CTRL2 + 4, 0x33);
	dcs_write_cmd_at(ctx, MCS_GVDDSET, 0x79, 0x79);
	dcs_write_cmd_at(ctx, MCS_SD_CTRL + 1, 0x1B);
	dcs_write_cmd_at(ctx, MCS_PWR_CTRL1 + 2, 0x83);
	dcs_write_cmd_at(ctx, MCS_SD_PCH_CTRL + 1, 0x83);
	dcs_write_cmd_at(ctx, MCS_RGB_VID_SET, 0x0E);
	dcs_write_cmd_at(ctx, MCS_PANSET, 0x00, 0x01);

	dcs_write_cmd_at(ctx, MCS_GOAVST, 0x85, 0x01, 0x00, 0x84, 0x01, 0x00);
	dcs_write_cmd_at(ctx, MCS_GOACLKA1, 0x18, 0x04, 0x03, 0x39, 0x00, 0x00,
			 0x00, 0x18, 0x03, 0x03, 0x3A, 0x00, 0x00, 0x00);
	dcs_write_cmd_at(ctx, MCS_GOACLKA3, 0x18, 0x02, 0x03, 0x3B, 0x00, 0x00,
			 0x00, 0x18, 0x01, 0x03, 0x3C, 0x00, 0x00, 0x00);
	dcs_write_cmd_at(ctx, MCS_GOAECLK, 0x01, 0x01, 0x20, 0x20, 0x00, 0x00,
			 0x01, 0x02, 0x00, 0x00);

	dcs_write_cmd_at(ctx, MCS_NO_DOC2, 0x00);

	dcs_write_cmd_at(ctx, MCS_PANCTRLSET1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	dcs_write_cmd_at(ctx, MCS_PANCTRLSET2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			 0, 0, 0, 0, 0);
	dcs_write_cmd_at(ctx, MCS_PANCTRLSET3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			 0, 0, 0, 0, 0);
	dcs_write_cmd_at(ctx, MCS_PANCTRLSET4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	dcs_write_cmd_at(ctx, MCS_PANCTRLSET5, 0, 4, 4, 4, 4, 4, 0, 0, 0, 0,
			 0, 0, 0, 0, 0);
	dcs_write_cmd_at(ctx, MCS_PANCTRLSET6, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4,
			 4, 0, 0, 0, 0);
	dcs_write_cmd_at(ctx, MCS_PANCTRLSET7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	dcs_write_cmd_at(ctx, MCS_PANCTRLSET8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);

	dcs_write_cmd_at(ctx, MCS_PANU2D1, 0x00, 0x26, 0x09, 0x0B, 0x01, 0x25,
			 0x00, 0x00, 0x00, 0x00);
	dcs_write_cmd_at(ctx, MCS_PANU2D2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x0A, 0x0C, 0x02);
	dcs_write_cmd_at(ctx, MCS_PANU2D3, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
	dcs_write_cmd_at(ctx, MCS_PAND2U1, 0x00, 0x25, 0x0C, 0x0A, 0x02, 0x26,
			 0x00, 0x00, 0x00, 0x00);
	dcs_write_cmd_at(ctx, MCS_PAND2U2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x0B, 0x09, 0x01);
	dcs_write_cmd_at(ctx, MCS_PAND2U3, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

	dcs_write_cmd_at(ctx, MCS_PWR_CTRL1 + 1, 0x66);

	dcs_write_cmd_at(ctx, MCS_NO_DOC3, 0x06);

	dcs_write_cmd_at(ctx, MCS_GMCT2_2P, 0x00, 0x09, 0x0F, 0x0E, 0x07, 0x10,
			 0x0B, 0x0A, 0x04, 0x07, 0x0B, 0x08, 0x0F, 0x10, 0x0A,
			 0x01);
	dcs_write_cmd_at(ctx, MCS_GMCT2_2N, 0x00, 0x09, 0x0F, 0x0E, 0x07, 0x10,
			 0x0B, 0x0A, 0x04, 0x07, 0x0B, 0x08, 0x0F, 0x10, 0x0A,
			 0x01);

	/* Exit CMD2 */
	dcs_write_cmd_at(ctx, MCS_CMD2_ENA1, 0xFF, 0xFF, 0xFF);

	mipi_dsi_dcs_nop(ctx);
	mipi_dsi_dcs_exit_sleep_mode(ctx);
	sunxi_lcd_delay_ms(120);

	dsi_dcs_wr_1para(ctx, DSI_DCS_SET_ADDRESS_MODE, 0x00);

	mipi_dsi_dcs_set_column_address(ctx, 0, KM040TMP02_X - 1);
	mipi_dsi_dcs_set_page_address(ctx, 0, KM040TMP02_Y - 1);

	dsi_dcs_wr_1para(ctx, DSI_DCS_SET_PIXEL_FORMAT, 0x77 /* 24bit x 24bit */);

	dsi_dcs_wr_1para(ctx, DSI_DCS_WRITE_POWER_SAVE, 0x00);


	mipi_dsi_dcs_set_display_on(ctx);
	mipi_dsi_dcs_nop(ctx);

	/* Send Command GRAM memory write (no parameters) */
	dsi_dcs_wr_0para(ctx, DSI_DCS_WRITE_MEMORY_START);

	sunxi_lcd_dsi_clk_enable(ctx);
}

static void LCD_panel_exit(u32 sel)
{
	sunxi_lcd_dsi_dcs_write_0para(sel,DSI_DCS_ENTER_SLEEP_MODE);
	sunxi_lcd_delay_ms(120);
	sunxi_lcd_dsi_clk_disable(sel);
}

//sel: 0:lcd0; 1:lcd1
static s32 LCD_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

__lcd_panel_t otm8009a_panel = {
	/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
	.name = "otm8009a",
	.func = {
		.cfg_panel_info = LCD_cfg_panel_info,
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		.lcd_user_defined_func = LCD_user_defined_func,
	},
};
