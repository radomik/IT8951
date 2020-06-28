#ifndef _miniGUI_H_
#define _miniGUI_H_

#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "IT8951.h"
#include "types.h"

typedef struct {
  uint16_t X;
  uint16_t Y;
} Point, *pPoint;

//14byte文件头
typedef struct {
	uint16_t cfType;//文件类型，"BM"(0x4D42)
	uint32_t cfSize;//文件大小（字节）
	uint32_t cfReserved;//保留，值为0
	uint32_t cfoffBits;//数据区相对于文件头的偏移量（字节）
}__attribute__((packed)) BITMAPFILEHEADER;
//__attribute__((packed))的作用是告诉编译器取消结构在编译过程中的优化对齐

//40byte信息头
typedef struct {
	uint32_t ciSize;//40
	uint32_t ciWidth;//宽度
	uint32_t ciHeight;//高度
	uint16_t ciPlanes;//目标设备的位平面数，值为1
	uint16_t ciBitCount;//每个像素的位数
	uint32_t ciCompress;//压缩说明
	uint32_t ciSizeImage;//用字节表示的图像大小，该数据必须是4的倍数
	uint32_t ciXPelsPerMeter;//目标设备的水平像素数/米
	uint32_t ciYPelsPerMeter;//目标设备的垂直像素数/米
	uint32_t ciClrUsed;//位图使用调色板的颜色数
	uint32_t ciClrImportant;//指定重要的颜色数，当该域的值等于颜色数时（或者等于0时），表示所有颜色都一样重要
}__attribute__((packed)) BITMAPINFOHEADER;

typedef struct {
	uint8_t blue;
	uint8_t green;
	uint8_t red;
	uint8_t reserved;
}__attribute__((packed)) PIXEL;//颜色模式RGB


struct BmpBuffer {
	uint32_t src_buf_size; // set to total_length after realloc of below buffers
	uint32_t dst_buf_size; // set to total_length after realloc of below buffers
	uint8_t *src_buf; // buffer of size prev_total_length or NULL initially
	uint8_t *dst_buf; // buffer of size prev_total_length or NULL initially

	uint32_t width;
	uint32_t height;
	uint32_t bits_per_pixel;
	uint32_t bytes_per_line;
	uint32_t skip;
	PIXEL    palette[256];
};

#define ABS(X)    ((X) > 0 ? (X) : -(X))

void EPD_Clear(Device *device, uint8_t color);
void EPD_DrawPixel(Device *device, uint16_t x0, uint16_t y0, uint8_t color);
uint8_t Show_bmp(
	Device *device,
	BmpBuffer *buffer,
	const char *path
);

void BmpBuffer_cleanup(BmpBuffer *buffer);

#endif
