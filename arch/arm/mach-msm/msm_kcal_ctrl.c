/* arch/arm/mach-msm/msm_kcal_ctrl.c
 *
 * Interface to calibrate display color temperature.
 *
 * Copyright (C) 2012 LGE
 * Copyright (C) 2013 Paul Reioux
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/earlysuspend.h>

#include "../../../drivers/video/msm/mdp.h"

#define KCAL_CTRL_MAJOR_VERSION	1
#define KCAL_CTRL_MINOR_VERSION	0
#define KCAL_PLATFORM_NAME      "DIAG0"

struct kcal_data {
	int red;
	int green;
	int blue;
};

struct kcal_platform_data {
	int (*set_values) (int r, int g, int b);
	int (*get_values) (int *r, int *g, int *b);
	int (*refresh_display) (void);
};

static bool lut_updated = false; 
static struct kcal_platform_data *kcal_ctrl_pdata;
static int last_status_kcal_ctrl;

/* pixel order : RBG */
static unsigned int lcd_rgb_linear_lut[256] = {
	/* default linear qlut */
	0x00000000, 0x00010101, 0x00020202, 0x00030303,
	0x00040404, 0x00050505, 0x00060606, 0x00070707,
	0x00080808, 0x00090909, 0x000a0a0a, 0x000b0b0b,
	0x000c0c0c, 0x000d0d0d, 0x000e0e0e, 0x000f0f0f,
	0x00101010, 0x00111111, 0x00121212, 0x00131313,
	0x00141414, 0x00151515, 0x00161616, 0x00171717,
	0x00181818, 0x00191919, 0x001a1a1a, 0x001b1b1b,
	0x001c1c1c, 0x001d1d1d, 0x001e1e1e, 0x001f1f1f,
	0x00202020, 0x00212121, 0x00222222, 0x00232323,
	0x00242424, 0x00252525, 0x00262626, 0x00272727,
	0x00282828, 0x00292929, 0x002a2a2a, 0x002b2b2b,
	0x002c2c2c, 0x002d2d2d, 0x002e2e2e, 0x002f2f2f,
	0x00303030, 0x00313131, 0x00323232, 0x00333333,
	0x00343434, 0x00353535, 0x00363636, 0x00373737,
	0x00383838, 0x00393939, 0x003a3a3a, 0x003b3b3b,
	0x003c3c3c, 0x003d3d3d, 0x003e3e3e, 0x003f3f3f,
	0x00404040, 0x00414141, 0x00424242, 0x00434343,
	0x00444444, 0x00454545, 0x00464646, 0x00474747,
	0x00484848, 0x00494949, 0x004a4a4a, 0x004b4b4b,
	0x004c4c4c, 0x004d4d4d, 0x004e4e4e, 0x004f4f4f,
	0x00505050, 0x00515151, 0x00525252, 0x00535353,
	0x00545454, 0x00555555, 0x00565656, 0x00575757,
	0x00585858, 0x00595959, 0x005a5a5a, 0x005b5b5b,
	0x005c5c5c, 0x005d5d5d, 0x005e5e5e, 0x005f5f5f,
	0x00606060, 0x00616161, 0x00626262, 0x00636363,
	0x00646464, 0x00656565, 0x00666666, 0x00676767,
	0x00686868, 0x00696969, 0x006a6a6a, 0x006b6b6b,
	0x006c6c6c, 0x006d6d6d, 0x006e6e6e, 0x006f6f6f,
	0x00707070, 0x00717171, 0x00727272, 0x00737373,
	0x00747474, 0x00757575, 0x00767676, 0x00777777,
	0x00787878, 0x00797979, 0x007a7a7a, 0x007b7b7b,
	0x007c7c7c, 0x007d7d7d, 0x007e7e7e, 0x007f7f7f,
	0x00808080, 0x00818181, 0x00828282, 0x00838383,
	0x00848484, 0x00858585, 0x00868686, 0x00878787,
	0x00888888, 0x00898989, 0x008a8a8a, 0x008b8b8b,
	0x008c8c8c, 0x008d8d8d, 0x008e8e8e, 0x008f8f8f,
	0x00909090, 0x00919191, 0x00929292, 0x00939393,
	0x00949494, 0x00959595, 0x00969696, 0x00979797,
	0x00989898, 0x00999999, 0x009a9a9a, 0x009b9b9b,
	0x009c9c9c, 0x009d9d9d, 0x009e9e9e, 0x009f9f9f,
	0x00a0a0a0, 0x00a1a1a1, 0x00a2a2a2, 0x00a3a3a3,
	0x00a4a4a4, 0x00a5a5a5, 0x00a6a6a6, 0x00a7a7a7,
	0x00a8a8a8, 0x00a9a9a9, 0x00aaaaaa, 0x00ababab,
	0x00acacac, 0x00adadad, 0x00aeaeae, 0x00afafaf,
	0x00b0b0b0, 0x00b1b1b1, 0x00b2b2b2, 0x00b3b3b3,
	0x00b4b4b4, 0x00b5b5b5, 0x00b6b6b6, 0x00b7b7b7,
	0x00b8b8b8, 0x00b9b9b9, 0x00bababa, 0x00bbbbbb,
	0x00bcbcbc, 0x00bdbdbd, 0x00bebebe, 0x00bfbfbf,
	0x00c0c0c0, 0x00c1c1c1, 0x00c2c2c2, 0x00c3c3c3,
	0x00c4c4c4, 0x00c5c5c5, 0x00c6c6c6, 0x00c7c7c7,
	0x00c8c8c8, 0x00c9c9c9, 0x00cacaca, 0x00cbcbcb,
	0x00cccccc, 0x00cdcdcd, 0x00cecece, 0x00cfcfcf,
	0x00d0d0d0, 0x00d1d1d1, 0x00d2d2d2, 0x00d3d3d3,
	0x00d4d4d4, 0x00d5d5d5, 0x00d6d6d6, 0x00d7d7d7,
	0x00d8d8d8, 0x00d9d9d9, 0x00dadada, 0x00dbdbdb,
	0x00dcdcdc, 0x00dddddd, 0x00dedede, 0x00dfdfdf,
	0x00e0e0e0, 0x00e1e1e1, 0x00e2e2e2, 0x00e3e3e3,
	0x00e4e4e4, 0x00e5e5e5, 0x00e6e6e6, 0x00e7e7e7,
	0x00e8e8e8, 0x00e9e9e9, 0x00eaeaea, 0x00ebebeb,
	0x00ececec, 0x00ededed, 0x00eeeeee, 0x00efefef,
	0x00f0f0f0, 0x00f1f1f1, 0x00f2f2f2, 0x00f3f3f3,
	0x00f4f4f4, 0x00f5f5f5, 0x00f6f6f6, 0x00f7f7f7,
	0x00f8f8f8, 0x00f9f9f9, 0x00fafafa, 0x00fbfbfb,
	0x00fcfcfc, 0x00fdfdfd, 0x00fefefe, 0x00ffffff
};

/* pixel order : RBG */
static unsigned int lcd_rgb_working_lut[256] = {
	/* default linear qlut */
	0x00000000, 0x00010101, 0x00020202, 0x00030303,
	0x00040404, 0x00050505, 0x00060606, 0x00070707,
	0x00080808, 0x00090909, 0x000a0a0a, 0x000b0b0b,
	0x000c0c0c, 0x000d0d0d, 0x000e0e0e, 0x000f0f0f,
	0x00101010, 0x00111111, 0x00121212, 0x00131313,
	0x00141414, 0x00151515, 0x00161616, 0x00171717,
	0x00181818, 0x00191919, 0x001a1a1a, 0x001b1b1b,
	0x001c1c1c, 0x001d1d1d, 0x001e1e1e, 0x001f1f1f,
	0x00202020, 0x00212121, 0x00222222, 0x00232323,
	0x00242424, 0x00252525, 0x00262626, 0x00272727,
	0x00282828, 0x00292929, 0x002a2a2a, 0x002b2b2b,
	0x002c2c2c, 0x002d2d2d, 0x002e2e2e, 0x002f2f2f,
	0x00303030, 0x00313131, 0x00323232, 0x00333333,
	0x00343434, 0x00353535, 0x00363636, 0x00373737,
	0x00383838, 0x00393939, 0x003a3a3a, 0x003b3b3b,
	0x003c3c3c, 0x003d3d3d, 0x003e3e3e, 0x003f3f3f,
	0x00404040, 0x00414141, 0x00424242, 0x00434343,
	0x00444444, 0x00454545, 0x00464646, 0x00474747,
	0x00484848, 0x00494949, 0x004a4a4a, 0x004b4b4b,
	0x004c4c4c, 0x004d4d4d, 0x004e4e4e, 0x004f4f4f,
	0x00505050, 0x00515151, 0x00525252, 0x00535353,
	0x00545454, 0x00555555, 0x00565656, 0x00575757,
	0x00585858, 0x00595959, 0x005a5a5a, 0x005b5b5b,
	0x005c5c5c, 0x005d5d5d, 0x005e5e5e, 0x005f5f5f,
	0x00606060, 0x00616161, 0x00626262, 0x00636363,
	0x00646464, 0x00656565, 0x00666666, 0x00676767,
	0x00686868, 0x00696969, 0x006a6a6a, 0x006b6b6b,
	0x006c6c6c, 0x006d6d6d, 0x006e6e6e, 0x006f6f6f,
	0x00707070, 0x00717171, 0x00727272, 0x00737373,
	0x00747474, 0x00757575, 0x00767676, 0x00777777,
	0x00787878, 0x00797979, 0x007a7a7a, 0x007b7b7b,
	0x007c7c7c, 0x007d7d7d, 0x007e7e7e, 0x007f7f7f,
	0x00808080, 0x00818181, 0x00828282, 0x00838383,
	0x00848484, 0x00858585, 0x00868686, 0x00878787,
	0x00888888, 0x00898989, 0x008a8a8a, 0x008b8b8b,
	0x008c8c8c, 0x008d8d8d, 0x008e8e8e, 0x008f8f8f,
	0x00909090, 0x00919191, 0x00929292, 0x00939393,
	0x00949494, 0x00959595, 0x00969696, 0x00979797,
	0x00989898, 0x00999999, 0x009a9a9a, 0x009b9b9b,
	0x009c9c9c, 0x009d9d9d, 0x009e9e9e, 0x009f9f9f,
	0x00a0a0a0, 0x00a1a1a1, 0x00a2a2a2, 0x00a3a3a3,
	0x00a4a4a4, 0x00a5a5a5, 0x00a6a6a6, 0x00a7a7a7,
	0x00a8a8a8, 0x00a9a9a9, 0x00aaaaaa, 0x00ababab,
	0x00acacac, 0x00adadad, 0x00aeaeae, 0x00afafaf,
	0x00b0b0b0, 0x00b1b1b1, 0x00b2b2b2, 0x00b3b3b3,
	0x00b4b4b4, 0x00b5b5b5, 0x00b6b6b6, 0x00b7b7b7,
	0x00b8b8b8, 0x00b9b9b9, 0x00bababa, 0x00bbbbbb,
	0x00bcbcbc, 0x00bdbdbd, 0x00bebebe, 0x00bfbfbf,
	0x00c0c0c0, 0x00c1c1c1, 0x00c2c2c2, 0x00c3c3c3,
	0x00c4c4c4, 0x00c5c5c5, 0x00c6c6c6, 0x00c7c7c7,
	0x00c8c8c8, 0x00c9c9c9, 0x00cacaca, 0x00cbcbcb,
	0x00cccccc, 0x00cdcdcd, 0x00cecece, 0x00cfcfcf,
	0x00d0d0d0, 0x00d1d1d1, 0x00d2d2d2, 0x00d3d3d3,
	0x00d4d4d4, 0x00d5d5d5, 0x00d6d6d6, 0x00d7d7d7,
	0x00d8d8d8, 0x00d9d9d9, 0x00dadada, 0x00dbdbdb,
	0x00dcdcdc, 0x00dddddd, 0x00dedede, 0x00dfdfdf,
	0x00e0e0e0, 0x00e1e1e1, 0x00e2e2e2, 0x00e3e3e3,
	0x00e4e4e4, 0x00e5e5e5, 0x00e6e6e6, 0x00e7e7e7,
	0x00e8e8e8, 0x00e9e9e9, 0x00eaeaea, 0x00ebebeb,
	0x00ececec, 0x00ededed, 0x00eeeeee, 0x00efefef,
	0x00f0f0f0, 0x00f1f1f1, 0x00f2f2f2, 0x00f3f3f3,
	0x00f4f4f4, 0x00f5f5f5, 0x00f6f6f6, 0x00f7f7f7,
	0x00f8f8f8, 0x00f9f9f9, 0x00fafafa, 0x00fbfbfb,
	0x00fcfcfc, 0x00fdfdfd, 0x00fefefe, 0x00ffffff
};

/* pixel order : sRBG */
//static unsigned int lcd_srgb_preset_lut[256] = {
	/* default linear qlut */
/*
	0x00000000, 0x00141414, 0x001c1c1c, 0x00212121,
	0x00262626, 0x002a2a2a, 0x002e2e2e, 0x00313131,
	0x00343434, 0x00373737, 0x003a3a3a, 0x003d3d3d,
	0x003f3f3f, 0x00414141, 0x00444444, 0x00464646,
	0x00484848, 0x004a4a4a, 0x004c4c4c, 0x004e4e4e,
	0x00505050, 0x00515151, 0x00535353, 0x00555555,
	0x00575757, 0x00585858, 0x005a5a5a, 0x005b5b5b,
	0x005d5d5d, 0x005e5e5e, 0x00606060, 0x00616161,
	0x00636363, 0x00646464, 0x00666666, 0x00676767,
	0x00686868, 0x006a6a6a, 0x006b6b6b, 0x006c6c6c,
	0x006d6d6d, 0x006f6f6f, 0x00707070, 0x00717171,
	0x00727272, 0x00737373, 0x00757575, 0x00767676,
	0x00777777, 0x00787878, 0x00797979, 0x007a7a7a,
	0x007b7b7b, 0x007c7c7c, 0x007d7d7d, 0x007e7e7e,
	0x00808080, 0x00818181, 0x00828282, 0x00838383,
	0x00848484, 0x00858585, 0x00868686, 0x00878787,
	0x00888888, 0x00888888, 0x00898989, 0x008a8a8a,
	0x008b8b8b, 0x008c8c8c, 0x008d8d8d, 0x008e8e8e,
	0x008f8f8f, 0x00909090, 0x00919191, 0x00929292,
	0x00939393, 0x00939393, 0x00949494, 0x00959595,
	0x00969696, 0x00979797, 0x00989898, 0x00999999,
	0x00999999, 0x009a9a9a, 0x009b9b9b, 0x009c9c9c,
	0x009d9d9d, 0x009e9e9e, 0x009e9e9e, 0x009f9f9f,
	0x00a0a0a0, 0x00a1a1a1, 0x00a2a2a2, 0x00a2a2a2,
	0x00a3a3a3, 0x00a4a4a4, 0x00a5a5a5, 0x00a5a5a5,
	0x00a6a6a6, 0x00a7a7a7, 0x00a8a8a8, 0x00a8a8a8,
	0x00a9a9a9, 0x00aaaaaa, 0x00ababab, 0x00ababab,
	0x00acacac, 0x00adadad, 0x00aeaeae, 0x00aeaeae,
	0x00afafaf, 0x00b0b0b0, 0x00b0b0b0, 0x00b1b1b1,
	0x00b2b2b2, 0x00b2b2b2, 0x00b3b3b3, 0x00b4b4b4,
	0x00b5b5b5, 0x00b5b5b5, 0x00b6b6b6, 0x00b7b7b7,
	0x00b7b7b7, 0x00b8b8b8, 0x00b9b9b9, 0x00b9b9b9,
	0x00bababa, 0x00bbbbbb, 0x00bbbbbb, 0x00bcbcbc,
	0x00bdbdbd, 0x00bdbdbd, 0x00bebebe, 0x00bebebe,
	0x00bfbfbf, 0x00c0c0c0, 0x00c0c0c0, 0x00c1c1c1,
	0x00c2c2c2, 0x00c2c2c2, 0x00c3c3c3, 0x00c4c4c4,
	0x00c4c4c4, 0x00c5c5c5, 0x00c5c5c5, 0x00c6c6c6,
	0x00c7c7c7, 0x00c7c7c7, 0x00c8c8c8, 0x00c8c8c8,
	0x00c9c9c9, 0x00cacaca, 0x00cacaca, 0x00cbcbcb,
	0x00cbcbcb, 0x00cccccc, 0x00cdcdcd, 0x00cdcdcd,
	0x00cecece, 0x00cecece, 0x00cfcfcf, 0x00d0d0d0,
	0x00d0d0d0, 0x00d1d1d1, 0x00d1d1d1, 0x00d2d2d2,
	0x00d2d2d2, 0x00d3d3d3, 0x00d4d4d4, 0x00d4d4d4,
	0x00d5d5d5, 0x00d5d5d5, 0x00d6d6d6, 0x00d6d6d6,
	0x00d7d7d7, 0x00d8d8d8, 0x00d8d8d8, 0x00d9d9d9,
	0x00d9d9d9, 0x00dadada, 0x00dadada, 0x00dbdbdb,
	0x00dbdbdb, 0x00dcdcdc, 0x00dcdcdc, 0x00dddddd,
	0x00dedede, 0x00dedede, 0x00dfdfdf, 0x00dfdfdf,
	0x00e0e0e0, 0x00e0e0e0, 0x00e1e1e1, 0x00e1e1e1,
	0x00e2e2e2, 0x00e2e2e2, 0x00e3e3e3, 0x00e3e3e3,
	0x00e4e4e4, 0x00e4e4e4, 0x00e5e5e5, 0x00e5e5e5,
	0x00e6e6e6, 0x00e6e6e6, 0x00e7e7e7, 0x00e7e7e7,
	0x00e8e8e8, 0x00e8e8e8, 0x00e9e9e9, 0x00e9e9e9,
	0x00eaeaea, 0x00eaeaea, 0x00ebebeb, 0x00ebebeb,
	0x00ececec, 0x00ececec, 0x00ededed, 0x00ededed,
	0x00eeeeee, 0x00eeeeee, 0x00efefef, 0x00efefef,
	0x00f0f0f0, 0x00f0f0f0, 0x00f1f1f1, 0x00f1f1f1,
	0x00f2f2f2, 0x00f2f2f2, 0x00f3f3f3, 0x00f3f3f3,
	0x00f4f4f4, 0x00f4f4f4, 0x00f5f5f5, 0x00f5f5f5,
	0x00f6f6f6, 0x00f6f6f6, 0x00f7f7f7, 0x00f7f7f7,
	0x00f8f8f8, 0x00f8f8f8, 0x00f9f9f9, 0x00f9f9f9,
	0x00f9f9f9, 0x00fafafa, 0x00fafafa, 0x00fbfbfb,
	0x00fbfbfb, 0x00fcfcfc, 0x00fcfcfc, 0x00fdfdfd,
	0x00fdfdfd, 0x00fefefe, 0x00fefefe, 0x00ffffff
};
*/

void static resetWorkingLut(void)
{
	memcpy((void *)lcd_rgb_working_lut, (void *)lcd_rgb_linear_lut,
		sizeof(lcd_rgb_linear_lut));
}

void static updateLUT(unsigned int lut_val, unsigned int color,
			unsigned int posn)
{
	int offset, mask;

	if (lut_val > 0xff)
		return;

	if (color == 0) {
		offset = 16;
		mask = 0x0000ffff;
	} else if (color == 1) {
		offset = 8;
		mask = 0x00ff00ff;
	} else if (color == 2) {
		offset = 0;
		mask = 0x00ffff00;
	} else
		// bad color select!
		return;

	lcd_rgb_working_lut[posn] = (lcd_rgb_working_lut[posn] & mask) |
					(lut_val << offset); 
}

static ssize_t kgamma_store(struct device *dev, struct device_attribute *attr,
                                                const char *buf, size_t count)
{
	int lut, color, posn, key, tmp;

	if (!count)
		return -EINVAL;

	sscanf(buf, "%u %u %u %u", &lut, &color, &posn, &key);

	if (lut > 0xff)
		return count;

	if (posn > 0xff)
		return count;

	if (color > 2)
		return count;

	tmp = (lut + color + posn) & 0xff;
	if (key == tmp) {
		updateLUT(lut, color, posn);
		lut_updated = true;
	}
	return count;
}

static ssize_t kgamma_show(struct device *dev, struct device_attribute *attr,
                                                                char *buf)
{
	int res = 0;

	if (lut_updated) {
		res = sprintf(buf, "OK\n");
		lut_updated = false;
	} else 
		res = sprintf(buf, "NG\n");

	return res;

}

static ssize_t kgamma_reset_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int reset;

	if (!count)
		return -EINVAL;

	sscanf(buf, "%u", &reset);

	if (reset)
		resetWorkingLut();

	return count;
}

static ssize_t kgamma_reset_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return 0;
}

static struct kcal_data kcal_value = {255, 255, 255};

static int update_lcdc_lut(void)
{
	struct fb_cmap cmap;
	int ret = 0;

	cmap.start = 0;
	cmap.len = 256;
	cmap.transp = NULL;

	cmap.red = (uint16_t *)&(kcal_value.red);
	cmap.green = (uint16_t *)&(kcal_value.green);
	cmap.blue = (uint16_t *)&(kcal_value.blue);

	ret = mdp_preset_lut_update_lcdc(&cmap, lcd_rgb_working_lut);

	//if (ret)
	//	pr_err("%s: failed to set lut! %d\n", __func__, ret);

	return ret;
}

static int kcal_set_values(int kcal_r, int kcal_g, int kcal_b)
{
        kcal_value.red = kcal_r;
        kcal_value.green = kcal_g;
        kcal_value.blue = kcal_b;
        return 0;
}

static int kcal_get_values(int *kcal_r, int *kcal_g, int *kcal_b)
{
        *kcal_r = kcal_value.red;
        *kcal_g = kcal_value.green;
        *kcal_b = kcal_value.blue;
        return 0;
}

static int kcal_refresh_values(void)
{
        return update_lcdc_lut();
}

static bool calc_checksum(unsigned int a, unsigned int b,
			unsigned int c, unsigned int d)
{
	unsigned char chksum = 0;

	chksum = ~((a & 0xff) + (b & 0xff) + (c & 0xff));

	if (chksum == (d & 0xff)) {
		return true;
	} else {
		return false;
	}
}

static ssize_t kcal_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	int kcal_r = 0;
	int kcal_g = 0;
	int kcal_b = 0;
	int chksum = 0;

	if (!count)
		return -EINVAL;

	sscanf(buf, "%d %d %d %d", &kcal_r, &kcal_g, &kcal_b, &chksum);
	chksum = (chksum & 0x0000ff00) >> 8;

	if (calc_checksum(kcal_r, kcal_g, kcal_b, chksum))
		kcal_ctrl_pdata->set_values(kcal_r, kcal_g, kcal_b);
	return count;
}

static ssize_t kcal_show(struct device *dev, struct device_attribute *attr,
								char *buf)
{
	int kcal_r = 0;
	int kcal_g = 0;
	int kcal_b = 0;

	kcal_ctrl_pdata->get_values(&kcal_r, &kcal_g, &kcal_b);

	return sprintf(buf, "%d %d %d\n", kcal_r, kcal_g, kcal_b);
}

static ssize_t kcal_ctrl_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int cmd = 0;
	if (!count)
		return last_status_kcal_ctrl = -EINVAL;

	sscanf(buf, "%d", &cmd);

	if(cmd != 1)
		return last_status_kcal_ctrl = -EINVAL;

	last_status_kcal_ctrl = kcal_ctrl_pdata->refresh_display();

	if(last_status_kcal_ctrl)
		return -EINVAL;
	else
		return count;
}

static ssize_t kcal_ctrl_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	if(last_status_kcal_ctrl)
		return sprintf(buf, "NG\n");
	else
		return sprintf(buf, "OK\n");
}

static ssize_t kcal_version_show(struct device *dev,
                                struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d.%d\n",
			KCAL_CTRL_MAJOR_VERSION,
			KCAL_CTRL_MINOR_VERSION);
}

static DEVICE_ATTR(power_reset, 0644, kgamma_reset_show, kgamma_reset_store); 
static DEVICE_ATTR(power_line, 0644, kgamma_show, kgamma_store); 
static DEVICE_ATTR(power_rail, 0644, kcal_show, kcal_store);
static DEVICE_ATTR(power_rail_ctrl, 0644, kcal_ctrl_show, kcal_ctrl_store);
static DEVICE_ATTR(power_rail_version, 0444, kcal_version_show, NULL);

static int kcal_ctrl_probe(struct platform_device *pdev)
{
	int rc = 0;
	kcal_ctrl_pdata = pdev->dev.platform_data;

	if(!kcal_ctrl_pdata->set_values || !kcal_ctrl_pdata->get_values ||
					!kcal_ctrl_pdata->refresh_display) {
		//pr_err("kcal function not registered\n");
		return -1;
	}

	rc = device_create_file(&pdev->dev, &dev_attr_power_reset);
	if(rc !=0)
		return -1;
	rc = device_create_file(&pdev->dev, &dev_attr_power_line);
	if(rc !=0)
		return -1;
	rc = device_create_file(&pdev->dev, &dev_attr_power_rail);
	if(rc !=0)
		return -1;
	rc = device_create_file(&pdev->dev, &dev_attr_power_rail_ctrl);
	if(rc !=0)
		return -1;
	rc = device_create_file(&pdev->dev, &dev_attr_power_rail_version);
	if(rc !=0)
		return -1;

	return 0;
}

static struct kcal_platform_data kcal_pdata = {
	.set_values = kcal_set_values,
	.get_values = kcal_get_values,
	.refresh_display = kcal_refresh_values
};

static struct platform_device kcal_platform_device = {
	.name   = KCAL_PLATFORM_NAME,
	.dev = {
		.platform_data = &kcal_pdata,
	}
};

static struct platform_device *msm_panel_devices[] __initdata = {
	&kcal_platform_device,
};

static struct platform_driver this_driver = {
	.probe  = kcal_ctrl_probe,
	.driver = {
		.name   = KCAL_PLATFORM_NAME,
	},
};

typedef int (*funcPtr)(void);

static void msm_kcal_early_suspend(struct early_suspend *handler)
{

}

static void msm_kcal_late_resume(struct early_suspend *handler)
{
	kcal_ctrl_pdata->refresh_display();
	pr_info("msm kcal late resume update!\n");
}

static struct early_suspend msm_kcal_early_suspend_struct_driver = {
        .level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN - 20,
        .suspend = msm_kcal_early_suspend,
        .resume = msm_kcal_late_resume,
};

int __init kcal_ctrl_init(void)
{
#if 0
	struct kcal_platform_data *kcalPtr;

	addr = kallsyms_lookup_name("klcd_pdata");
	kcalPtr = (struct kcal_platform_data *)addr;
	kcalPtr->set_values = kcal_set_values;
	kcalPtr->get_values = kcal_get_values;
	kcalPtr->refresh_display = kcal_refresh_values;

#endif
	unsigned int addr;

	addr =  kallsyms_lookup_name("update_preset_lcdc_lut");
	*(funcPtr *)addr = (funcPtr)update_lcdc_lut;

	platform_add_devices(msm_panel_devices,
		ARRAY_SIZE(msm_panel_devices));

	register_early_suspend(&msm_kcal_early_suspend_struct_driver);

	//pr_info("generic kcal ctrl initialized\n");
	//pr_info("generic kcal ctrl version %d.%d\n",
	//	KCAL_CTRL_MAJOR_VERSION,
	//	KCAL_CTRL_MINOR_VERSION); 
	return platform_driver_register(&this_driver);
}

device_initcall(kcal_ctrl_init);

//MODULE_DESCRIPTION("Generic MSM KCAL driver");
MODULE_LICENSE("GPL and additional rights");


