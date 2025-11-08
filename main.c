
#include "umka-lang/src/umka_api.h"
#include <stdio.h>
#include <stdlib.h>
#define UMPROF_IMPL
#include "umprof.h"

static void
printHelp()
{
	printf("Umprof - a simple umka profiler.\numprof [ -j ] [ -o output-file ] source-file\n");
}

static void
warningCallback(UmkaError *warn)
{
	fprintf(stderr, "Warning: %s:%d: %s\n", warn->fileName, warn->line, warn->msg);
}

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		printHelp();
		return 1;
	}
	FILE *of = stdout;
	bool json = 0;

	int argoff = 1;
	for (; argoff < argc - 1; argoff++) {
		if (argv[argoff][0] != '-')
			break;

		if (strcmp(argv[argoff], "-j") == 0)
			json = 1;

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
	if (!umkaInit(umka, argv[argoff], NULL, 1024 * 1024, NULL, argc - argoff, argv + argoff,
		true, true, warningCallback))
		return 1;
	umprofInit(umka);

	if (!umkaCompile(umka)) {
		UmkaError *error = umkaGetError(umka);
		fprintf(stderr, "Error %s (%d, %d): %s\n", error->fileName, error->line, error->pos,
		    error->msg);
		return 1;
	}

	int exitCode = umkaRun(umka);
	if (exitCode != 0) {
		UmkaError *error = umkaGetError(umka);
		fprintf(stderr, "\nRuntime error %s (%d): %s\n", error->fileName, error->line,
		    error->msg);
		return exitCode;
	}

	if (json) {
		umprofPrintEventsJSON(of);
	} else {
		UmprofInfo info[2048] = {0};
		int len = umprofGetInfo(info, 2048);
		umprofPrintInfo(of, info, len);
	}

	if (of != stdout)
		fclose(of);

	return 0;
}
