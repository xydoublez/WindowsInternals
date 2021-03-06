#include "stdafx.h"

#pragma comment(lib, "ntdll")

extern "C" {
	#include "ntndk.h"
}

typedef enum _MI_SYSTEM_VA_TYPE {
	MiVaUnused = 0,
	MiVaSessionSpace = 1,
	MiVaProcessSpace = 2,
	MiVaBootLoaded = 3,
	MiVaPfnDatabase = 4,
	MiVaNonPagedPool = 5,
	MiVaPagedPool = 6,
	MiVaSpecialPool = 7,
	MiVaSystemCache = 8,
	MiVaSystemPtes = 9,
	MiVaHal = 10,
	MiVaSessionGlobalSpace = 11,
	MiVaDriverImageSpace = 12,
	MiVaMaximumType = 13
} MI_SYSTEM_VA_TYPE;

typedef struct _MM_SYSTEM_VA_LIST_ENTRY {
	ULONG Number;
	ULONG Peak;
	ULONG Limit;
	ULONG Failures;
} MM_SYSTEM_VA_LIST_ENTRY, *PMM_SYSTEM_VA_LIST_ENTRY;

typedef struct _MM_SYSTEM_VA_LIST {
	ULONG AvailableVa;
	ULONG AvailableVaLow;
	ULONG Reserved;
	ULONG TotalFailures;
	MM_SYSTEM_VA_LIST_ENTRY Entry[5];
} MM_SYSTEM_VA_LIST, *PMM_SYSTEM_VA_LIST;

int main(int argc, const char* argv[]) {
	NTSTATUS ntStatus;
	BOOLEAN old;
	ULONG resultLength;
	MM_SYSTEM_VA_LIST systemVaInfo = { 0 };
	ULONG i;
	ULONG SetNonPaged = 0, SetPaged = 0, SetSession = 0, SetSystem = 0, SetCache = 0, SetQuery = 0, SetRegistry = 0;

	//
	// Print header
	//
	printf("\nMemLimit v1.00 - Query and set hard limits on system VA space consumption\n");
	printf("Copyright (C) 2008 Alex Ionescu\n");
	printf("www.alex-ionescu.com\n\n");

	//
	// Check if no parameters given
	//
	if (argc == 1)
	{
		//
		// Display usage
		//
		printf("usage: memlimit [-q|-r][-n size] [-p size] [-s size] [-c size] [-t size]\n");
		printf("%6s    Queries the current consumption, peak, and maximums on system VA for this system and does not set any limits\n", "-q");
		printf("%6s    Specifies that the limits being set should be written to the registry, making them persistent\n", "-r");
		printf("%6s    Specifies the maximum amount of system VA space that can be used by the nonpaged pool\n"
			"          Under certain conditions, this limit may be exceeded by a small amount\n", "-n");
		printf("%6s    Specifies the maximum amount of system VA space that can be used by the paged pool\n", "-p");
		printf("%6s    Specifies the maximum amount of system VA space that can be used by session space allocations\n", "-s");
		printf("%6s    Specifies the maximum amount of system VA space that can be used by the system cache\n"
			"          Under certain conditions, this limit may be exceeded by a small amount\n", "-c");
		printf("%6s    Specifies the maximum amount of system VA space that can be used by I/O mappings\n"
			"          and other resources that consume system page table entries (PTEs)\n", "-t");
		printf("\nThe default is 0, which means no limit is enforced.\n"
			"Sizes are in MB automatically rounded up to the next system VA allocation boundary,\n"
			"which is 2 MB on 32-bit systems that have Physical Address Extension (PAE) enabled\n"
			"and 4 MB on 32-bit systems that do not have PAE enabled.\n");
		return -1;
	}

	//
	// Get increase working set privilege
	//
	ntStatus = RtlAdjustPrivilege(SE_INCREASE_QUOTA_PRIVILEGE, TRUE, FALSE, &old);
	if (!NT_SUCCESS(ntStatus))
	{
		//
		// Display error
		//
		printf("Failed to get required privilege.\n");
		printf("Make sure that you are running with administrative privileges\n"
			"and that your account has the increase quota privilege\n");
		return -1;
	}

	//
	// Check what we should do
	//
	for (i = 0; i < (ULONG)argc; i++)
	{
		//
		// Check what the user wants
		//
		if (strstr(argv[i], "-q")) SetQuery = i + 1;
		if (strstr(argv[i], "-r")) SetRegistry = i + 1;
		if (strstr(argv[i], "-n")) SetNonPaged = i + 1;
		if (strstr(argv[i], "-p")) SetPaged = i + 1;
		if (strstr(argv[i], "-s")) SetSession = i + 1;
		if (strstr(argv[i], "-c")) SetCache = i + 1;
		if (strstr(argv[i], "-t")) SetSystem = i + 1;
	}

	//
	// Do a query to get current limits
	//
	ntStatus = NtQuerySystemInformation((SYSTEM_INFORMATION_CLASS) 106,
		&systemVaInfo,
		sizeof(systemVaInfo),
		&resultLength);
	if (!NT_SUCCESS(ntStatus))
	{
		//
		// Shouldn't really happen
		//
		printf("Failed to query memory limits. This tool only supports Vista SP1/Server 08 and higher!\n");
		return -1;
	}

	//
	// Check if this is only a query
	//
	if (SetQuery)
	{
	doQuery:
		//
		// Tell the user
		//
		printf("System Va Consumption:\n\nType\t\t\tCurrent\t\t   Peak\t\t   Limit \n"
			"Non Paged Pool\t\t%7d KB\t%7d KB\t%7d KB\n"
			"Paged Pool\t\t%7d KB\t%7d KB\t%7d KB\n"
			"System Cache\t\t%7d KB\t%7d KB\t%7d KB\n"
			"System PTEs\t\t%7d KB\t%7d KB\t%7d KB\n"
			"Session Space\t\t%7d KB\t%7d KB\t%7d KB\n",
			systemVaInfo.Entry[0].Number / 1024,
			systemVaInfo.Entry[0].Peak / 1024,
			systemVaInfo.Entry[0].Limit / 1024,
			systemVaInfo.Entry[1].Number / 1024,
			systemVaInfo.Entry[1].Peak / 1024,
			systemVaInfo.Entry[1].Limit / 1024,
			systemVaInfo.Entry[2].Number / 1024,
			systemVaInfo.Entry[2].Peak / 1024,
			systemVaInfo.Entry[2].Limit / 1024,
			systemVaInfo.Entry[3].Number / 1024,
			systemVaInfo.Entry[3].Peak / 1024,
			systemVaInfo.Entry[3].Limit / 1024,
			systemVaInfo.Entry[4].Number / 1024,
			systemVaInfo.Entry[4].Peak / 1024,
			systemVaInfo.Entry[4].Limit / 1024);
		return 0;
	}

	//
	// Now get the limits
	//
	if (SetNonPaged) systemVaInfo.Entry[0].Limit = atol(argv[SetNonPaged]) * 1024 * 1024;
	if (SetPaged) systemVaInfo.Entry[1].Limit = atol(argv[SetPaged]) * 1024 * 1024;
	if (SetCache) systemVaInfo.Entry[2].Limit = atol(argv[SetCache]) * 1024 * 1024;
	if (SetSystem) systemVaInfo.Entry[3].Limit = atol(argv[SetSystem]) * 1024 * 1024;
	if (SetSession) systemVaInfo.Entry[4].Limit = atol(argv[SetSession]) * 1024 * 1024;

	//
	// And set them!
	//
	ntStatus = NtSetSystemInformation((SYSTEM_INFORMATION_CLASS) 106,
		&systemVaInfo,
		sizeof(systemVaInfo));
	if (!NT_SUCCESS(ntStatus))
	{
		//
		// Shouldn't really happen
		//
		printf("Failed to set memory limits. This tool only supports Vista SP1/Server 08 and higher!\n");
		return -1;
	}

	//
	// Print current state
	//
	printf("Limits set succesfully!\n\n");
	goto doQuery;
}
