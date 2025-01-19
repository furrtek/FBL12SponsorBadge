#include "usbd_storage_if.h"
#include "fat_data.h"

#define STORAGE_LUN_NBR                  1
#define STORAGE_BLK_NBR                  0x10000
#define STORAGE_BLK_SIZ                  0x200

/** USB Mass storage Standard Inquiry Data. */
const int8_t STORAGE_Inquirydata_FS[] = {	/* 36 */
  /* LUN 0 */
  0x00,
  0x80,
  0x02,
  0x02,
  (STANDARD_INQUIRY_DATA_LEN - 5),
  0x00,
  0x00,
  0x00,
  'F', 'E', ' ', ' ', ' ', ' ', ' ', ' ', /* Manufacturer : 8 bytes */
  'B', 'a', 'd', 'g', 'e', ' ', 'F', 'B', /* Product      : 16 Bytes */
  'L', '1', '2', ' ', ' ', ' ', ' ', ' ',
  '0', '.', '0' ,'1'                      /* Version      : 4 Bytes */
};

extern USBD_HandleTypeDef hUsbDeviceFS;

static int8_t STORAGE_Init_FS(uint8_t lun);
static int8_t STORAGE_GetCapacity_FS(uint8_t lun, uint32_t *block_num, uint16_t *block_size);
static int8_t STORAGE_IsReady_FS(uint8_t lun);
static int8_t STORAGE_IsWriteProtected_FS(uint8_t lun);
static int8_t STORAGE_Read_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_Write_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_GetMaxLun_FS(void);

USBD_StorageTypeDef USBD_Storage_Interface_fops_FS = {
	STORAGE_Init_FS,
	STORAGE_GetCapacity_FS,
	STORAGE_IsReady_FS,
	STORAGE_IsWriteProtected_FS,
	STORAGE_Read_FS,
	STORAGE_Write_FS,
	STORAGE_GetMaxLun_FS,
	(int8_t *)STORAGE_Inquirydata_FS
};

int8_t STORAGE_Init_FS(uint8_t lun) {
  return (USBD_OK);
}

int8_t STORAGE_GetCapacity_FS(uint8_t lun, uint32_t *block_num, uint16_t *block_size) {
	*block_num  = STORAGE_BLK_NBR;
	*block_size = STORAGE_BLK_SIZ;
	return (USBD_OK);
}

int8_t STORAGE_IsReady_FS(uint8_t lun) {
	return (USBD_OK);
}

int8_t STORAGE_IsWriteProtected_FS(uint8_t lun) {
	return (USBD_OK);
}

int8_t STORAGE_Read_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len) {
	if (fat32_read(buf, blk_addr << 9))
		return (USBD_FAIL);

	return (USBD_OK);
}

int8_t STORAGE_Write_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len) {
	if (fat32_write(buf, blk_addr << 9))
		return (USBD_FAIL);

	return (USBD_OK);
}

int8_t STORAGE_GetMaxLun_FS(void) {
	return (STORAGE_LUN_NBR - 1);
}

