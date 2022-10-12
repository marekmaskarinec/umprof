// umprof - a profiler for umka, public domain
// contact: <marek@mrms.cz>
//
// for example usage see main.c in this repo

#ifndef UMPROF_H
#define UMPROF_H

#include <time.h>
#include <string.h>

// maximum length of a function name
#ifndef UMPROF_STRING_LENGTH
#define UMPROF_STRING_LENGTH 2048
#endif

typedef enum {
	EventCall,
	EventReturn,
} UmprofEventType;

// Stores either a function call or a return
typedef struct {
	UmprofEventType type;
	clock_t clock;
	const char *name;
} UmprofEvent;

// Info about a specific function
typedef struct {
	const char *name;
	int callCount;
	clock_t clock;
	clock_t clockSelf;
	float percent;
	float percentSelf;
} UmprofInfo;

// Struct used while parsing events to info
typedef struct {
	UmprofInfo *arr;
	int arrlen;
	int arrcap;
} UmprofEventParser;

// Registers hook functions to the umka instance.
void umprofInit(void *umka);

// Parser collected events into and stores them in output. maxInfo is the size
// of the output buffer. Returns the amount of functions stored in output.
int umprofGetInfo(UmprofInfo *output, int maxInfo);

// Prints a table made from arr into the file f.
void umprofPrintInfo(FILE *f, UmprofInfo *arr, int arrlen);

#ifdef UMPROF_IMPL

UmprofEvent *umprofEvents = NULL;
int umprofEventCount;
int umprofEventCap;

static void umprofAddEvent(UmprofEventType type, const char *name) {
	if (umprofEventCount == umprofEventCap) {
		umprofEventCap *= 2;
		umprofEvents = realloc(umprofEvents, umprofEventCap * sizeof(UmprofEvent));
		if (umprofEvents == NULL)
			fprintf(stderr, "umprof: oom\n");
	}

	umprofEvents[umprofEventCount++] = (UmprofEvent){
		.type = type, .name = name, .clock = clock() };
}

static void umprofCallHook(const char *filename, const char *funcName, int line) {
	umprofAddEvent(EventCall, funcName);	
}

static void umprofReturnHook(const char *filename, const char *funcName, int line) {
	umprofAddEvent(EventReturn, funcName);
}

void umprofInit(void *umka) {
	free(umprofEvents);
	umkaSetHook(umka, UMKA_HOOK_CALL, umprofCallHook);
	umkaSetHook(umka, UMKA_HOOK_RETURN, umprofReturnHook);

	umprofEvents = malloc(sizeof(UmprofEvent) * 512);
	umprofEventCount = 0;
	umprofEventCap = 512;
}

static UmprofInfo *umprofGetFunc(UmprofEventParser *par, const char *name) {
	for (int i=0; i < par->arrlen; i++)
		if (par->arr[i].name && strcmp(par->arr[i].name, name) == 0) return &par->arr[i];

	if (par->arrlen == par->arrcap) return NULL;

	UmprofInfo *info = &par->arr[par->arrlen++];
	info->name = name;
	info->callCount = 0;

	return info;
}

static UmprofEvent *umprofParseEvent(UmprofEventParser *par, UmprofInfo *out, UmprofEvent *in) {
	clock_t noRec = out->clock;
	clock_t noRecSelf = out->clockSelf;
	clock_t notSelf = 0;
	UmprofEvent *p = in + 1;
	while (p && p->type != EventReturn) {
		UmprofInfo *info = umprofGetFunc(par, p->name);
		if (!info) return 0;

		clock_t offset = info->clock;
		p = umprofParseEvent(par, info, p);
		if (p == NULL) return p;
		notSelf += info->clock - offset;

		++p;
	}

	out->callCount++;
	out->clock = noRec + p->clock - in->clock;
	out->clockSelf = noRecSelf + p->clock - in->clock - notSelf;

	return p;
}

int umprofGetInfo(UmprofInfo *output, int maxInfo) {
	if (!umprofEventCount) return 1;

	UmprofEventParser par = {
		.arr = output,
		.arrcap = maxInfo};

	UmprofInfo *main = umprofGetFunc(&par, umprofEvents[0].name);
	if (!umprofParseEvent(&par, main, &umprofEvents[0]))
		return 0;

	clock_t total = umprofEvents[umprofEventCount-1].clock - umprofEvents[0].clock;
	
	for (int i=0; i < par.arrlen; i++) {
		par.arr[i].percent = par.arr[i].clock / (float)total;
		par.arr[i].percentSelf = par.arr[i].clockSelf / (float)total;
	}

	return par.arrlen;
}

void umprofPrintInfo(FILE *f, UmprofInfo *arr, int arrlen) {
#define MAX_NUMBER 14
#define MAX_FLOAT 14
#define MAX_FUNC_NAME 30
	fprintf(f, "%*s%*s%*s%*s%*s%*s\n",
		MAX_NUMBER, "count",
		MAX_FLOAT, "%", MAX_FLOAT, "% self",
		MAX_NUMBER, "clock", MAX_NUMBER, "clock self",
		MAX_FUNC_NAME, "function name");
	for (int i=0; i < MAX_FLOAT * 2 + MAX_NUMBER * 3 + MAX_FUNC_NAME; i++)
		fprintf(f, "-");
	fprintf(f, "\n");

	for (int i=0; i < arrlen; i++) {
		fprintf(f, "%*d%*f%*f%*ld%*ld%*s\n", MAX_NUMBER, arr[i].callCount,
			MAX_FLOAT, arr[i].percent, MAX_FLOAT, arr[i].percentSelf,
			MAX_NUMBER, arr[i].clock, MAX_NUMBER, arr[i].clockSelf,
			MAX_FUNC_NAME, arr[i].name);
	}
#undef MAX_NUMBER
#undef MAX_FLOAT
#undef MAX_FUNC_NAME
}

void umprofPrintEventsJSON(FILE *f) {
	const float start = (float)umprofEvents[0].clock / CLOCKS_PER_SEC * 1000000 - 1;

	fprintf(f, "[");

	for (int i=0; i < umprofEventCount; ++i)
		fprintf(f,
			"{\"cat\":\"function\",\"name\":\"%s\",\"ph\":\"%s\","
			"\"pid\":0,\"tid\":0,\"ts\":%g}%s",
			umprofEvents[i].name,
			umprofEvents[i].type == EventCall ? "B" : "E",
			(float)umprofEvents[i].clock / CLOCKS_PER_SEC * 1000000 - start,
			i < umprofEventCount - 1 ? "," : "");

	fprintf(f, "]\n");
}

#endif // UMPROF_IMPL
#endif // !UMPROF_H
