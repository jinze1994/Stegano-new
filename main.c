#include "stegano.h"
#include "mlbc.h"

const char* msg_usage = 
"Usage:  %s --write    [input_filename] [output_filename] [password] [message]\n"
"        %s --read     [input_filename] [password]\n"
"        %s --estimate [input_filename]\n"
"        %s --recode   [input_filename] [output_filename]\n"
"        %s --diff     [input_filename] [another_filename]\n";

int main(int argc, char* argv[]) {
	const char *cmd = NULL;
	const char *input_filename = NULL, *output_filename = NULL, *input_filename2 = NULL;
	const char *password = NULL, *message = NULL;
	FILE *input_file = NULL, *input_file2 = NULL, *output_file = NULL, *rep_file = NULL;

	// Read Arguments
	if (argc == 4 && !strcmp(argv[1], "--read"))
		cmd = argv[1], input_filename = argv[2], password = argv[3];
	else if (argc == 6 && !strcmp(argv[1], "--write"))
		cmd = argv[1], input_filename = argv[2], output_filename = argv[3], password = argv[4], message = argv[5];
	else if (argc == 3 && !strcmp(argv[1], "--estimate"))
		cmd = argv[1], input_filename = argv[2];
	else if (argc == 4 && !strcmp(argv[1], "--recode"))
		cmd = argv[1], input_filename = argv[2], output_filename = argv[3];
	else if (argc == 4 && !strcmp(argv[1], "--diff"))
		cmd = argv[1], input_filename = argv[2], input_filename2 = argv[3];
	else
		Panic(msg_usage, argv[0], argv[0], argv[0], argv[0], argv[0]);

	if (input_filename) {
		input_file = fopen(input_filename, "r");
		Assert(input_file != NULL, "Failed to open %s\n", input_filename);
	}
	if (input_filename2) {
		input_file2 = fopen(input_filename2, "r");
		Assert(input_file != NULL, "Failed to open %s\n", input_filename2);
	}
	if (output_filename) {
		output_file = fopen(output_filename, "w");
		Assert(output_file != NULL, "Failed to open %s\n", output_filename);
	}

	initMLBC();

	// Process Command
	int rv = 0;
	if (!strcmp(cmd, "--estimate")) {

		rv = printJpegQuantity(input_file);
		if (rv) goto cleanup;

	} else if (!strcmp(cmd, "--read")) {

		char buf[256];
		rv = steganoDecode(input_file, password, buf);
		if (rv) goto cleanup;

		printf("Decode Success!\nMessage: %s\n", buf);

	} else if (!strcmp(cmd, "--write")) {
		rep_file = tmpfile();
		// rep_file = fopen("tmp.jpg", "w+b");
		Assert(rep_file != NULL, "Failed to open temp file\n");

		rv = jpegChangeQuantity(input_file, rep_file, 75);
		if (rv) goto cleanup;
		rewind(rep_file);

		rv = printJpegQuantity(rep_file);
		if (rv) goto cleanup;
		rewind(rep_file);

		rv = steganoEncode(rep_file, output_file, message, password);
		if (rv) goto cleanup;

		printf("Encode Success!\n");

	} else if (!strcmp(cmd, "--recode")) {
		rv = jpegChangeQuantity(input_file, output_file, 80);
		if (rv) goto cleanup;

	} else if (!strcmp(cmd, "--diff")) {
		int cnt, w, h;
		rv = diff(input_file, input_file2, &cnt, &w, &h);
		if (rv) goto cleanup;
		printf("different byte: %d %lf\n", cnt, cnt * 1.0 / w / h / 3);

	} else
		Panic("Wrong command\n");

	// Clean up
cleanup:
	destroyMLBC();
	if (rv)
		printf("%s\n", stegano_describe(rv));

	if (input_file)
		fclose(input_file);
	if (output_file)
		fclose(output_file);
	if (rep_file)
		fclose(rep_file);
	return 0;
}
