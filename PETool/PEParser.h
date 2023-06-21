#pragma once

#include<string>
#include<vector>
#include<Windows.h>
#include "Defines.h"
using namespace std;

typedef struct PEFunction {
	WORD ordinal;
	CString name;
	DWORD address;
}PEFunctuion,*PPEFunction;

typedef struct ExportData {
	CString moudleName;
	vector<PEFunction> functions;
}ExportData,* PExportData;

typedef struct ImportData {
	CString moudleName;
	vector<PEFunction> functions;
}ImportData,*PImportData;

class PEParser
{
private:
	CString mPath;
	//模块加载首地址
	char* mImageBase;
	HANDLE mFileHandle;
	HANDLE mMappingHandle;

	//头部
	IMAGE_DOS_HEADER mDosHeader = IMAGE_DOS_HEADER();
	IMAGE_NT_HEADERS mNtHeader = IMAGE_NT_HEADERS();
	vector<IMAGE_SECTION_HEADER> mSectionHeaders = vector<IMAGE_SECTION_HEADER>();
	//表
	vector<ExportData> mExportDirectory = vector<ExportData>();
	vector<ImportData> mImportDescriptors = vector<ImportData>();
	IMAGE_BASE_RELOCATION mRelocation = IMAGE_BASE_RELOCATION();

	//DWORD rvaToFileOffset(DWORD rva);
public:

	PEParser(CString& path) :mPath(path) {
		mFileHandle = CreateFile(path.GetString(),  GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (mFileHandle ==INVALID_HANDLE_VALUE)
		{
			printf("无法打开文件");
			goto fail;
		}

		DWORD ret= GetFileSize(mFileHandle, NULL);
		if (ret== INVALID_FILE_SIZE)
		{
			printf("无法打开文件");
		}
		else {
			printf("fileSize=%d", ret);

		}


		mMappingHandle = CreateFileMapping(mFileHandle, NULL, PAGE_READONLY, 0, 0, L"PEParser");
		if (mMappingHandle == INVALID_HANDLE_VALUE)
		{
			printf("无法创建映射");
			goto fail;
		}

		mImageBase = (char*)MapViewOfFile(mMappingHandle, FILE_MAP_READ, 0, 0, 0);

		if (mImageBase==NULL)
		{
			printf("无法映射文件");
			goto fail;
		}

	fail:
		printf("无法解析文件");
	};

	virtual ~PEParser() {

	
		if (!mPath.IsEmpty()&&mImageBase)
		{
			UnmapViewOfFile(mImageBase);
		}

		if (mMappingHandle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(mMappingHandle);
		}

		if (mFileHandle!=INVALID_HANDLE_VALUE)
		{
			CloseHandle(mFileHandle);
		}
	};
	bool isPEFile();
	/*解析表头（不包括数据）*/
	bool parseHeaders();
	/*解析数据表*/
	bool parseDirectories();

	IMAGE_DOS_HEADER& getDosHeader();
	IMAGE_NT_HEADERS& getNtHeader();
	vector<IMAGE_SECTION_HEADER>& getSectionHeaders();
	vector<ExportData>& getExportData();
	vector<ImportData>& getImportData();
	IMAGE_BASE_RELOCATION& getReLocation();
};

