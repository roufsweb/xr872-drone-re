/*
 * Custom Drone Camera Firmware for XR872
 * Provides a simple MJPEG stream over Wi-Fi AP.
 */

#include <stdio.h>
#include <string.h>
#include "kernel/os/os.h"
#include "common/framework/platform_init.h"
#include "common/framework/net_ctrl.h"
#include "driver/component/csi_camera/camera.h"
#include "driver/component/csi_camera/gc0328c/drv_gc0328c.h"
#include "driver/chip/hal_i2c.h"
#include "lwip/sockets.h"
#include "sys/dma_heap.h"
#include "driver/chip/hal_gpio.h"

/* Camera Sensor Pins (Adjust based on hardware if needed) */
#define SENSOR_I2C_ID 			I2C0_ID
#define SENSOR_RESET_PIN        GPIO_PIN_15
#define SENSOR_RESET_PORT       GPIO_PORT_A
#define SENSOR_POWERDOWN_PIN    GPIO_PIN_14
#define SENSOR_POWERDOWN_PORT   GPIO_PORT_A

/* Memory and Resolution */
#define JPEG_PSRAM_SIZE			(620*1024)
#define JPEG_IMAGE_WIDTH		(640)
#define JPEG_IMAGE_HEIGHT		(480)

static CAMERA_Cfg camera_cfg = {
	.jpeg_cfg.jpeg_en = 1,
	.jpeg_cfg.quality = 64,
	.jpeg_cfg.width = JPEG_IMAGE_WIDTH,
	.jpeg_cfg.height = JPEG_IMAGE_HEIGHT,
	.jpeg_cfg.jpeg_mode = JPEG_MOD_OFFLINE,
	.sensor_cfg.i2c_id = SENSOR_I2C_ID,
	.sensor_cfg.pwcfg.Pwdn_Port = SENSOR_POWERDOWN_PORT,
	.sensor_cfg.pwcfg.Reset_Port = SENSOR_RESET_PORT,
	.sensor_cfg.pwcfg.Pwdn_Pin = SENSOR_POWERDOWN_PIN,
	.sensor_cfg.pwcfg.Reset_Pin = SENSOR_RESET_PIN,
	.sensor_func.init = HAL_GC0328C_Init,
	.sensor_func.deinit = HAL_GC0328C_DeInit,
	.sensor_func.ioctl = HAL_GC0328C_IoCtl,
};

static uint8_t* gmemaddr;
static CAMERA_Mgmt mem_mgmt;

static int camera_mem_create(CAMERA_Mgmt *mgmt)
{
	uint8_t* addr = (uint8_t*)dma_malloc(JPEG_PSRAM_SIZE, DMAHEAP_PSRAM);
	if (addr == NULL) {
		printf("PSRAM malloc fail\n");
		return -1;
	}
	memset(addr, 0 , JPEG_PSRAM_SIZE);
	mgmt->yuv_buf.addr = (uint8_t *)ALIGN_16B((uint32_t)addr);
	mgmt->yuv_buf.size = camera_cfg.jpeg_cfg.width*camera_cfg.jpeg_cfg.height*3/2;
	mgmt->jpeg_buf[0].addr = (uint8_t *)ALIGN_1K((uint32_t)mgmt->yuv_buf.addr + CAMERA_JPEG_HEADER_LEN + mgmt->yuv_buf.size);
	mgmt->jpeg_buf[0].size = 50*1024;
	gmemaddr = addr;
	return 0;
}

static void stream_task(void *arg)
{
	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0) {
		printf("Socket creation failed\n");
		OS_ThreadDelete(NULL);
	}

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(8080);

	if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("Bind failed\n");
		close(listen_fd);
		OS_ThreadDelete(NULL);
	}

	listen(listen_fd, 5);
	printf("MJPEG Stream Server started on port 8080\n");

	while (1) {
		int client_fd = accept(listen_fd, NULL, NULL);
		if (client_fd < 0) continue;

		printf("Client connected\n");

		const char *header = "HTTP/1.0 200 OK\r\n"
		                     "Content-Type: multipart/x-mixed-replace;boundary=boundarydonotcross\r\n"
		                     "\r\n";
		send(client_fd, header, strlen(header), 0);

		CAMERA_JpegBuffInfo jpeg_info;
		while (1) {
			if (HAL_CAMERA_CaptureImage(CAMERA_OUT_JPEG, &jpeg_info, 1) < 0) {
				printf("Capture failed\n");
				break;
			}
			
			char frame_header[128];
			int frame_size = jpeg_info.size + CAMERA_JPEG_HEADER_LEN;
			uint8_t *addr = mem_mgmt.jpeg_buf[jpeg_info.buff_index].addr - CAMERA_JPEG_HEADER_LEN;

			sprintf(frame_header, "--boundarydonotcross\r\n"
			                      "Content-Type: image/jpeg\r\n"
			                      "Content-Length: %d\r\n"
			                      "\r\n", frame_size);
			
			if (send(client_fd, frame_header, strlen(frame_header), 0) < 0) break;
			if (send(client_fd, addr, frame_size, 0) < 0) break;
			if (send(client_fd, "\r\n", 2, 0) < 0) break;
			
			OS_MSleep(40); // ~25 FPS
		}
		printf("Client disconnected\n");
		close(client_fd);
	}
}

int main(void)
{
	platform_init();
	printf("\n--- XR872 Custom Drone Camera Firmware ---\n");

	/* Wi-Fi Access Point Setup */
	net_switch_mode(WLAN_MODE_HOSTAP);
	wlan_ap_set((unsigned char*)"XR872-Drone", 11, NULL);
	wlan_ap_enable();
	printf("Wi-Fi AP 'XR872-Drone' enabled\n");

	/* Camera Initialization */
	memset(&mem_mgmt, 0, sizeof(CAMERA_Mgmt));
	if (camera_mem_create(&mem_mgmt) != 0) {
		printf("Camera memory allocation failed\n");
		return -1;
	}
	camera_cfg.mgmt = &mem_mgmt;
	if (HAL_CAMERA_Init(&camera_cfg) != HAL_OK) {
		printf("Camera hardware init failed\n");
		return -1;
	}
	printf("Camera GC0328 initialized\n");

	/* Start Streaming Task */
	OS_Thread_t thread;
	if (OS_ThreadCreate(&thread, "stream", stream_task, NULL, OS_THREAD_PRIO_APP, 4 * 1024) != OS_OK) {
		printf("Failed to create stream task\n");
		return -1;
	}

	while (1) {
		OS_MSleep(1000);
	}
	return 0;
}
