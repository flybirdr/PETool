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
	//ģ������׵�ַ
	char* mImageBase;
	HANDLE mFileHandle;
	HANDLE mMappingHandle;

	//ͷ��
	IMAGE_DOS_HEADER mDosHeader = IMAGE_DOS_HEADER();
	IMAGE_NT_HEADERS mNtHeader = IMAGE_NT_HEADERS();
	vector<IMAGE_SECTION_HEADER> mSectionHeaders = vector<IMAGE_SECTION_HEADER>();
	//��
	vector<ExportData> mExportDirectory = vector<ExportData>();
	vector<ImportData> mImportDescriptors = vector<ImportData>();
	IMAGE_BASE_RELOCATION mRelocation = IMAGE_BASE_RELOCATION();

	//DWORD rvaToFileOffset(DWORD rva);
public:

	PEParser(CString& path) :mPath(path) {
		mFileHandle = CreateFile(path.GetString(),  GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (mFileHandle ==INVALID_HANDLE_VALUE)
		{
			printf("�޷����ļ�");
			goto fail;
		}

		DWORD ret= GetFileSize(mFileHandle, NULL);
		if (ret== INVALID_FILE_SIZE)
		{
			printf("�޷����ļ�");
		}
		else {
			printf("fileSize=%d", ret);

		}


		mMappingHandle = CreateFileMapping(mFileHandle, NULL, PAGE_READONLY, 0, 0, L"PEParser");
		if (mMappingHandle == INVALID_HANDLE_VALUE)
		{
			printf("�޷�����ӳ��");
			goto fail;
		}

		mImageBase = (char*)MapViewOfFile(mMappingHandle, FILE_MAP_READ, 0, 0, 0);

		if (mImageBase==NULL)
		{
			printf("�޷�ӳ���ļ�");
			goto fail;
		}

	fail:
		printf("�޷������ļ�");
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
	/*������ͷ�����������ݣ�*/
	bool parseHeaders();
	/*�������ݱ�*/
	bool parseDirectories();

	IMAGE_DOS_HEADER& getDosHeader();
	IMAGE_NT_HEADERS& getNtHeader();
	vector<IMAGE_SECTION_HEADER>& getSectionHeaders();
	vector<ExportData>& getExportData();
	vector<ImportData>& getImportData();
	IMAGE_BASE_RELOCATION& getReLocation();
};

