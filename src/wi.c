#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[]) {
    FILE *file = fopen(argv[1], "rb");

    if(file == NULL) {
        perror("wi");
        exit(EXIT_FAILURE);
    }

	char filetype_buffer[4];
	if(fread(filetype_buffer, 1, 4, file) != 4 || strncmp(filetype_buffer, "RIFF", 4) != 0) {
		//printf("wi: Invalid file format\n");
		exit(EXIT_FAILURE);
	}

	if(fseek(file, 12, SEEK_SET) != 0) {
		//something went wrong
		//abort
	}

	while(1) {
		char label[4]; 
		if(fread(label, 1, 4, file) == 4) {
			if(strncmp(label, "fmt ", 4) == 0) {
				printf("We've hit gold!\n");
				break;
			}
		}
		
		unsigned char size[4];
		fread(size, 1, 4, file);
		if(fseek(file, size[0] + (size[1] << 8) + (size[2] << 16) + (size[3] << 24), SEEK_CUR) != 0) {
			//something went wrong
			//abort
		}
	}

/*
    unsigned char header[70];
	fseek(file, 0, SEEK_SET);
	for(int i = 0; i < 70; i++) {
		header[i] = fgetc(file);
		if(header[i] == EOF) { exit(EXIT_FAILURE); }
    }

	printf("ChunkID: %c%c%c%c\n", header[0], header[1], header[2], header[3]);
	printf("ChunkSIZE: %d\n", header[4] + (header[5] << 8) + (header[6] << 16) + (header[7] << 24));
	printf("Format: %c%c%c%c\n", header[8], header[9], header[10], header[11]);
	printf("fmt chunkID: %c%c%c%c\n", header[12], header[13], header[14], header[15]);
	printf("fmt chunkSize: %d\n", header[16] + (header[17] << 8) + (header[18] << 16) + (header[19] << 24));
	printf("AudioFormat: %d\n", header[20] + (header[21] << 8));
	printf("NumChannels: %d\n", header[22] + (header[23] << 8));
	printf("SampleRate: %d\n", header[24] + (header[25] << 8) + (header[26] << 16) + (header[27] << 24));
	printf("ByteRate: %d\n", header[28] + (header[29] << 8) + (header[30] << 16) + (header[31] << 24));
	printf("BlockAlign: %d\n", header[32] + (header[33] << 8));
	printf("BitsPerSample: %d\n", header[34] + (header[35] << 8));
	printf("Data ChunkMarker: %c%c%c%c\n", header[36], header[37], header[38], header[39]);
	printf("Data ChunkSIZE: %d\n", header[40] + (header[41] << 8) + (header[42] << 16) + (header[43] << 24));
	printf("Data ChunkID: %c%c%c%c\n", header[44], header[45], header[46], header[47]);
	printf("Info1: %c%c%c%c\n", header[48], header[49], header[50], header[51]);
	printf("Info1 Size: %d\n", header[52] + (header[53] << 8) + (header[54] << 16) + (header[55] << 24));
	printf("Info1 Content: %c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", header[56], header[57], header[58], header[59], header[60], header[61], header[62], header[63], header[64], header[65], header[66], header[67], header[68], header[69]);
	
	//printf("Info2: %c%c%c%c\n", header[60], header[61], header[62], header[63]);
	//printf("Info2 Size: %c%c%c%c\n", header[64], header[65], header[66], header[67]);
	//printf("Info2 Content: %c%c\n", header[68], header[69]);

	//bytes[0] + (bytes[1] << 8) + (bytes[2] << 16) + (bytes[3] << 24);
	//uint32_t myInt1 = (bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3];
*/


    return 0;
}