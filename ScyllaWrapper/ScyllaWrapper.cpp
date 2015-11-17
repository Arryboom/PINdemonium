// ScyllaWrapper.cpp: definisce le funzioni esportate per l'applicazione DLL.
//

#pragma once
#include "stdafx.h"
#include "ScyllaWrapper.h"


#define SCYLLA_ERROR_FILE_FROM_PID -4
#define SCYLLA_ERROR_DUMP -3
#define SCYLLA_ERROR_IAT_NOT_FOUND -2
#define SCYLLA_ERROR_IAT_NOT_FIXED -1
#define SCYLLA_SUCCESS_FIX 0


/**
Extract the .EXE file which has lauched the process having PID pid
**/
BOOL GetFilePathFromPID(DWORD dwProcessId, WCHAR *filename){
	
	HANDLE processHandle = NULL;

	processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessId);
	if (processHandle) {
	if (GetModuleFileNameEx(processHandle,NULL, filename, MAX_PATH) == 0) {
    //if (GetProcessImageFileName(processHandle, filename, MAX_PATH) == 0) {
		printf("Failed to get module filename.\n");
		return false;
	}
	CloseHandle(processHandle);
	} else {
		printf("Failed to open process.\n" );
		return false;
	}

	return true;
	
}


DWORD_PTR GetExeModuleBase(DWORD dwProcessId)
{
	MODULEENTRY32 lpModuleEntry = { 0 };
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessId);
	lpModuleEntry.dwSize = sizeof(lpModuleEntry);
	Module32First(hSnapShot, &lpModuleEntry);

	CloseHandle(hSnapShot);

	return (DWORD_PTR)lpModuleEntry.modBaseAddr;
}

UINT32 IATAutoFix(DWORD pid, DWORD_PTR oep, WCHAR *outputFile)
{
	printf("----------------IAT Fixing Test----------------\n");

	
	DWORD_PTR iatStart = 0;
	DWORD iatSize = 0;
	WCHAR originalExe[MAX_PATH]; // Path of the original PE which as launched the current process
	WCHAR *dumpFile = L"./tmp_dump_file.exe";  //Path of the file where the process will be dumped during the Dumping Process

	//getting the Base Address
	DWORD_PTR hMod = GetExeModuleBase(pid);
	if(!hMod){
		printf("Can't find PID\n");
	}
	printf("GetExeModuleBase %X\n", hMod);

	//Dumping Process
	BOOL success = GetFilePathFromPID(pid,originalExe);
	if(!success){
		printf("Error in getting original Path from Pid: %d\n",pid);
		return SCYLLA_ERROR_FILE_FROM_PID;
	}
	printf("\nOriginal Exe Path: %S\n",originalExe);
		
	success = ScyllaDumpProcessW(pid,originalExe,hMod,oep,dumpFile);
	if(!success){
		printf("\n[SCYLLA DUMP] Error Dumping  Pid: %d, FileToDump: %S, Hmod: %X, oep: %X, output: %S \n",pid,originalExe,hMod,oep,dumpFile);
		return SCYLLA_ERROR_DUMP;
	}
	printf("\n[SCYLLA DUMP] Successfully dumped Pid: %d, FileToDump: %S, Hmod: %X, oep: %X, output: %S \n",pid,originalExe,hMod,oep,dumpFile);
		
	//DebugBreak();
	//Searching the IAT
	int error = ScyllaIatSearch(pid, &iatStart, &iatSize, hMod + 0x00001028, TRUE);
	if(error){
		printf("\n[SCYLLA SEARCH] error %d \n",error);
		return SCYLLA_ERROR_IAT_NOT_FOUND;
	}
	
	printf("\n[SCYLLA FIX] FIXING ...... iat_start : %08x\t iat_size : %08x\t pid : %d\n", iatStart,iatSize,pid,outputFile);

	//Fixing the IAT
	error = ScyllaIatFixAutoW(iatStart,iatSize,pid,dumpFile,outputFile);
	if(error){
		printf("\n[SCYLLA FIX] error %d\n",error);
		return SCYLLA_ERROR_IAT_NOT_FIXED;
	}
	printf("\n[SCYLLA FIX] Success\n");
	return SCYLLA_SUCCESS_FIX;
	
}




UINT32 ScyllaDumpAndFix(int pid, int oep, WCHAR * output_file){
	
	return IATAutoFix(pid, oep, output_file);
}


void ScyllaWrapAddSection(const WCHAR * dump_path , const CHAR * sectionName, DWORD sectionSize, UINT32 offset, BYTE * sectionData){
	ScyllaAddSection(dump_path , sectionName, sectionSize, offset , sectionData);
}


