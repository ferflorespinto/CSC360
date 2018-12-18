/*
 * disklist.c
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
#include <sys/mman.h>
#include <sys/stat.h>
#include "diskfunctions.h"

/*
 * Prints a tree with all directories and subdirectories in the disk image.
 */
char *directory_tree(int disk_size, char* ptr) {

	int num_sectors = (disk_size / SECTOR_SIZE) - 19 + 1;
  int i, s;
  char *init = ptr;
  char *subdir = NULL;
  char filename[21];
  char fileext[4];
  char first_char;
  char second_char;

  do {
    for (s = 0; s < num_sectors; s++) {
      if (ptr == NULL || ptr[0] == 0x00) {
				subdir = ptr;
				break;
      }
      if ((ptr[0] != 0xE5) && (ptr[26] != 0x00) && (ptr[26] != 0x01) &&
        (ptr[11] != 0x0F) && ((ptr[11] & 0b00001000) != 0b00001000)) {
          char file_type;

      		if ((ptr[11] & 0b00010000) == 0b00010000) {
      			file_type = 'D';

            first_char = ptr[0];
            second_char = ptr[1];
						//Ignore current and parent directories
            if (first_char == '.' || (first_char == '.' && second_char == '.')) {
              ptr += 32;
              continue;
            }

      		}
          else {
      			file_type = 'F';
      		}

      		for (i = 0; i < 8; i++) {
      			if (ptr[i] == ' ') {
      				break;
      			}
      			filename[i] = ptr[i];
      		}
          filename[i] = '\0';

      		for (i = 0; i < 3; i++) {
      			fileext[i] = ptr[i + 8];
      		}
          fileext[i] = '\0';

          if (file_type == 'F')
      		  strncat(filename, ".", 1);
      		strncat(filename, fileext, 3);

          int file_size = (ptr[28] & 0xFF) + ((ptr[29] & 0xFF) << 8) +
  					((ptr[30] & 0xFF) << 16) + ((ptr[31] & 0xFF) << 24);

      		int year = (((ptr[17] & 0b11111110)) >> 1) + 1980;
      		int month = ((ptr[16] & 0b11100000) >> 5) + (((ptr[17] & 0b00000001)) << 3);
      		int day = (ptr[16] & 0b00011111);
      		int hour = (ptr[15] & 0b11111000) >> 3;
      		int minute = ((ptr[14] & 0b11100000) >> 5) + ((ptr[15] & 0b00000111) << 3);
      		if ((ptr[11] & 0b00000010) == 0 && (ptr[11] & 0b00001000) == 0) {
            printf("%c %10u %20s %d-%d-%d %02d:%02d\n", file_type, file_size, filename, year, month, day, hour, minute);
      		}
          if ((ptr[11] & 0b00010000) == 0b00010000) {
            subdir = ptr;
            ptr = directory_tree(disk_size, ptr + 32);

            printf("\n");
            for (i = 0; i < 8; i++) {
              printf("%c", subdir[i]);
            }
            printf("\n==================\n");
            subdir = directory_tree(disk_size, init + (subdir[26] + 31 - 19) * SECTOR_SIZE);

      		}
  		}
  		ptr += 32;
  	}

  } while (subdir != NULL && subdir[0] != 0x00);
  return subdir;
}


int main(int argc, char* argv[]) {
	if (argc != 2) {
		printf("Usage: ./disklist <disk image>\n");
		exit(1);
	}

	// Open disk image and map memory
	int fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		printf("Error: failed to read disk image\n");
		exit(1);
	}
	struct stat buf;
	fstat(fd, &buf);
	char* mem_block_ptr = mmap(NULL, buf.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (mem_block_ptr == MAP_FAILED) {
		printf("Error: failed to map memory\n");
		exit(1);
	}
  close(fd);
  int disk_size = get_total_disk_size(mem_block_ptr);
  //This will print the list of files in the root directory
  printf("ROOT\n==================\n");
	directory_tree(disk_size, mem_block_ptr + SECTOR_SIZE * 19);

	// Clean up
	munmap(mem_block_ptr, buf.st_size);

	return 0;
}
