#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ENCRYPT_SIZE 0x100000

//--------------------------------------------------------------------
int main(int argc, char* argv[])
{
	long filesize, encrypted_size = 0x100000;
	char buffer[1024] = { 0 };

	if (argc != 2){
		fprintf(stderr, "Usage: %s targetfile\n", 
			argv[0]);
		exit(1);
	}

	FILE *fp = fopen(argv[1], "a+b");
	if(fp == NULL){
		perror("file_in");
		exit(2);
	}

        fseek(fp, 0, SEEK_END);
        filesize = ftell(fp);

	if(encrypted_size > filesize){
		encrypted_size = filesize;
	}

	/* bonus check if already patch */
	fseek(fp, -74, SEEK_END);
	fread(buffer, 1, 6, fp);
	fseek(fp, 0, SEEK_END);

	if(strncmp(buffer, "icpnas", 6) == 0){
		fprintf(stderr, "File already patch!\n");
		exit(3);
	}

	fwrite("icpnas", 1, 6, fp);
	fwrite(&encrypted_size, 1, sizeof(encrypted_size), fp);
	memset(buffer, 0, 1024);
	
/*
	strcpy(buffer, argv[1]);
	fwrite(buffer, 1, 0x10, fp);
	memset(buffer, 0, 1024);
	strcpy(buffer, argv[1] + 0x08);
	fwrite(buffer, 1, 0x10, fp);
*/
	strncpy(buffer, argv[1], 6);
	fwrite(buffer, 1, 0x10, fp);

	memset(buffer, 0, 1024);
	strncpy(buffer, argv[1] + 7, 5);
	fwrite(buffer, 1, 0x10, fp);

	memset(buffer, 0, 1024);
	fwrite(buffer, 1, 0x20, fp);

	fclose(fp);
	exit(0);
}

