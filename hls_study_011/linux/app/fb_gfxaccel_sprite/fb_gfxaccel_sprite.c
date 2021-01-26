/******************************************************
 *    Filename:     fb_gfxaccel_sprite.c 
 *     Purpose:     test app for graphics acceleration IP
 *  Target Plf:     ZYBO 
 *  Created on: 	2021/01/23
 *      Author: 	atsupi.com
 *     Version:		1.0
 ******************************************************/

//#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include "azplf_bsp.h"
#include "azplf_hal.h"
#include "azplf_util.h"
#include "font.h"
#include "sprite.h"

/* 
 * Data address
 * 
 * Read and write sub-frame use the same settings
 */
static u32 WriteFrameAddr[NUMBER_OF_FRAMES];
static u32 ResourceAddr; // resource buffer
static u32 mappedResAddr; // logical address
static u32 CamFrameAddr; // dummy camera buffer

// driver instances
static VdmaInstance vdmaInst_0;
static VdmaInstance vdmaInst_1;  /* dummy for testing */
static LQ070outInstance lq070Inst;
static GfxaccelInstance gfxaccelInst;

static int fbActive;
static int fbBackgd;

static u32 system_time = 0; // 1/60 sec unit
struct timeval start_time;
pthread_mutex_t mutex;

static int quit = 0;

static int scene = 0;
static int scene_change = 0;	// flag to step to next_scene
static int next_scene = 0;

static Sprite Sprite1 = {
	448, // x;
	224, // y;
	32, // dx;
	32, // dy;
	0, // src_x;
	0, // src_y;
	{ // anim
		0, // used;
		0, // num_anim;
		0, // curr_num;
		0, // reserved;
		0, // duration;
		0, // prev_time;
		{ 0 }, // src_x[8];
		{ 0 }, // src_y[8];
		{ 0 }, // msk_x[8];
		{ 0 }, // msk_y[8];
	}
};

static Sprite Sprite2 = {
	384, // x;
	224, // y;
	32, // dx;
	32, // dy;
	0, // src_x;
	0, // src_y;
	{ // anim
		1, // used;
		2, // num_anim;
		0, // curr_num;
		0, // reserved;
		30, // duration; 1/60s unit
		0, // prev_time;
		{ 0, 1, -1, -1, -1, -1, -1, -1 }, // src_x[8]; put -1 if not used
		{ 2, 2, -1, -1, -1, -1, -1, -1 }, // src_y[8]; put -1 if not used
		{ 0, 1, -1, -1, -1, -1, -1, -1 }, // msk_x[8]; put -1 if not used
		{ 1, 1, -1, -1, -1, -1, -1, -1 }, // msk_y[8]; put -1 if not used
	}
};

/******************* Function Prototypes ************************************/

static int ReadSetup(VdmaInstance *inst);

void drawTrianglePolygons(int fbNum)
{
	pos points[] = {
		{ 100, 100 },
		{ 160, 100 },
		{ 130, 150 },
		{ 190, 150 },
		{ 160, 200 },
		{ 220, 200 },
		{ 190, 250 },
		{ 250, 250 },
		{ 220, 200 },
		{ 280, 200 },
		{ 250, 250 },
		{ 310, 250 },
		{ 280, 300 },
		{ 340, 300 },
		{ 310, 350 },
		{ 370, 350 },
		{ 340, 300 },
		{ 400, 300 },
		{ 370, 250 },
		{ 430, 250 },
		{ 400, 200 },
		{ 460, 200 },
		{ 430, 150 },
		{ 490, 150 },
		{ 460, 100 },
		{ 520, 100 },
	};
	int i;

	gfxaccel_draw_line(&gfxaccelInst, WriteFrameAddr[fbNum], 
		points[0].x, points[0].y, points[1].x, points[1].y, 
		0xffffffff);
	for (i = 0; i < sizeof(points)/sizeof(points[0]) - 2; i++)
	{
		gfxaccel_draw_line(&gfxaccelInst, WriteFrameAddr[fbNum], 
			points[i+1].x, points[i+1].y, points[i+2].x, points[i+2].y, 
			0xffffffff);
		gfxaccel_draw_line(&gfxaccelInst, WriteFrameAddr[fbNum], 
			points[i+2].x, points[i+2].y, points[i].x, points[i].y, 
			0x3ff00000);
	}
}

static void fillColorTiles(u32 baseAddr)
{
	int i, j;
	unsigned int col, data;

	for (i = 0; i < 480 / 16; i++) {
		for (j = 0; j < 800 / 16; j++) {
			col = ((i) << 2) | (j);
			data = RGB1(col >> 2, col >> 1, col);
			gfxaccel_fill_rect(&gfxaccelInst, baseAddr, 
				j * 16, i * 16, j * 16 + 15, i * 16 + 15, 
				data);
		}
	}
}

static void copyFramebufferToBitmap(u32 *dst, u32 *src, int width, int height, int stride)
{
	int x, y;
	u32 pix;
	u32 *ptr = src;
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pix = ptr[x];
			// Shift VDMA bitmap pixel data to RGB bitmap
			pix = ((pix & 0x3FC00000) >> 6) | ((pix & 0xFF000) >> 4) | ((pix & 0x3FC) >> 2);
			*dst++ = pix;
		}
		ptr += stride;
	}
}

static void copyBitmapToFramebuffer(u32 *dst, u32 *src, int width, int height, int stride)
{
	int x, y;
	u32 pix;
	u32 *ptr = src;
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pix = ptr[x];
			// Shift RGB bitmap to VDMA bitmap pixel data
			pix = ((pix & 0xFF0000) << 6) | ((pix & 0xFF00) << 4) | ((pix & 0xFF) << 2);
			*dst++ = pix;
		}
		ptr += stride;
	}
}

static void saveFBtoBitmapFile(u32 baseAddr)
{
	Bitmap bmp;
	loadBitmapFile("base.bmp", &bmp);
	copyFramebufferToBitmap(bmp.data, 
		(u32 *)vdma_get_frame_address(&vdmaInst_0, 0), 
		DISP_WIDTH, DISP_HEIGHT, DISP_WIDTH);
	saveBitmapFile(&bmp, "result.bmp");
	free(bmp.data);
}

static void loadBitmapFiletoFB(u32 baseAddr)
{
	Bitmap bmp;
	printf("load bitmap file res/resource.bmp ...\n");
	loadBitmapFile("./res/resource.bmp", &bmp);
//	loadBitmapFile("resource.bmp", &bmp);
	printf("done.\n");
	printf("copy bitmap to resource frame buffer ...\n");
	copyBitmapToFramebuffer((u32 *)baseAddr, bmp.data, 
		DISP_WIDTH, DISP_HEIGHT, DISP_WIDTH);
	printf("done.\n");
	free(bmp.data);
}

static u32 mapResourceFBtoMem(u32 baseAddr)
{
	int fd;
	u32 result;
	u32 frame_page = 0x00200000; // 2MB page

	printf("In mapResourceFBtoMem()\n");
	printf("File page size=0x%08x (%dKB)\n", frame_page, frame_page>>10);

	fd = open("/dev/mem", O_RDWR);
	result = (u32)mmap(NULL, frame_page, PROT_READ|PROT_WRITE, MAP_SHARED, fd, baseAddr);
	printf("Mapping I/O: 0x%08x to vmem: 0x%08x\n", baseAddr, result);
	close(fd);

	if (result == 0 || result == 0xFFFFFFFF)
	{
		printf("Error: Mapping failure\n");
		return;
	}

	return (result);
}

static int parse_argument(int argc, char *argv[])
{
	int mode = 0;
	int i;

	for (i = 0; i < argc; i++)
	{
		if (*argv[i] == '-' && *(argv[i]+1) == 's')
		{
			printf("  Shot mode enabled.\n");
			mode = 1;
			break;
		}
		else if (*argv[i] == '-' && *(argv[i]+1) == 'd')
		{
			printf("  Debug mode enabled.\n");
			mode = 2;
			break;
		}
	}

	return (mode);
}

static int ProcessTime(void)
{
	static u32 old_time = 0;
	struct timeval curr_time;
	float duration; // us unit

	pthread_mutex_lock(&mutex);
	gettimeofday(&curr_time, NULL);
//	printf("current time=%d:%d\n", (int)curr_time.tv_sec, (int)curr_time.tv_usec);
	duration = (curr_time.tv_sec - start_time.tv_sec) * 1000000.0 + (curr_time.tv_usec - start_time.tv_usec);
	duration -= system_time * 16000.0;

	system_time += duration / 16000.0;
	pthread_mutex_unlock(&mutex);

	if (system_time - old_time >= 59) {
//		printf("%d\n", system_time / 60);
		old_time = system_time;
	}
	if (duration < 60 * 1000 && duration >= 16) {
		return (system_time);
	}

	return 0;
}

static void DrawTestFrame(int fbNum)
{
	int i, j;

    for (i = 0; i < 480 / 16; i++)
    {
    	for (j = 0; j < 800 / 16; j++)
    	{
    		gfxaccel_bitblt(&gfxaccelInst, 
				ResourceAddr, j * 16, (i /*% 15*/) * 16, 16, 16, 
				WriteFrameAddr[fbNum], j * 16, i * 16, 
				GFXACCEL_BB_NONE);
    	}
    }

    // Invoke fill rectangle accelerator
    gfxaccel_fill_rect(&gfxaccelInst, WriteFrameAddr[fbNum],
		 80,  80, 719, 399, RGB8(255, 255, 255));
    // Invoke fill rectangle accelerator
    gfxaccel_fill_rect(&gfxaccelInst, WriteFrameAddr[fbNum],
		 83,  83, 716, 396, RGB8(0, 64, 255));
    // Invoke fill rectangle accelerator
    gfxaccel_fill_rect(&gfxaccelInst, WriteFrameAddr[fbNum],
		 80, 240, 719, 241, RGB8(255, 255, 255));
    // Invoke fill rectangle accelerator
    gfxaccel_fill_rect(&gfxaccelInst, WriteFrameAddr[fbNum],
		480, 240, 481, 399, RGB8(255, 255, 255));
}

static void UpdateFrame(void)
{
	static int x = 0;
	static int y = 0;
	int status;

	// draw backfround frame as per scene number
	if (scene == 0)
	{
		DrawTestFrame(fbBackgd);
		drawTrianglePolygons(fbBackgd);
		drawText(WriteFrameAddr[fbBackgd], 240, 448, "2021 (c) ATSUPI.COM");
	}
	else
	{
		gfxaccel_bitblt(&gfxaccelInst, 
			ResourceAddr, 0, 0, 800, 480, 
			WriteFrameAddr[fbBackgd], 0, 0, GFXACCEL_BB_NONE);
		gfxaccel_fill_rect(&gfxaccelInst, WriteFrameAddr[fbBackgd], 
			x * 32, y * 32, x * 32 + 63, y * 32 + 63, 
			RGB8(255, 255, 255));
	    if (++x == 24)
	    {
	    	x = 0;
	    	if (++y == 14) y = 0;
	    }
		drawSprite(&Sprite1, WriteFrameAddr[fbBackgd], system_time);
		drawSprite(&Sprite2, WriteFrameAddr[fbBackgd], system_time);
	}

	// switch display frame on double buffer
	fbActive ^= 1;
	fbBackgd ^= 1;
	status = vdma_start_parking(&vdmaInst_0, VDMA_READ, fbActive);
	if (status != PST_SUCCESS) {
		printf("Start Park failed\r\n");
		return;
	}
#ifdef DEBUG
	printf("current frame = %d\r\n", fbActive);
#endif
}

static int SceneChangeCheck(void)
{
	int update = 0;

	pthread_mutex_lock(&mutex);
	if (scene_change) {
		scene = next_scene;
		scene_change = 0;
		update = 1;
	}
	pthread_mutex_unlock(&mutex);

	if (update) printf("Scene changed to %d\n", scene);
	return (update);
}

void *thread1(void *arg)
{
	gettimeofday(&start_time, NULL);

	printf("Loop starts.\r\n");
	while (1) {
		if (ProcessTime()) {
			SceneChangeCheck();
			UpdateFrame();
		}
		if (quit) break;
		usleep(5000); // 5ms wait
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int i, j;
	u32 size = FRAME_HORIZONTAL_LEN * FRAME_VERTICAL_LEN;
	int status;
	int mode;
	u32 ReadAddr;
	pthread_t pt;
	char ch = ' ';
	pos basePos;

	mode = parse_argument(argc, argv);

	WriteFrameAddr[0] = WRITE_ADDRESS_BASE;
	WriteFrameAddr[1] = WRITE_ADDRESS_BASE + frame_page;
	ResourceAddr      = WriteFrameAddr[1]  + frame_page;
	CamFrameAddr      = ResourceAddr + frame_page;

	fbActive = 0;
	fbBackgd = 1;

	printf("\r\n--- Entering main() --- \r\n");

	/* The information of the XAxiVdma_Config comes from hardware build.
	 * The user IP should pass this information to the AXI DMA core.
	 */
	printf("Step 0\n");
	vdma_init(&vdmaInst_0, VDMA_0_BASEADDRESS, WriteFrameAddr[0]);
	if (VDMA_INSTANCE_NUM > 1) {
//		vdma_init(&vdmaInst_1, VDMA_1_BASEADDRESS, CamFrameAddr);
	}

	printf("Step 1\n");
	/* Setup the read channel
	 */
	status = ReadSetup(&vdmaInst_0);
	if (status != PST_SUCCESS) {
		printf("Read channel setup failed %d\r\n", status);
		if(status == PST_VDMA_MISMATCH_ERROR)
			printf("DMA Mismatch Error\r\n");
		return PST_FAILURE;
	}
	printf("Step 2\n");

	// Start DMA engine to transfer
	status = vdma_start(&vdmaInst_0, VDMA_READ);
	if (status != PST_SUCCESS) {
		printf("Start read transfer failed %d\r\n", status);
		return PST_FAILURE;
	}

	status = vdma_start_parking(&vdmaInst_0, VDMA_READ, fbActive);
	if (status != PST_SUCCESS) {
		printf("Start Park failed\r\n");
		return PST_FAILURE;
	}

	// map Resource frambuffer address
	mappedResAddr = mapResourceFBtoMem(ResourceAddr);
	printf("Test passed\r\n");

	lq070out_init(&lq070Inst, LQ070OUT_BASE_ADDR);
    lq070out_setmode(&lq070Inst, LQ070_PG_NONE);
    printf("Hello World\n\r");

    printf("Initialize gfxaccel instance\r\n");
	if (gfxaccel_init(&gfxaccelInst, GFXACCEL_BASE_ADDR) == PST_FAILURE) {
		return PST_FAILURE;
	}

    // Clear frame buffer
    printf("Clear frame buffer\r\n");
    gfxaccel_fill_rect(&gfxaccelInst, WriteFrameAddr[0], 0, 0, 799, 479, 0x0);

    // Setup Resource frame
    printf("Setup Resource frame buffer\r\n");
    fillColorTiles(ResourceAddr);

    printf("BitBlt Resource frame buffer to main frame buffer\r\n");
	DrawTestFrame(fbActive);

    // Invalidate frame buffer to reload updated data

    // Output results
	ReadAddr = vdma_get_frame_address(&vdmaInst_0, 0);
    printf("Read Frame address: 0x%x", ReadAddr);
    for (i = 0; i < 8; i++)
    {
    	printf("\r\nfb(%03d): ", i);
    	for (j = 0; j < 16; j++)
		{
    		printf("%02x ", FB(ReadAddr, ((i+76)*800+(j+76))) & 0xFF);
		}
    }
	printf("\r\n");
	saveFBtoBitmapFile(ReadAddr);

	drawTrianglePolygons(fbActive);

	// configure sprite drawing module
	printf("loadBitmapFiletoFB()\n");
	loadBitmapFiletoFB(mappedResAddr);

	printf("configure Sprite Resource\n");
	basePos.x = 512;
	basePos.y = 0;
	configSpriteResouce(&gfxaccelInst, ResourceAddr, &basePos);

	printf("configure Font Resource\n");
	basePos.x = 256;
	basePos.y = 0;
	configFontResouce(&gfxaccelInst, ResourceAddr, &basePos);

	sleep(1); // 1sec wait before starting graphics work thread

	// create mutex for thread handling
	pthread_mutex_init(&mutex);
	// create graphics worker thread
	pthread_create(&pt, NULL, &thread1, NULL);

	while (1) {
		do {
			scanf("%c", &ch);
		} while (ch == '\r');

		switch (scene) {
		case 0:
			if (ch == 'x') {
				quit = 1; // quit thread
				sleep(2); // wait until thread terminates
				break;
			} else if (ch == ' ' || ch == 'n') {
				pthread_mutex_lock(&mutex);
				scene_change = 1;
				next_scene   = 1;
				pthread_mutex_unlock(&mutex);
			} else {
				printf("Command not found\r\n");
			}
			break;
		case 1:
			if (ch == 0x1b || ch == 'e') { // ESC
				pthread_mutex_lock(&mutex);
				scene_change = 1;
				next_scene   = 0;
				pthread_mutex_unlock(&mutex);
			} else {
				printf("Command not found\r\n");
			}
			break;
		default:
			break;
		}
		if (quit) break;
		pthread_mutex_lock(&mutex);
		printf("system time=%d\n", system_time);
		pthread_mutex_unlock(&mutex);
		sleep(1);
	}
    printf("--- Exiting main() --- \r\n");
	pthread_mutex_destroy(&mutex);
	munmap((void *)mappedResAddr, frame_page);
	gfxaccel_deinit(&gfxaccelInst);
	lq070out_deinit(&lq070Inst);
	vdma_deinit(&vdmaInst_0);

	return 0;
}

/*****************************************************************************
 *
 * This function sets up the read channel
 *
 *****************************************************************************/
static int ReadSetup(VdmaInstance *inst)
{
	int Index;
	u32 Addr;
	int status;

	printf("Step 2.1\n");
	status = vdma_config(inst, VDMA_READ);
	if (status != PST_SUCCESS) {
		printf("Read channel config failed %d\r\n", status);
		return PST_FAILURE;
	}

	printf("Step 2.2\n");
	// Initialize buffer addresses (physical addresses)
	Addr = READ_ADDRESS_BASE;
	for(Index = 0; Index < NUMBER_OF_FRAMES; Index++) {
		vdma_set_frame_address(inst, Index, Addr);
		Addr += frame_page;
	}

	printf("Step 2.3\n");
	// Set the buffer addresses for transfer in the DMA engine
	status = vdma_dma_setbuffer(inst, VDMA_READ);
	if (status != PST_SUCCESS) {
		printf("Read channel set buffer address failed %d\r\n", status);
		return PST_FAILURE;
	}

	printf("Step 2.4\n");
	return PST_SUCCESS;
}
