/*
 * diskinfo.c
 *
 * University of Victoria
 * CSC360 – Fall 2018
 * Assignment 3
 * Name: Jorge Flores Pinto
 * ID: V00880059
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "diskfunctions.h"

/*
 * Retrieves the name of the operating system from a disk image.
 * The starting byte of the OS name is 3. The boot sector is 8 bytes long.
 */
void get_OS_name(char* os_name, char* ptr) {
	int i;
	for (i = 0; i < 8; i++) {
		os_name[i] = ptr[i + 3];
	}
}

/*
 * Retrieves the volume label of the disk image (the entry that has the volume
 * label flag/bit set to 1).
 */
void get_disk_label(char* dl, char* ptr) {
	int i;
	ptr += SECTOR_SIZE * 19;
	while (ptr[0] != 0x00) {
		if (ptr[11] == 0x08) {
			//Found label
			for (i = 0; i < 8; i++) {
				dl[i] = ptr[i];
			}
			break;
		}
		ptr += 32;
	}
}

/*
 * Returns the file count in the disk image.
 * The following criteria is followed to determine if a file should be
 * counted:
 * 	 - If the first byte of the filename is neither 0x00 or 0xE5
 *   - If the First Logical Cluster Byte (26) is not set to either 0x00 or 0x01
 *   - If the attributes byte (11) is not 0x0F, its bit 4 (subdirectory) is not
 *     set to 1, its bit 3 (volume label) is not set to 1.
 */
int count_files_in_disk(int disk_size, char* ptr) {
	int num_sectors = (disk_size / SECTOR_SIZE) - 19;
	int count = 0;
	char *init = ptr;
	char *subdir;

	int i;
	for (i = 0; i < num_sectors; i++) {
		if (ptr[0] == 0x00) {
			return count;
		}
		if ((ptr[0] != 0x00) && (ptr[26] != 0x00) && (ptr[26] != 0x01) && (ptr[11] != 0x0F) &&
			((ptr[11] & 0b00001000) != 0b00001000) &&
			((ptr[11] & 0b00010000) != 0b00010000)) {
			count++;
		}
		if ((ptr[11] & 0b00010000) == 0b00010000) { // subdirectory found
			subdir = ptr;
			count += count_files_in_disk(disk_size, init + (subdir[26] + 31 - 19) * SECTOR_SIZE);
		}
		ptr += 32;
	}
	return count;
}

void display_information(char* os_name, char* disk_label, int disk_size,
	int free_size, int num_root_files, int num_fat_copies, int sectors) {
	printf("OS Name: %s\n", os_name);
	printf("Label of the disk: %s\n", disk_label);
	printf("Total size of the disk: %d bytes\n", disk_size);
	printf("Free size of the disk: %d bytes\n\n", free_size);
	printf("==============\n");
	printf("The number of files in the disk (including all files in the root directory and files in all subdirectories): %d\n\n",
	 num_root_files);
	printf("=============\n");
	printf("Number of FAT copies: %d\n", num_fat_copies);
	printf("Sectors per FAT: %d\n", sectors);
}

int main(int argc, char* argv[]) {
	struct stat sb;
	char* mem_block_ptr;
	char os_name[100];
	char disk_label[100];
	int fd; // File descriptor
	int disk_size, free_disk_size, num_root_files, num_fat_copies, sectors_per_fat;

	if (argc < 2 || argc > 2) {
		printf("Usage: ./diskinfo <disk image>\n");
		exit(1);
	}

	// Attempting to open disk image provided
	if ((fd = open(argv[1], O_RDONLY)) == -1) {
		printf("Error: failed to read disk image\n");
		exit(1);
	}
	//Get information from the file, mostly just using it to retrieve the size
	//of the disk image
	fstat(fd, &sb);

	//Attempting to map memory
	mem_block_ptr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (mem_block_ptr == MAP_FAILED) {
		printf("Error: failed to map memory\n");
		exit(1);
	}
	close(fd);

	//Retrieving disk information. See documentation for the byte sequence of the
	//boot sector and directory entries.
	get_OS_name(os_name, mem_block_ptr);
	get_disk_label(disk_label, mem_block_ptr);
	disk_size = get_total_disk_size(mem_block_ptr);
	free_disk_size = get_free_disk_size(disk_size, mem_block_ptr);
	num_root_files = count_files_in_disk(disk_size, mem_block_ptr + (SECTOR_SIZE * 19));

	num_fat_copies = mem_block_ptr[16];
	sectors_per_fat = mem_block_ptr[22] + (mem_block_ptr[23] << 8);
	display_information(os_name, disk_label, disk_size, free_disk_size,
		num_root_files, num_fat_copies, sectors_per_fat);

	//Unmapping is required!
	munmap(mem_block_ptr, sb.st_size);
	return 0;
}
