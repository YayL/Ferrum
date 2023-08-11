#include "common/io.h"

FILE * open_file (const char * filename, const char * options) {
	FILE * out = fopen(filename, options);
	if (out == NULL) {
		println("[Error]: Cannot open file: '{s}'", filename);
		exit(1);
	}
	return out;
}

char* read_file(const char * filename, size_t * length) {

	FILE * fp;
	char * line = NULL, * buffer = NULL;
    size_t temp = 0;

	fp = open_file(filename, "rb");	

	buffer = malloc(sizeof(char));
	buffer[0] = 0;

	while (getline(&line, length, fp) != -1) {
        temp += *length;
		buffer = format("{2s}", buffer, line);
	}

	fclose(fp);
	if (line)
		free(line);
    
    *length = temp;

	return buffer;
}

void write_file(const char * filename, char * write_buffer) {
	FILE * fp;
	
	fp = open_file(filename, "wb");

	if (!fputs(write_buffer, fp)) {
		println("Error: Unable to write buffer to file: '{s}'", filename);
		exit(1);
	}
	fclose(fp);
}

char * get_abs_path(const char * path) {

    char buf[PATH_MAX + 1] = {0};
    realpath(path, buf);

    unsigned long length = strlen(buf);
    char * ptr = malloc(sizeof(char) * (length + 1));
    memcpy(ptr, buf, length);
    ptr[length] = '\0';

    return ptr;
}
