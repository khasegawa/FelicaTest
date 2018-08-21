#include <stdio.h>
#include <stdint.h>
#include <shlobj.h>

#define FELICA_LIB_PATH  L"\\Sony Shared\\FeliCaLibrary\\felica.dll"
#define POLLING_INTERVAL 500

typedef struct {
	uint8_t *systemcode;
	uint8_t timeslot;
} POLLING;

typedef struct {
	uint8_t *idm;
	uint8_t *pmm;
} CARD_INF;

typedef struct {
	HINSTANCE hInst;
	BOOL (*read)(POLLING *, unsigned char *, CARD_INF *);
	BOOL (*dispose)(void);
} FELICA_LIB;

int open_felica_lib(FELICA_LIB *flibp, TCHAR *flibpath)
{
	TCHAR lpath[_MAX_PATH];
	HINSTANCE hInst;

	SHGetSpecialFolderPath(NULL, lpath, CSIDL_PROGRAM_FILES_COMMON, FALSE);
	wcscat_s(lpath, _MAX_PATH, flibpath);

	if ((hInst = LoadLibrary(lpath)) == NULL
	|| !(BOOL (*)(void))GetProcAddress(hInst, "initialize_library")()
	|| !(BOOL (*)(void))GetProcAddress(hInst, "open_reader_writer_auto")()
	|| !(flibp->read = (BOOL(*)(POLLING *, unsigned char *, CARD_INF *))GetProcAddress(hInst, "polling_and_get_card_information"))
	|| !(flibp->dispose = (BOOL(*)(void))GetProcAddress(hInst, "dispose_library"))) {
		return 1;
	}

	flibp->hInst = hInst;

	return 0;
}

int read_felica(uint8_t *idm, FELICA_LIB *flibp, uint8_t *systemcode, uint8_t timeslot)
{
	POLLING polling = { systemcode, timeslot };
	uint8_t ncards = 0;
	uint8_t pmm[8];
	CARD_INF cardinf = { idm, pmm };

	if (!flibp->read(&polling, &ncards, &cardinf) || ncards == 0) {
		return 1;
	}

	return 0;
}

void close_felica_lib(FELICA_LIB *flibp)
{
	flibp->dispose();
	FreeLibrary(flibp->hInst);
}

int main(int argc, char *argv[])
{
	uint8_t idm[8];
	FELICA_LIB flib;
	uint8_t systemcode[] = { 0xff, 0xff };  // Any type of FeliCa (note: network order)

	if (open_felica_lib(&flib, FELICA_LIB_PATH) != 0) {
		fprintf(stderr, "Felica Library open error\n");
		exit(1);
	}

	while (1) {
		if (read_felica(idm, &flib, systemcode, 0) == 0) {
			printf("IDm: ");
			for (int i = 0; i < 8; i++) {
				printf("%02X", idm[i]);
			}
			puts("");
		}
		Sleep(POLLING_INTERVAL);
	}

	close_felica_lib(&flib);

	return 0;
}
