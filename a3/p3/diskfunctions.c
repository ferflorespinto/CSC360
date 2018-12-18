#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "diskfunctions.h"


/*
 * Returns the total size of the disk.
 * The number of bytes per sector in FAT12 should be 512 in FAT12, but as per
 * provided suggestions we should rely on the information found in the boot
 * sector, and so we retrieve this number from it.
 */
int get_total_disk_size(char* ptr) {
	int total_sector_count = ptr[19] + (ptr[20] << 8);
	int bytes_per_sector = ptr[11] + (ptr[12] << 8);
	return total_sector_count * bytes_per_sector;
}

int get_fat_entry(int n, char* ptr) {
	int result;
	int first_byte;
	int second_byte;

	if ((n % 2) == 0) {
		first_byte = ptr[SECTOR_SIZE + ((3 * n) / 2) + 1] & 0x0F;
		second_byte = ptr[SECTOR_SIZE + ((3 * n) / 2)] & 0xFF;
		result = (first_byte << 8) + second_byte;

	}
	else {
		first_byte = ptr[SECTOR_SIZE + ((3 * n) / 2)] & 0xF0;
		second_byte = ptr[SECTOR_SIZE + ((3 * n) / 2) + 1] & 0xFF;
		result = (first_byte >> 4) + (second_byte << 4);
	}

	return result;
}

/*
 * Returns how much free space is left in the disk.
 * If the FAT entry in a directory is 0x00, that means that sector is free.
*/
int get_free_disk_size(int disk_size, char* ptr) {
	int free_sectors = 0;
	int num_sectors = ptr[19] + (ptr[20] << 8);
	int temp_entry;
	int i;

	for (i = 19; i < num_sectors; i++) {
		temp_entry = get_fat_entry(i, ptr);
		if (temp_entry == 0x00) {
			free_sectors++;
		}
	}
	return free_sectors * SECTOR_SIZE;
}
