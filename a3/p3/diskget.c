/*
 * diskget.c
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
 * Copies a file from a disk image to the current working directory.
 */
void copy_file(char* p_image, char* p_local, char* fptr, int file_size) {
  int first_logical_sector = fptr[26] + (fptr[27] << 8);
	int n;
	int bytes_to_write = file_size;
	int phys_address;

	do {
    if (bytes_to_write == file_size) {
      n = first_logical_sector;
    }
    else {
      n = get_fat_entry(n, p_image);

    }
		phys_address =  (31 + n) * SECTOR_SIZE;

		int i;
		for (i = 0; i < SECTOR_SIZE; i++) {
			if (bytes_to_write <= 0) {
				break;
			}
			p_local[file_size - bytes_to_write] = p_image[i + phys_address];
			bytes_to_write--;
		}
	} while (bytes_to_write > 0);
}

/*
 * Locates a file in the disk image.
 */
char *find_file(int disk_size, char* ptr, char *filename) {
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
        char file_type;

    		if ((ptr[11] & 0b00010000) == 0b00010000) {
    			file_type = 'D';
    		}
        else {
    			file_type = 'F';
    		}
        if (file_type == 'F') {
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

      		strncat(fname, ".", 1);
      		strncat(fname, fileext, strlen(fileext));
          //printf("fname: %s\n", fname);

          if (strncmp(fname, filename, 25) == 0) {
            //printf("Found file!\n");
            found_file = ptr;
            return found_file;
          }
          else {
            ptr += 32;
            continue;
          }
        }

        if ((ptr[11] & 0b00010000) == 0b00010000) {
          subdir = ptr;
          ptr += 32;
          //Search in the rest of the directory
          found_file = find_file(disk_size, ptr, filename);
          if (found_file != NULL) {
            return found_file;
          }

          //Search in subdirectories
          if (found_file == NULL) {
            found_file = find_file(disk_size, init + (subdir[26] + 31 - 19) * SECTOR_SIZE, filename);
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
  char *filefound;
  FILE *fp;

	if (argc != 3) {
		printf("Usage: ./diskget <disk image> <file to copy>\n");
		exit(1);
	}

	// Open disk image and map memory
	int fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		printf("Error: failed to read disk image\n");
		exit(1);
	}
	fstat(fd, &buf);
	char* p_image = mmap(NULL, buf.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (p_image == MAP_FAILED) {
		printf("Error: failed to map disk image memory\n");
		close(fd);
		exit(1);
	}

  int disk_size = get_total_disk_size(p_image);
  filefound = find_file(disk_size, p_image + SECTOR_SIZE * 19, argv[2]);

  if (filefound == NULL) {
    munmap(p_image, buf.st_size);
    printf("Error: file \"%s\" not found\n", argv[2]);
    exit(1);
  }

  int file_size = (filefound[28] & 0xFF) + ((filefound[29] & 0xFF) << 8) +
    ((filefound[30] & 0xFF) << 16) + ((filefound[31] & 0xFF) << 24);

  fp = fopen(argv[2], "wb+");
  if (fp == NULL) {
    munmap(p_image, buf.st_size);
    close(fd);
    fclose(fp);
    printf("Error: failed to open file\n");
    exit(1);
  }
  int fd2 = fileno(fp);

  if (fseek(fp, file_size - 1, SEEK_SET) == -1) {
    munmap(p_image, buf.st_size);
    close(fd);
    fclose(fp);
    close(fd2);
    printf("Error: failed to seek the end of file\n");
    exit(1);

  }
  if (write(fd2, "", 1) != 1) {
    munmap(p_image, buf.st_size);
    close(fd);
    fclose(fp);
    close(fd2);
    printf("Error: failed to write null character at end of file\n");
    exit(1);
  }
  // Map memory for file to be written
  char* p_local = mmap(NULL, file_size, PROT_WRITE, MAP_SHARED, fd2, 0);
  if (p_local == MAP_FAILED) {
    printf("Error: failed to map file memory\n");
    exit(1);
  }
  copy_file(p_image, p_local, filefound, file_size);

  munmap(p_local, file_size);
  fclose(fp);
  close(fd2);
	munmap(p_image, buf.st_size);
	close(fd);

	return 0;
}
