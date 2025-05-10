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
	char * buffer;
    size_t size;

	fp = open_file(filename, "rb");	
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);

	buffer = malloc((size + 1) * sizeof(char));
    
    fread_unlocked(buffer, sizeof(char), size, fp);
	fclose(fp);
    buffer[size] = '\0';
    
    if (length != NULL)
        *length = size;

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
