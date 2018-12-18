/*
 * diskput.c
 *
 * University of Victoria
 * CSC360 – Fall 2018
 * Assignment 3
 * Name: Jorge Flores Pinto
 * ID: V00880059
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "diskfunctions.h"

/*
 * Finds a free space to record the file's information, such as name, extension,
 * size, and date. We also record the location of the first logical sector,
 * which will point to the data cluster where the file's data begins.
 *
 */
void update_directory(char* file_name, int file_size, int first_logical_sector,
	char* p) {

	while (p[0] != 0x00) {
		p += 32;
	}

	// Set file_name and extension
	int i;
	int done = -1;
	for (i = 0; i < 9; i++) {
		char character = file_name[i];
		if (character == '.' || i == 8) { //Max num of chars for basic filenames is
			done = i;												//8 bytes
			break;
		}
		if (done == -1)
			p[i] = character;
		else
			p[i] = ' ';
	}
	for (i = 0; i < 3; i++) {
		p[i + 8] = file_name[i + done + 1];
	}

	// Set attributes
	p[11] = 0x00;

	// Set create date/time
	time_t t = time(NULL);
	struct tm *now = localtime(&t);
	int year = now->tm_year + 1900;
	int month = (now->tm_mon + 1);
	int day = now->tm_mday;
	int hour = now->tm_hour;
	int minute = now->tm_min;

	p[14] = 0x00;
	p[15] = 0x00;
	p[16] = 0x00;
	p[17] = 0x00;
	p[17] |= (year - 1980) << 1;
	p[17] |= (month - ((p[16] & 0xE0) >> 5)) >> 3;
	p[16] |= (month - (((p[17] & 0x01)) << 3)) << 5;
	p[16] |= (day & 0x1F);
	p[15] |= (hour << 3) & 0xF8;
	p[15] |= (minute - ((p[14] & 0xE0) >> 5)) >> 3;
	p[14] |= (minute - ((p[15] & 0x07) << 3)) << 5;

	// Set first logical cluster
	p[26] = (first_logical_sector - (p[27] << 8)) & 0xFF;
	p[27] = (first_logical_sector - p[26]) >> 8;

	// Set file size
	p[28] = (file_size & 0xFF);
	p[29] = (file_size & 0x0000FF00) >> 8;
	p[30] = (file_size & 0x00FF0000) >> 16;
	p[31] = (file_size & 0xFF000000) >> 24;
}

/*
 * Retrieves the next available space in data memory.
 */
int get_free_fat_index(char* p) {
	int n = 2; //First data cluster is **always** 2
	char *curr = p + (n + 31 - 1) * SECTOR_SIZE;
	while (get_fat_entry(n, p) != 0x00 || curr[0] != 0x00) {
		n++;
		curr = p + (n + 31) * SECTOR_SIZE;
	}
	return n;
}

/*
 * Sets the FAT entry the current entry being written should point to.
 * Please refer to the specification for more information on how to do FAT
 * packing and what it means.
 */
void set_fat_entry(int n, int value, char* p) {
	unsigned int first_byte, second_byte;

	if ((n % 2) == 0) {
		first_byte = (unsigned int) (value >> 8) & 0x0F;
		p[SECTOR_SIZE + ((3 * n) / 2) + 1] = first_byte;
		second_byte = (unsigned int) value & 0xFF;
		p[SECTOR_SIZE + ((3 * n) / 2)] = second_byte;

	} else {
		first_byte = (unsigned int) (value << 4) & 0xF0;
		p[SECTOR_SIZE + ((3 * n) / 2)] = first_byte;
		second_byte = (unsigned int) (value >> 4) & 0xFF;
		p[SECTOR_SIZE + ((3 * n) / 2) + 1] = second_byte;
	}
}

/*
 * A function that visually displays the bytes being retrieved when accessing
 * the FAT table for data writing. This function is exactly the same as the
 * function get_fat_entry in diskfunctions.c. It is meant for testing only.
 */
int get_fat_entry_print(int n, char* ptr) {
	unsigned int result;
	unsigned int first_byte;
	unsigned int second_byte;
	printf("------ Getting FAT Entry ------\n");
	printf("Index n provided: %d\n", n);

	if ((n % 2) == 0) {
		printf("Even!\n");
		first_byte = ptr[SECTOR_SIZE + ((3 * n) / 2) + 1] & 0x0F;
		second_byte = ptr[SECTOR_SIZE + ((3 * n) / 2)] & 0xFF;
		result = (first_byte << 8) + second_byte;

	} else {
		printf("Odd!\n");
		first_byte = ptr[SECTOR_SIZE + ((3 * n) / 2)] & 0xF0;
		second_byte = ptr[SECTOR_SIZE + ((3 * n) / 2) + 1] & 0xFF;
		result = (first_byte >> 4) + (second_byte << 4);
	}
	printf("ptr[%d] = 0x%3x.\n", SECTOR_SIZE + ((3 * n) / 2), ptr[SECTOR_SIZE + ((3 * n) / 2)]);
	printf("ptr[%d] = 0x%3x.\n", SECTOR_SIZE + ((3 * n) / 2) + 1, ptr[SECTOR_SIZE + ((3 * n) / 2) + 1]);
	printf("first_byte: 0x%2x (%u).\n", first_byte, first_byte);
	printf("second_byte: 0x%2x (%u).\n", second_byte, second_byte);
	printf("result: 0x%3x (%u).\n\n", result, result);
	return result;
}

/*
 * Writes the file in the current directory into the disk image.
 */
void copy_file(char* p_image, char* p_local, char* fptr, char* filename,
	 int file_size) {

	int current = get_free_fat_index(p_image);

	int bytes_to_write = file_size;
	int phys_address; // = SECTOR_SIZE * (31 + n);

	if (p_image != fptr) {
		update_directory(filename, file_size, current, p_image + (fptr[26] + 31) * SECTOR_SIZE);
	}
	else {
		update_directory(filename, file_size, current, p_image + 19 * SECTOR_SIZE);
	}

	do {
		phys_address =  (31 + current) * SECTOR_SIZE;

		int i;
		for (i = 0; i < SECTOR_SIZE; i++) {
			if (bytes_to_write <= 0) {
				set_fat_entry(current, 0xFFF, p_image);
				return;
			}
			p_image[i + phys_address] = p_local[file_size - bytes_to_write];
			bytes_to_write--;
		}

		set_fat_entry(current, 0x77, p_image);
		int next = get_free_fat_index(p_image);
		set_fat_entry(current, next, p_image);
		get_fat_entry(current, p_image);
		current = next;
	} while (bytes_to_write > 0);
}

/*
 * Finds the file directory where the file is to be written. Returns NULL if no
 * directory is specified (that means the file is to be placed in the root
 * directory), or if the directory does not exist. Otherwise, returns a pointer
 * to the directory specified.
 */
char *find_dir(int disk_size, char* ptr, char *dir) {
	int num_sectors = (disk_size / SECTOR_SIZE) - 19 + 1;
  int i, s;
  char *init = ptr;
  char *subdir = NULL;
  char fname[21];
  char fileext[4];
  char *found_file = NULL;

  for (s = 0; s < num_sectors; s++) {
    if (ptr[0] == 0x00) {
      subdir = NULL;
      break;
    }
    if ((ptr[0] != 0xE5) && (ptr[26] != 0x00) && (ptr[26] != 0x01) &&
      (ptr[11] != 0x0F) && ((ptr[11] & 0b00001000) != 0b00001000)) {
        for (i = 0; i < 8; i++) {
    			if (ptr[i] == ' ') {
    				break;
    			}
    			fname[i] = ptr[i];
    		}
        fname[i] = '\0';

    		for (i = 0; i < 3; i++) {
          if (ptr[i + 8] == ' ') {
            break;
          }
    			fileext[i] = ptr[i + 8];
    		}
        fileext[i] = '\0';

        if (strncmp(fname, dir, 25) == 0) {
          //printf("Found file!\n");
          found_file = ptr;
          return found_file;
        }

        if ((ptr[11] & 0b00010000) == 0b00010000) {
          subdir = ptr;
          ptr += 32;
          //Search in the rest of the directory
          found_file = find_dir(disk_size, ptr, dir);
          if (found_file != NULL) {
            return found_file;
          }

          //Search in subdirectories
          if (found_file == NULL) {
            found_file = find_dir(disk_size, init + (subdir[26] + 31 - 19) * SECTOR_SIZE, dir);
            return found_file;
          }

    		}
		}
		ptr += 32;
	}
  return NULL;
}

int main(int argc, char* argv[]) {
  struct stat buf;
  struct stat buf2;
  const char delim[2] = "/";
  char *file_path = argv[2];
  char *last_subdir = NULL;
  char *file_name;
  char *token;
  char *tokens[200];
  int i;
  int path_has_subdirs = 0;
  int count = 1;

	if (argc != 3) {
		printf("Usage: ./diskput <disk image> <file>\n");
		exit(1);
	}

  for(i = 0; i < strlen(file_path); i++) {
    if (file_path[i] == '/') {
      path_has_subdirs = 1;
    }
  }
  if (path_has_subdirs == 1) {
    token = strtok(file_path, delim);
    while( token != NULL ) {
      tokens[count] = token;
      token = strtok(NULL, delim);
      count++;
    }
    tokens[i] = NULL;

    last_subdir = strdup(tokens[count - 2]);
    file_name = strdup(tokens[count - 1]);
  }
  else {
    file_name = strndup(file_path, strlen(file_path));
  }

	// Open disk image and map memory
	int fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		printf("Error: failed to read disk image\n");
		exit(1);
	}
	fstat(fd, &buf);
	char* p_image = mmap(NULL, buf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (p_image == MAP_FAILED) {
		printf("Error: failed to map disk image memory\n");
		close(fd);
		exit(1);
	}

	// Open file and map memory
	int fd2 = open(file_name, O_RDWR);
	if (fd2 < 0) {
		printf("Error: File not found.\n");
		munmap(p_image, buf.st_size);
		close(fd);
		exit(1);
	}
	fstat(fd2, &buf2);
	int file_size = buf2.st_size;
	char* p_local = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd2, 0);
	if (p_local == MAP_FAILED) {
		printf("Error: failed to map file memory\n");
		munmap(p_image, buf.st_size);
		close(fd);
		close(fd2);
		exit(1);
	}

	// Copy file from local directory to disk image if there's space
	int total_disk_space = get_total_disk_size(p_image);
	int free_disk_space = get_free_disk_size(total_disk_space, p_image);

	if (file_size >= free_disk_space) {
		printf("Error: not enough space in disk to write the file '%s'\n",
			file_name);
		printf("Space available: %d\n", free_disk_space);
		printf("Space required: %d\n", file_size);
		munmap(p_image, buf.st_size);
		close(fd);
		close(fd2);
		exit(1);
	}

  char *subdir_ptr;
  if (path_has_subdirs == 1) {
    subdir_ptr = find_dir(total_disk_space, p_image + SECTOR_SIZE * 19, last_subdir);
		if (subdir_ptr == NULL) {
			printf("Error: could not find specified directory\n");
			munmap(p_image, buf.st_size);
			close(fd);
			close(fd2);
			exit(1);
		}
	}
	else {
		subdir_ptr = p_image;
	}

	copy_file(p_image, p_local, subdir_ptr, file_name, file_size);

	// Clean up
	munmap(p_image, buf.st_size);
	munmap(p_local, file_size);
	close(fd);
	close(fd2);

	return 0;
}
