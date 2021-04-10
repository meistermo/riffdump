#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>
#include <math.h>

static long find_chunk(FILE*, char*);
static int print_file_meta(FILE*, char*);
static int print_chunk_meta(FILE*, long);
static int count_chunks(FILE*);
static int read_all_chunks(unsigned long[], FILE*);

typedef struct Options {
	unsigned char a;
	unsigned char list_some;
	unsigned char list_all;
	unsigned char count_only;
	char *chunks;
} Option;

static int parse_opt (int key, char *arg, struct argp_state *state) {
	static Option opt = { 0, 0, 0, 0, "" };
	
	switch (key) {
		case 'c':
			opt.count_only = 1;
			break;

		case 'l':
			if(arg == NULL) { opt.list_all = 1; break; } //without argument to --list
			else {opt.list_some = 1; opt.chunks = arg; } //with argument to --list
			break;

		case ARGP_KEY_ARG:
			/*empty statement*/; 
			FILE *file = fopen(arg, "rb");
			if(file == NULL) { argp_failure(state, 1, 0, "No such file or directory"); }
			char filetype_buffer[4];
			if(fread(filetype_buffer, 1, 4, file) != 4 || strncmp(filetype_buffer, "RIFF", 4) != 0) { argp_failure(state, 1, 0, "Invalid file format"); }

			//flag specific behaviour:::

			if(opt.list_all) {
				int size = count_chunks(file);
				unsigned long addresses[size];
				read_all_chunks(addresses, file);
				for(int i = 0; i < size; i++) {
					print_chunk_meta(file, addresses[i]);
				}
			}
			
			if(opt.list_some) {
				if((int)(strlen(opt.chunks) - floor(strlen(opt.chunks) / 5)) % 4 != 0) { argp_failure(state, 1, 0, "Invalid searchstring"); }
				if( ((int)(strlen(opt.chunks) - floor(strlen(opt.chunks) / 5)) / 4) != (int)(floor(strlen(opt.chunks) / 5) + 1)) { argp_failure(state, 1, 0, "Invalid searchstring"); }
				for(int i = 0; i < (int)floor(strlen(opt.chunks) / 5) + 1; i++) {
					char fourcc[] = {
						opt.chunks[0 + (i * 4) + i],
						opt.chunks[1 + (i * 4) + i],
						opt.chunks[2 + (i * 4) + i],
						opt.chunks[3 + (i * 4) + i]
					};
					long chunk_adress = find_chunk(file, fourcc);
					if(chunk_adress < 0) { argp_failure(state, 1, 0, "Chunk not found"); }
					if(print_chunk_meta(file, chunk_adress) != 0) { argp_failure(state, 1, 0, "Unable to read file"); }
				}
			}

			if(opt.count_only) {
				printf("%d\n", count_chunks(file));
			}

			//if no options given
			if(!opt.list_some && !opt.a && !opt.list_all && !opt.count_only) {
				if(print_file_meta(file, arg) != 0) { argp_failure(state, 1, 0, "Unable to read file"); }
			}
			
			fclose(file);
			break;

		case ARGP_KEY_NO_ARGS:
			argp_failure(state, 1, 0, "Too few arguments");
			break;
	}
	return 0; 
} 

//finds a chunk specified by a given identifier and returns its first byte adress to ptr
//can return a negative value if EOF is ever read or something went wrong
static long find_chunk(FILE *file, char *identifier) {
	
	if(fseek(file, 12, SEEK_SET) != 0) { return -1; }

	unsigned char buffer[4];
	while(1) {
		if(fread(buffer, 1, 4, file) < 4 ) { return -2; }; //read error
		if(strncmp(buffer, identifier, 4) == 0) { return ftell(file) - 4; } //success
		if(fread(buffer, 1, 4, file) < 4) { return -2; } //read error
		if(fseek(file, buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24), SEEK_CUR) != 0) { return -3; } //read error
	}
}

//reads file metadata (size [4 bytes] and format type [4 bytes]) and prints it as formatted output
//returns a negative number if something went wrong while trying to read from the file, else 0 
static int print_file_meta(FILE *file, char *filename) {
	
	unsigned char buffer[8];
	
	if(fseek(file, 4, SEEK_SET) != 0) { return -1; }
	if(fread(buffer, 1, 8, file) < 8) { return -1; }
	
	printf("%s |» Format: RIFF/%c%c%c%c, Size: %d bytes\n", filename, buffer[4], buffer[5], buffer[6], buffer[7] == 32 ? 0 : buffer[7], 8 + (buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24)));
	
	return 0;
}

//reads chunk metadata (fourcc [4 bytes] and size [4 bytes]) and prints it as formatted output
//returns a negative number if something went wrong while trying to read from the file, else 0
static int print_chunk_meta(FILE *file, long address) {
	
	unsigned char buffer[8];
	
	if(fseek(file, address, SEEK_SET) != 0) { return -1; }	
	if(fread(buffer, 1, 8, file) < 8) {return -1; }
	
	printf("'%c%c%c%c' at 0x%X |» Size: %d bytes\n", buffer[0], buffer[1], buffer[2], buffer[3], (int)address, 8 + (buffer[4] + (buffer[5] << 8) + (buffer[6] << 16) + (buffer[7] << 24)));
	
	return 0;
}

//todo: validate file integrity based on file size vs chunk sizes
//counts all subchunks of the root RIFF chunk by crawling the size metadata (always bytes 5-8 of a new chunk)
//returns the number of chunks found or a negative number if something went wrong while trying to read from the file
static int count_chunks(FILE *file) {
	
	int n = 0;
	unsigned char chunk_size[4];

	if(fseek(file, 16, SEEK_SET) != 0) { return -1; }
	while(fread(chunk_size, 1, 4, file) == 4) {
		if(fseek(file, 4 + (chunk_size[0] + (chunk_size[1] << 8) + (chunk_size[2] << 16) + (chunk_size[3] << 24)), SEEK_CUR) != 0) { return -1; }
		n++;
	}
	
	return n;
}

//writes the byte adresses of all chunks in this file to addresses[]
//returns a negative number if something went wrong while trying to read from the file, else 0
static int read_all_chunks(unsigned long addresses[], FILE *file) {
	
	int n = 0;
	unsigned char chunk_size[4];
	long adr = 12;

	if(fseek(file, 16, SEEK_SET) != 0) {return -1; }

	while(fread(chunk_size, 1, 4, file) == 4) {
		addresses[n] = adr;
		n++;

		if(fseek(file, 4 + (chunk_size[0] + (chunk_size[1] << 8) + (chunk_size[2] << 16) + (chunk_size[3] << 24)), SEEK_CUR) != 0) { return -1; }
		adr += 8 + (chunk_size[0] + (chunk_size[1] << 8) + (chunk_size[2] << 16) + (chunk_size[3] << 24));
	}

	return 0;
}

int main(int argc, char *argv[]) {
	struct argp_option options[] = {
		{ "list", 'l', "<chunks>", OPTION_ARG_OPTIONAL, "List (all) chunks" },
		{ "count", 'c', 0, 0, "Count all chunks" },
		{ 0 }
	}; 
	struct argp argp = { options, parse_opt }; 
	return argp_parse(&argp, argc, argv, 0, 0, 0);
}