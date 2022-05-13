
#include <stdlib.h>
#include <stdio.h>
#include "umka-lang/src/umka_api.h"
#define UMPROF_IMPL
#include "umprof.h"


void printHelp() {
	printf("Umprof - a simple umka profiler.\numprof [ -o output-file ] source-file\n");
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printHelp();
		return 1;
	}
	FILE *of = stdout;

	int argoff = 1;
	for (; argoff < argc-1; argoff++) {
		if (argv[argoff][0] != '-') break;

		if (strcmp(argv[argoff], "-o") == 0) {
			if (argoff == argc - 1) {
				fprintf(stderr, "umprof: missing argument\n");
				return 1;
			}

			of = fopen(argv[argoff + 1], "w");
			if (!of) {
				fprintf(stderr, "umprof: could not open file\n");
				return 1;
			}

			++argoff;
		}
	}

	void *umka = umkaAlloc();
	if (!umkaInit(umka, argv[argoff], NULL, 0, 1024 * 1024, NULL, argc - argoff, argv + argoff))
		return 1;
	umprofInit(umka);

	if (!umkaCompile(umka)) {
		UmkaError error;
		umkaGetError(umka, &error);
		fprintf(stderr, "Error %s (%d, %d): %s\n", error.fileName, error.line, error.pos, error.msg);
		return 1;
	}

	if (!umkaRun(umka)) {
    UmkaError error;
    umkaGetError(umka, &error);
    fprintf(stderr, "\nRuntime error %s (%d): %s\n", error.fileName, error.line, error.msg);
		return 1;
	}

	UmprofInfo info[2048] = {0};
	int len = umprofGetInfo(info, 2048);
	umprofPrintInfo(of, info, len);

	if (of != stdout) fclose(of);

	return 0;
}
