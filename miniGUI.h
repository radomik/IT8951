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

//14byte�ļ�ͷ
typedef struct {
	uint16_t cfType;//�ļ����ͣ�"BM"(0x4D42)
	uint32_t cfSize;//�ļ���С���ֽڣ�
	uint32_t cfReserved;//������ֵΪ0
	uint32_t cfoffBits;//������������ļ�ͷ��ƫ�������ֽڣ�
}__attribute__((packed)) BITMAPFILEHEADER;
//__attribute__((packed))�������Ǹ��߱�����ȡ���ṹ�ڱ�������е��Ż�����

//40byte��Ϣͷ
typedef struct {
	uint32_t ciSize;//40
	uint32_t ciWidth;//���
	uint32_t ciHeight;//�߶�
	uint16_t ciPlanes;//Ŀ���豸��λƽ������ֵΪ1
	uint16_t ciBitCount;//ÿ�����ص�λ��
	uint32_t ciCompress;//ѹ��˵��
	uint32_t ciSizeImage;//���ֽڱ�ʾ��ͼ���С�������ݱ�����4�ı���
	uint32_t ciXPelsPerMeter;//Ŀ���豸��ˮƽ������/��
	uint32_t ciYPelsPerMeter;//Ŀ���豸�Ĵ�ֱ������/��
	uint32_t ciClrUsed;//λͼʹ�õ�ɫ�����ɫ��
	uint32_t ciClrImportant;//ָ����Ҫ����ɫ�����������ֵ������ɫ��ʱ�����ߵ���0ʱ������ʾ������ɫ��һ����Ҫ
}__attribute__((packed)) BITMAPINFOHEADER;

typedef struct {
	uint8_t blue;
	uint8_t green;
	uint8_t red;
	uint8_t reserved;
}__attribute__((packed)) PIXEL;//��ɫģʽRGB


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
