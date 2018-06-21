## MIPI-DSI

### Panel drivers

* `disp/lcd/dx0960be40a1`
* `disp/lcd/inet_dsi_panel.c`
* `disp/lcd/lt070me05000.c`
* `disp/lcd/mb709_mipi.c`
* `disp/lcd/S6D7AA0X01.c`
* `disp/lcd/tft720x1280.c`

### Low-level DSI driver

* `disp/de/lowlevel_sun50iw1/de_dsi.c`

### Device-tree configuration

An example of an `lcd` node configured for a MIPI panel:

```
		lcd0@01c0c000 {
			compatible = "allwinner,sunxi-lcd0";
			pinctrl-names = "active", "sleep";
			status = "okay";
			device_type = "lcd0";
			lcd_used = <0x0>;
			lcd_driver_name = "mb709_mipi";
			lcd_backlight = <0x32>;
			lcd_if = <0x4>;
			lcd_x = <0x400>;
			lcd_y = <0x258>;
			lcd_width = <0x0>;
			lcd_height = <0x0>;
			lcd_dclk_freq = <0x37>;
			lcd_pwm_used = <0x1>;
			lcd_pwm_ch = <0x10>;
			lcd_pwm_freq = <0xc350>;
			lcd_pwm_pol = <0x1>;
			lcd_pwm_max_limit = <0xfa>;
			lcd_hbp = <0x78>;
			lcd_ht = <0x604>;
			lcd_hspw = <0x14>;
			lcd_vbp = <0x17>;
			lcd_vt = <0x27b>;
			lcd_vspw = <0x2>;
			lcd_dsi_if = <0x2>;
			lcd_dsi_lane = <0x4>;
			lcd_dsi_format = <0x0>;
			lcd_dsi_eotp = <0x0>;
			lcd_dsi_vc = <0x0>;
			lcd_dsi_te = <0x0>;
			lcd_frm = <0x0>;
			lcd_gamma_en = <0x0>;
			lcd_bright_curve_en = <0x0>;
			lcd_cmap_en = <0x0>;
			lcd_bl_en = <0x30 0x7 0xa 0x1 0x0 0xffffffff 0x1>;
			lcd_bl_en_power = "none";
			lcd_power = "vcc-mipi";
			lcd_fix_power = "vcc-dsi-33";
			lcd_gpio_0 = <0x30 0x3 0x18 0x1 0x0 0xffffffff 0x1>;
		};
```
