#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ENCRYPT_SIZE 0x100000

int keybits;
int y, z;
 
//--------------------------------------------------------------------
int code(int idx, unsigned long *rand_seed)
{
        unsigned long res1, res2;
 
        if(idx + z == 0)
                res2 = 0;
        else
                res2 = 0x4e35 * (idx + z);
        res1 = *rand_seed * 0x15a;
        *rand_seed = 0x4e35 * *rand_seed + 1;
       
        z = y + res2 + res1;
        y = res1;
        return *rand_seed ^ z;
}
 
//--------------------------------------------------------------------
int assemble(char *password)
{
        int i;
        unsigned long table[1024];
        unsigned long prev_val, crypt_val;
 
        for(i = 0; i < keybits / 16; i++)
        {
                if(i == 0)
                        prev_val = 0;
                else
                        prev_val = table[i - 1];
                table[i] = prev_val ^ ((password[2 * i] << 8) + password[2 * i + 1]);
                if(i == 0)
                        crypt_val = 0;
                crypt_val = crypt_val ^ code(i, &table[i]);
 
        }
        return crypt_val;
}

//--------------------------------------------------------------------
unsigned char encrypt(unsigned char v, char *password)
{
        int crypt_val;
        unsigned char xor_value;
        int i, tmp;

        crypt_val = assemble(password);
	xor_value = (crypt_val >> 8);
	tmp = crypt_val & 0xff;
		
        for(i = 0; i < keybits / 8; i++)
                password[i] ^= v;
 
        return v ^ (xor_value ^ tmp);
} 

 
 //--------------------------------------------------------------------
int file_encrypt(FILE *f_in, FILE *f_out, long size, char *password)
{
	long filesize, decrypted_len, block_size;
	char buffer[1024] = { 0 };
	unsigned char buffer2[1024] = { 0 };
	int i;

	fseek(f_in, 0, SEEK_END);
	filesize = ftell(f_in);
		
	if(filesize <= size){
		size = filesize;
		decrypted_len = filesize;
	}else{
		decrypted_len = size;
	}
 
	rewind(f_in);

	while(decrypted_len > 0)
	{
		block_size = 1024;
		if(decrypted_len < 1024)
			block_size = decrypted_len;
		fread(buffer, 1, block_size, f_in);

		for(i = 0; i < block_size; i++)
		{
			buffer2[i] = encrypt(buffer[i], password);
		}
		fwrite(buffer2, 1, block_size, f_out);
		decrypted_len -= block_size;
		filesize -= block_size;
	}
 
	while(filesize > 0)
	{
		block_size = 1024;
		if(filesize < 1024)
			block_size = filesize;

		fread(buffer, 1, block_size, f_in);
		fwrite(buffer, 1, block_size, f_out);
		filesize -= block_size;
	}

	return 0;
}

//--------------------------------------------------------------------
unsigned char decrypt(unsigned char v, char *password)
{
        int crypt_val;
        unsigned char xor_value;
        int i;
       
        crypt_val = assemble(password);
        xor_value = v ^ (crypt_val >> 8) ^ (crypt_val & 0xff);
        for(i = 0; i < keybits / 8; i++)
			password[i] ^= xor_value;
 
        return xor_value;
}
 
//--------------------------------------------------------------------
int file_decrypt(FILE *f_in, FILE *f_out, char *password)
{
        long filesize, encrypted_len, block_size;
        char buffer[1024] = { 0 };
        unsigned char buffer2[1024] = { 0 };
        int i;
 
        fseek(f_in, 0, SEEK_END);
        filesize = ftell(f_in);
        fseek(f_in, -74, SEEK_END);
        fread(buffer, 1, 6, f_in);
 
        if(strncmp(buffer, "icpnas", 6))
        {
			printf("sig: %s %s\n", buffer, "icpnas");
			return -1;
        }
        else
        {
			fread(&encrypted_len, 4, 1, f_in);
			printf("len=%ld\n", encrypted_len);
			fread(buffer, 1, 16, f_in);
			printf("model name = %s\n", buffer);
			fread(buffer, 1, 16, f_in);
			printf("version = %s\n", buffer);
			rewind(f_in);

			filesize -= encrypted_len;
			filesize -= 74;
        }
 
        while(encrypted_len > 0)
        {
			block_size = 1024;
			if(encrypted_len < 1024)
				block_size = encrypted_len;
			fread(buffer, 1, block_size, f_in);

			for(i = 0; i < block_size; i++)
			{
				buffer2[i] = decrypt(buffer[i], password);
			}
			
			fwrite(buffer2, 1, block_size, f_out);
			encrypted_len -= block_size;
        }
 
        while(filesize > 0)
        {
			block_size = 1024;
			if(filesize < 1024)
				block_size = filesize;
			fread(buffer, 1, block_size, f_in);
			fwrite(buffer, 1, block_size, f_out);
			filesize -= block_size;
        }
	
        return 0;
}

//--------------------------------------------------------------------
int main(int argc, char* argv[])
{
	char *key;

	if (argc != 5)
	{
		fprintf(stderr, "Usage: %s e|d \"key\" sourcefile targetfile\n", 
			argv[0]);
		fprintf(stderr, "%s d QNAPNASVERSION4 " \
			"TS-219_3.5.0_Build0816.img " \
			"TS-219_3.5.0_Build0816.img.tgz\n", argv[0]);
		fprintf(stderr, "%s e QNAPNASVERSION4 " \
			"TS-219_3.5.0_Build0816.img.tgz " \
			"TS-219_3.5.0_Build0816.img\n", argv[0]);

		exit(1);
	}

	FILE *f = fopen(argv[3], "rb");
	if(f == NULL){
		perror("file_in");
		exit(2);
	}

	FILE *f2 = fopen(argv[4], "wb");
	if(f2 == NULL){
		fclose(f);
		perror("file_out");
		exit(3);
	}

	key = argv[2];
	keybits = strlen(key) * 8;
	printf("Using %d-bit encryption - (%s)\n", keybits, key);

	if(argv[1][0] == 'e')
		file_encrypt(f, f2, ENCRYPT_SIZE, key);
	else if(argv[1][0] == 'd')
		file_decrypt(f, f2, key);
	else
		fprintf(stderr, "Unknown action\n");

	fclose(f);
	fclose(f2);

	exit(0);
}

