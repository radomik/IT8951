#include <string.h>
#include "miniGUI.h"

extern IT8951DevInfo gstI80DevInfo;

void EPD_Clear(Device *device, uint8_t color) {
	memset(device->gpFrameBuf, color, Device_width(device) * Device_height(device));
}

void EPD_DrawPixel(Device *device, uint16_t x0, uint16_t y0, uint8_t color)
{
	if (x0 < 0 || y0 < 0  || x0 >= Device_width(device) || y0 >= Device_height(device)) {
		return;
	}

	device->gpFrameBuf[y0*Device_width(device) + x0] = color;
}

static void Bitmap_format_Matrix(BmpBuffer *buffer)
{
	uint32_t i,j,k;
    uint8_t *psrc = buffer->src_buf;
    uint8_t *pdst = buffer->dst_buf;
    uint8_t *p = psrc;
	uint8_t temp;
	uint32_t count;

	//由于bmp存储是从后面往前面，所以需要倒序进行转换
	switch(buffer->bits_per_pixel)
	{
		case 1:
			pdst += (buffer->width * buffer->height);

			for (i=0; i<buffer->height; i++)
			{
				pdst -= buffer->width;
				count = 0;
				for (j=0; j<(buffer->width+7)/8; j++)
				{
					temp = p[j];

					for (k=0; k<8; k++)
					{
						pdst[0]= ((temp & (0x80>>k)) >> (7-k));
						count++;
						pdst++;
						if (count == buffer->width)
						{
							break;
						}
					}
				}
				pdst -= buffer->width;
				p += buffer->bytes_per_line;
			}
		break;
		case 4:
			pdst += (buffer->width * buffer->height);

			for(i=0; i<buffer->height; i++)
			{
				pdst -= buffer->width;
				count = 0;
				for (j=0; j<(buffer->width+1)/2; j++)
				{
					temp = p[j];
					pdst[0]= ((temp & 0xf0) >> 4);
					count++;
					pdst++;
					if (count == buffer->width)
					{
						break;
					}

					pdst[0] = temp & 0x0f;
					count++;
					pdst++;
					if (count == buffer->width)
					{
						break;
					}
				}
				pdst -= buffer->width;
				p += buffer->bytes_per_line;
			}
		break;
		case 8:
			pdst += (buffer->width*buffer->height);
			for(i=0; i<buffer->height; i++)
			{
				p = psrc+(i+1)*buffer->bytes_per_line;
				p -= buffer->skip;
				for(j=0; j<buffer->width; j++)
				{
					pdst -= 1;
					p -= 1;
					pdst[0] = p[0];
				}
			}
		break;
		case 16:
			pdst += (buffer->width*buffer->height*2);
			for(i=0; i<buffer->height; i++)
			{
				p = psrc+(i+1)*buffer->bytes_per_line;
				p -= buffer->skip;
				for(j=0;j<buffer->width;j++)
				{
					pdst -= 2;
					p -= 2;
					pdst[0] = p[1];
					pdst[1] = p[0];
				}
			}
		break;
		case 24:
			pdst += (buffer->width*buffer->height*3);
			for(i=0; i<buffer->height; i++)
			{
				p = psrc+(i+1)*buffer->bytes_per_line;
				p -= buffer->skip;
				for(j=0; j<buffer->width; j++)
				{
					pdst -= 3;
					p -= 3;
					pdst[0] = p[2];
					pdst[1] = p[1];
					pdst[2] = p[0];
				}
			}
		break;
		case 32:
			pdst += (buffer->width*buffer->height*4);
			for(i=0; i<buffer->height; i++)
			{
				p = psrc+(i+1)*buffer->width*4;
				for(j=0; j<buffer->width; j++)
				{
					pdst -= 4;
					p -= 4;
					pdst[0] = p[2];
					pdst[1] = p[1];
					pdst[2] = p[0];
					pdst[3] = p[3];
				}
			}
		break;

		default:
		break;
	}
}

static void DrawMatrix(
	Device *device,
	const BmpBuffer *buffer,
	uint16_t Xpos,
	uint16_t Ypos
)
{
	uint16_t i, j, x, y;
	uint8_t r, g, b;
	uint8_t temp1, temp2;
	const uint32_t w = buffer->width;
	const uint32_t h = buffer->height;
	const uint8_t *buf = buffer->dst_buf;

	for (y=0,j=Ypos; y<h; y++,j++)
	{
 		for (x=0,i=Xpos; x<w; x++,i++)
		{
			switch(buffer->bits_per_pixel)
			{
				case 1:
				case 4:
				case 8:
					r = buffer->palette[buf[(y*w+x)]].red;
					g = buffer->palette[buf[(y*w+x)]].green;
					b = buffer->palette[buf[(y*w+x)]].blue;
				break;

				case 16:
					temp1 = buf[(y*w+x)*2];
					temp2 = buf[(y*w+x)*2+1];
					r = (temp1 & 0x7c)<<1;
					g = (((temp1 & 0x03) << 3 ) | ((temp2&0xe0) >> 5))<<3;
					b = (temp2 & 0x1f)<<3;
				break;

				case 24:
					r = buf[(y*w+x)*3];
					g = buf[(y*w+x)*3+1];
					b = buf[(y*w+x)*3+2];
				break;

				case 32:
					r = buf[(y*w+x)*4];
					g = buf[(y*w+x)*4+1];
					b = buf[(y*w+x)*4+2];
				break;

				default:
				break;
			}

			double gray = (r*299 + g*587 + b*114 + 500) / 1000;
			device->gpFrameBuf[j*Device_width(device) + i] = (uint8_t)gray;
		}
	}
}

uint8_t Show_bmp(
	Device *device,
	BmpBuffer *buffer,
	const char *path
) {
	BITMAPFILEHEADER FileHead;
	BITMAPINFOHEADER InfoHead;
	uint32_t total_length;
	uint8_t *buf = NULL;
	uint32_t ret = -1;

	FILE *fp = fopen(path,"rb");
	if (!fp) {
		perror("Fail to open image file");
		return 1;
	}

	ret = fread(&FileHead, sizeof(BITMAPFILEHEADER), 1, fp);
	if (ret != 1) {
		perror("Fail to read BMP header");
		fclose(fp);
		return 1;
	}

	if (FileHead.cfType != 0x4D42) {
		printf("It's not a BMP file\n");
		fclose(fp);
		return 1;
	}

	printf("\n*****************************************\n");
	printf("BMP_cfSize:%d \n", FileHead.cfSize);
 	printf("BMP_cfoffBits:%d \n", FileHead.cfoffBits);

	ret = fread((char*)&InfoHead, sizeof(BITMAPINFOHEADER), 1, fp);
	if (ret != 1) {
		perror("Fail to read BMP info");
		fclose(fp);
		return 1;
	}

	printf("BMP_ciSize:%d \n", InfoHead.ciSize);
 	printf("BMP_ciWidth:%d \n", InfoHead.ciWidth);
	printf("BMP_ciHeight:%d \n", InfoHead.ciHeight);
	printf("BMP_ciPlanes:%x \n", InfoHead.ciPlanes);
	printf("BMP_ciBitCount:%x \n", InfoHead.ciBitCount);
	printf("BMP_ciCompress:%x \n", InfoHead.ciCompress);
	printf("BMP_ciSizeImage:%x \n", InfoHead.ciSizeImage);
	printf("BMP_ciXPelsPerMeter:%x \n", InfoHead.ciXPelsPerMeter);
	printf("BMP_ciYPelsPerMeter:%x \n", InfoHead.ciYPelsPerMeter);
	printf("BMP_ciClrUsed:%x \n", InfoHead.ciClrUsed);
	printf("BMP_ciClrImportant:%x \n", InfoHead.ciClrImportant);
 	printf("*****************************************\n\n");

	total_length = FileHead.cfSize-FileHead.cfoffBits;
	buffer->bytes_per_line=((InfoHead.ciWidth*InfoHead.ciBitCount+31)>>5)<<2;
	buffer->skip=(4-((InfoHead.ciWidth*InfoHead.ciBitCount)>>3))&3;

	buffer->width = InfoHead.ciWidth;
    buffer->height = InfoHead.ciHeight;
	buffer->bits_per_pixel = InfoHead.ciBitCount;

	if (buffer->width > Device_width(device) || buffer->height > Device_height(device)) {
		printf("Image too big %u x %u > %u x %u",
			buffer->width, buffer->height, Device_width(device), Device_height(device));
		return 1;
	}

	printf("total_length:%d,%d\n", InfoHead.ciSizeImage,FileHead.cfSize-FileHead.cfoffBits);
	printf("bitsPerPixel = %d\n", InfoHead.ciBitCount);
	printf("bytes_per_line = %d\n", buffer->bytes_per_line);
	printf("imageSize = %d [%ux%u]\n", buffer->bytes_per_line*buffer->height, buffer->width, buffer->height);
	printf("skip = %d\n", buffer->skip);

	buffer->src_buf_size = total_length;
	buffer->src_buf = realloc(buffer->src_buf, total_length);

    if (!buffer->src_buf) {
		perror("Fail to allocate src_buf");
		return 1;
    }

	buffer->dst_buf_size = total_length;
	buffer->dst_buf = realloc(buffer->dst_buf, total_length);

    if (!buffer->dst_buf) {
		perror("Fail to allocate dst_buf");
		return 1;
    }

    fseek(fp, FileHead.cfoffBits, SEEK_SET);

    buf = buffer->src_buf;
    while ((ret = fread(buf, 1,  total_length, fp)) >= 0) {
        if (ret == 0) {
            usleep(100);
            continue;
        }
		buf = ((uint8_t*)buf) + ret;
        total_length = total_length - ret;
        if (total_length == 0) {
            break;
		}
    }

	switch(buffer->bits_per_pixel) {
		case 1:
			fseek(fp, 54, SEEK_SET);
			ret = fread(buffer->palette, 1, 8, fp);
			if (ret != 8) {
				perror("Fail to read palette");
				return 1;
			}

			buffer->dst_buf_size = InfoHead.ciWidth * InfoHead.ciHeight;
			buffer->dst_buf = realloc(buffer->dst_buf, buffer->dst_buf_size);

			if (!buffer->dst_buf) {
				perror("Fail to allocate dst_buf");
				return 1;
			}
		break;

		case 4:
			fseek(fp, 54, SEEK_SET);
			ret = fread(buffer->palette, 1, 4*16, fp);
			if (ret != 64) {
				perror("Fail to read palette");
				return 1;
			}

			buffer->dst_buf_size = InfoHead.ciWidth * InfoHead.ciHeight;
			buffer->dst_buf = realloc(buffer->dst_buf, buffer->dst_buf_size);

			if (!buffer->dst_buf) {
				perror("Fail to allocate dst_buf");
				return 1;
			}
		break;

		case 8:
			fseek(fp, 54, SEEK_SET);

			ret = fread(buffer->palette, 1, 4*256, fp);

			if (ret != 1024) {
				perror("Fail to read palette");
				return 1;
			}
		break;

		default:
		break;
	}

	fclose(fp);

	Bitmap_format_Matrix(buffer);

	uint32_t x = (Device_width(device) - buffer->width) / 2;
	uint32_t y = (Device_height(device) - buffer->height) / 2;
	printf("Display at (x0,y0) = (%u,%u)\n", x, y);
	DrawMatrix(device, buffer, x, y);

	return 0;
}

void BmpBuffer_cleanup(BmpBuffer *buffer) {
	free(buffer->src_buf);
	free(buffer->dst_buf);
	memset(buffer, 0, sizeof(*buffer));
}
