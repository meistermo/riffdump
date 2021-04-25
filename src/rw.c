#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>
#include <math.h>
#include <stdbool.h>

static long find_chunk(FILE*, char*);
static int print_file_meta(FILE*, char*, unsigned char);
static int print_chunk_meta(FILE*, long, unsigned char);
static int print_chunk_data(FILE*, long, unsigned char);
static int print_chunk_count(FILE*, long, unsigned char);
static int count_subchunks(FILE*, long);
static int read_subchunks(unsigned long[], FILE*, long);
static bool validate_file_format(FILE*);

const char *argp_program_version = "riffdump 0.1";
const char *argp_program_bug_address = "<mo@momme.wtf>";

static struct argp_option options[] = {
	{ "list", 'l', 0, 0, "List subchunks" },
	{ "sub", 's', "<chunk>", 0, "Specify the chunk to operate on (Default: RIFF)" },
	{ "data", 'd', "<chunk>", 0, "Dump chunk data"},
	{ "count", 'c', 0, 0, "Count subchunks" },
	{ "verbose", 'v', 0, 0, "Produce verbose output" },
	{ 0 }
}; 

typedef struct Options {
	bool list;
	char *sub;
	bool count;
	char *data;
	bool verbose;
} Options;

static int parse_opt (int key, char *arg, struct argp_state *state) { 
	Options *options = state->input;
	
	switch (key) {
		case 'd':
			options->data = arg;
			break;
		case 'c':
			options->count = true;
			break;

		case 's':
			options->sub = arg;
			break;

		case 'l':
			options->list = true;
			break;

		case 'v':
			options->verbose = true;
			break;

		case ARGP_KEY_ARG:
			/*empty statement*/;
			FILE *file = fopen(arg, "rb");
			if(file == NULL) { argp_failure(state, 1, 0, "No such file or directory"); }
			if(!validate_file_format(file)) { argp_failure(state, 1, 0, "Unrecognized file format"); }
			
			//flag specific behaviour:::

			if(options->list) {
				if(strlen(options->sub)) { //list subchunks
					int size = count_subchunks(file, find_chunk(file, options->sub));
					unsigned long addresses[size];
					read_subchunks(addresses, file, find_chunk(file, options->sub));
					for(int i = 0; i < size; i++) {
						print_chunk_meta(file, addresses[i], options->verbose);
					}					
				} else { //list chunks
					int size = count_subchunks(file, 0);
					unsigned long addresses[size];
					read_subchunks(addresses, file, 0);
					for(int i = 0; i < size; i++) {
						print_chunk_meta(file, addresses[i], options->verbose);
					}
				}
			}
			
			if(options->count) {
				if(strlen(options->sub)) { //count subchunks
					print_chunk_count(file, find_chunk(file, options->sub), options->verbose);
				} else { //count chunks
					print_chunk_count(file, 0, options->verbose);
				}
			}

			if(strlen(options->data)) {
				print_chunk_data(file, find_chunk(file, options->data), options->verbose);
			}

			//if no options given
			if(!strlen(options->data) && !strlen(options->sub) && !options->list && !options->count) {
				if(print_file_meta(file, arg, options->verbose) != 0) { argp_failure(state, 1, 0, "Unable to read file"); }
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
static int print_file_meta(FILE *file, char *filename, unsigned char verbose) {
	
	unsigned char buffer[8];
	
	if(fseek(file, 4, SEEK_SET) != 0) { return -1; }
	if(fread(buffer, 1, 8, file) < 8) { return -1; }
	
	if(verbose) {
		printf("%s |» Format: RIFF/%c%c%c%c, Size: %d bytes\n", filename, buffer[4], buffer[5], buffer[6], buffer[7] == 32 ? 0 : buffer[7], 8 + (buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24)));
	} else {
		printf("%c%c%c%c\n", buffer[4], buffer[5], buffer[6], buffer[7]);
	}
	
	return 0;
}

//reads chunk metadata (fourcc [4 bytes] and size [4 bytes]) and prints it as formatted output
//returns a negative number if something went wrong while trying to read from the file, else 0
static int print_chunk_meta(FILE *file, long address, unsigned char verbose) {
	
	unsigned char buffer[8];
	
	if(fseek(file, address, SEEK_SET) != 0) { return -1; }	
	if(fread(buffer, 1, 8, file) < 8) {return -1; }
	
	if(verbose) {
		printf("'%c%c%c%c' at 0x%X |» Size: %d bytes\n", buffer[0], buffer[1], buffer[2], buffer[3], (int)address, 8 + (buffer[4] + (buffer[5] << 8) + (buffer[6] << 16) + (buffer[7] << 24)));
	} else {
		printf("%c%c%c%c\n", buffer[0], buffer[1], buffer[2], buffer[3]);
	}

	return 0;
}

static int print_chunk_data(FILE *file, long address, unsigned char verbose) {
	unsigned char size[8];
	
	if(fseek(file, address + 4, SEEK_SET) != 0) { return -1; }	
	if(fread(size, 1, 4, file) < 4) {return -1; }
	
	unsigned long data_size = (size[0] + (size[1] << 8) + (size[2] << 16) + (size[3] << 24));
	unsigned char buffer[data_size];

	if(fread(buffer, 1, data_size, file) < data_size) { return -1; }
	
	if(verbose) {
		for(int i = 0; i < data_size; i++) {
			printf("%c ", buffer[i]);
		}
	} else {
		for(int i = 0; i < data_size; i++) {
			printf("%X ", (int)buffer[i]);
		}
	}
		
	printf("\n");
	return 0;
}

static int print_chunk_count(FILE *file, long offset, unsigned char verbose) {
	int n = count_subchunks(file, offset);
	if(verbose) {
		printf("%d %schunk%s found\n", n, offset > 0 ? "sub" : "", n > 1 ? "s" : "");
	} else {
		printf("%d\n", n);
	}
	return 0;
}

//counts all subchunks of the root RIFF chunk by crawling the size metadata (always bytes 5-8 of a new chunk)
//returns the number of chunks found or a negative number if something went wrong while trying to read from the file
static int count_subchunks(FILE *file, long offset) {
	
	int n = 0;
	unsigned char buffer[4];
	
	if(fseek(file, offset + 4, SEEK_SET) != 0) { return -1; }
	if(fread(buffer, 1, 4, file) != 4) { return -1; }
	unsigned long p_size = (buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24));

	if(fseek(file, 8, SEEK_CUR) != 0) { return -1; }
	unsigned long c = 0;
	while(c < p_size - 4) {
		if(fread(buffer, 1, 4, file) != 4) { return -1; };
		if(fseek(file, 4 + (buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24)), SEEK_CUR) != 0) { return -1; }
		c += 8 + (buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24));
		n++;
	}
	
	return n;
}

//writes the byte adresses of all subchunks of the chunk at <offset> to addresses[]
//returns a negative number if something went wrong while trying to read from the file, else 0
static int read_subchunks(unsigned long addresses[], FILE *file, long offset) {
	
	int n = 0;
	unsigned char buffer[4];
	long adr = 12 + offset;

	//if(fseek(file, 16 + offset, SEEK_SET) != 0) {return -1; }
	if(fseek(file, offset + 4, SEEK_SET) != 0) {return -1; }
	if(fread(buffer, 1, 4, file) != 4) { return -1; }
	unsigned long p_size = (buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24));

	if(fseek(file, 8, SEEK_CUR) != 0) { return -1; }
	unsigned long c = 0;
	while(c < p_size - 4) {
		if(fread(buffer, 1, 4, file) != 4) { return -1; }
		addresses[n] = adr;
		n++;

		if(fseek(file, 4 + (buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24)), SEEK_CUR) != 0) { return -1; }
		adr += 8 + (buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24));
		c += 8 + (buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24));
	}
	return 0;
}

//naive validation based on the leading RIFF file that should be present
//returns 1 if the file is valid, 0 if it isn't
static bool validate_file_format(FILE* file) {

	unsigned char buffer[4];

	if(fseek(file, 0, SEEK_SET) != 0) {};
	if(fread(buffer, 1, 4, file) != 4) {};

	return !strncmp(buffer, "RIFF", 4);
}

int main(int argc, char *argv[]) {

	Options opt;
	opt.list = false;
	opt.count = false;
	opt.verbose = false;
	opt.data = "";
	opt.sub = "";
	
	struct argp argp = { options, parse_opt };
	return argp_parse(&argp, argc, argv, 0, 0, &opt);
}