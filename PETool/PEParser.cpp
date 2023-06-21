#include "pch.h"
#include "PEParser.h"

bool PEParser::isPEFile()
{
	//��ַ
	if (!mImageBase)
	{
		return false;
	}
	//�ж��Ƿ�ΪMZ
	auto pDosHeader = (PIMAGE_DOS_HEADER)(mImageBase);
	if (pDosHeader->e_magic!=IMAGE_DOS_SIGNATURE)
	{
		return false;
	}
	//�ж��Ƿ�ΪPE��ʽ
	auto pNtHeader = (PIMAGE_NT_HEADERS)(mImageBase + pDosHeader->e_lfanew);
	if (pNtHeader->Signature!=IMAGE_NT_SIGNATURE)
	{
		return false;
	}
    return true;
}

bool PEParser::parseHeaders()
{
	if (!isPEFile())
	{
		return false;
	}
	//����DOSͷ
	auto dosSize = sizeof(IMAGE_DOS_HEADER);
	memcpy_s(&mDosHeader, dosSize, mImageBase, dosSize);

	//����NTͷ
	auto ntSize = sizeof(IMAGE_NT_HEADERS);
	auto mNtBase = mImageBase + mDosHeader.e_lfanew;
	memcpy_s(&mNtHeader, ntSize, mNtBase, ntSize);

	//���������ͷ
	auto sectionCount = mNtHeader.FileHeader.NumberOfSections;
	auto pSectionHeader = IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS)mNtBase);
	auto sectionHeaderSize = sizeof(IMAGE_SECTION_HEADER);
	for (int i = 0; i < sectionCount; i++)
	{
		IMAGE_SECTION_HEADER sectionHeader;
		memset(&sectionHeader, 0, sectionHeaderSize);
		memcpy_s(&sectionHeader, sectionHeaderSize, pSectionHeader, sectionHeaderSize);
		mSectionHeaders.push_back(sectionHeader);
		++pSectionHeader;
	}

    return true;
}

bool PEParser::parseDirectories()
{

	if (!isPEFile())
	{
		return false;
	}
	int deltaRVA = 0;
	PIMAGE_DATA_DIRECTORY pDataDirectory = mNtHeader.OptionalHeader.DataDirectory;
	for (size_t i = 0; ; i++)
	{
		if (pDataDirectory->VirtualAddress==0)
		{
			++pDataDirectory;
			continue;
		}

		auto ntBase = mImageBase + mDosHeader.e_lfanew;
		auto sectionCount = mNtHeader.FileHeader.NumberOfSections;
		auto pSectionHeader = IMAGE_FIRST_SECTION((PIMAGE_NT_HEADERS)ntBase);
		
		//��λ�������飬�����ļ�ƫ��
		int sectionLoc = -1;
		for (size_t j = 0; j < sectionCount; j++)
		{
			auto sectionHeader = pSectionHeader + j;
			//С��һ���������ʼλ�þͱ�ʾ��ǰ������ǰһ��������
			if (pDataDirectory->VirtualAddress< sectionHeader->VirtualAddress)
			{
				sectionLoc = j - 1;
				break;
			}
		}
		if (sectionLoc==-1)
		{
			break;
		}
		//��λ����������
		pSectionHeader += sectionLoc;
		//�������RVA���ļ�ƫ�ƵĲ�ֵ
		DWORD delta = pSectionHeader->VirtualAddress - pSectionHeader->PointerToRawData;
		//�����ļ�ƫ��
		DWORD fileOffset = pDataDirectory->VirtualAddress - delta;

		//0�������
		if (i==IMAGE_DIRECTORY_ENTRY_EXPORT)
		{
			
			PIMAGE_EXPORT_DIRECTORY pExportDirectory =(PIMAGE_EXPORT_DIRECTORY)(mImageBase + fileOffset);
			
				int numberOfFunction = pExportDirectory->NumberOfFunctions;
				int numberOfName = pExportDirectory->NumberOfNames;

				DWORD* addressOfFunctions = (DWORD*)(mImageBase + (pExportDirectory->AddressOfFunctions - delta));
				DWORD* addressOfNames = (DWORD*)(mImageBase + (pExportDirectory->AddressOfNames - delta));
				WORD* addressOfNameOrdinals = (WORD*)(mImageBase + (pExportDirectory->AddressOfNameOrdinals - delta));
				
				ExportData exportData;
				exportData.moudleName = CString(mImageBase + (pExportDirectory->Name - delta));
				//�Ȱ����еĺ�����ַ������
				for (size_t i = 0; i < numberOfFunction; i++)
				{
					PEFunction peFunction;
					peFunction.address = addressOfFunctions[i];
					exportData.functions.push_back(peFunction);
				}
				//Ȼ���ڵ�������
				for (size_t i = 0; i < numberOfName; i++)
				{
					WORD ordinal = addressOfNameOrdinals[i];
					exportData.functions[ordinal].name = CString(mImageBase + (addressOfNames[i] - delta));
				}

				mExportDirectory.push_back(exportData);
			
			
			
		}
		//1�������
		else if (i==IMAGE_DIRECTORY_ENTRY_IMPORT)
		{
			PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(mImageBase + fileOffset);
			//���������ÿ��IMAGE_IMPORT_DESCRIPTOR�ṹ��Ӧһ����̬���ӿ�
			while (pImportDescriptor->FirstThunk)
			{

				DWORD offsetToOriginalFirstThunk = pImportDescriptor->OriginalFirstThunk - delta;
				DWORD offsetToFirstThunk = pImportDescriptor->FirstThunk - delta;
				PIMAGE_THUNK_DATA pOriginalThunkData =(PIMAGE_THUNK_DATA)(mImageBase + offsetToOriginalFirstThunk);
				PIMAGE_THUNK_DATA pThunkData = (PIMAGE_THUNK_DATA)(mImageBase + offsetToFirstThunk);

				ImportData importData;
				importData.moudleName = CString(mImageBase + (pImportDescriptor->Name - delta));

				int sizeOfDWORD = sizeof(DWORD);
				//����IMAGE_THUNK_DATA�ṹ���飬ÿ���ṹ��Ӧһ�����뺯��
				for (;;)
				{
					//�ж�OriginalThunk�Ƿ����,������һ������Ϊ0��IMAGE_THUNK_DATA�ṹ����
					if (*(ULONGLONG*)pOriginalThunkData==0)
					{
						break;
					}
					else {
						PEFunction peFunction;
						//���Ϊ1����ŵ���
						if (pOriginalThunkData->u1.Ordinal >> 63) {
							peFunction.ordinal = (pOriginalThunkData->u1.Ordinal << 1) >> 1;
						}
						//Ϊ0��ʾ�Ժ������Ƶ���
						else
						{
							peFunction.name = CString(mImageBase + (pOriginalThunkData->u1.ForwarderString - delta) + sizeof(WORD));
						}
						//��ȡ������ַRVA
						peFunction.address = pThunkData->u1.AddressOfData;

						importData.functions.push_back(peFunction);
					}
					++pOriginalThunkData;
					++pThunkData;
				}

				mImportDescriptors.push_back(importData);
				++pImportDescriptor;
			}
		}
		//2����Դ��
		else if (i==IMAGE_DIRECTORY_ENTRY_RESOURCE)
		{

		}
		//3���쳣��
		else if (i==IMAGE_DIRECTORY_ENTRY_EXCEPTION)
		{

		}
		//4����ȫ��
		else if (i==IMAGE_DIRECTORY_ENTRY_SECURITY)
		{

		}
		//5���ض�λ��
		else if (i==IMAGE_DIRECTORY_ENTRY_BASERELOC)
		{

		}
		//6�����Ա�
		else if (i==IMAGE_DIRECTORY_ENTRY_DEBUG)
		{

		}
#ifdef _WIN64
		//x64û�������
		else if (i == 7)
		{
			continue;
		}
#else
		//7��CopyRight
		else if (i == IMAGE_DIRECTORY_ENTRY_COPYRIGHT)
		{

		}
#endif // X64
		//8��Global Ptr
		else if (i==IMAGE_DIRECTORY_ENTRY_GLOBALPTR)
		{

		}
		//9��TLS
		else if (i==IMAGE_DIRECTORY_ENTRY_TLS)
		{

		}
		//10��load config
		else if (i==IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG)
		{

		}
		//11��bound import
		else if (i==IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT)
		{

		}
		//12��IAT
		else if (i==IMAGE_DIRECTORY_ENTRY_IAT)
		{

		}
		//13��Delay import
		else if (i==IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT)
		{

		}
		//14��COM descriptor
		else if (i==IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR)
		{

		}
		//15��0
		else
		{
			break;
		}

		++pDataDirectory;
	}


	return false;
}

IMAGE_DOS_HEADER& PEParser::getDosHeader()
{
	return mDosHeader;
}

IMAGE_NT_HEADERS& PEParser::getNtHeader()
{
	return mNtHeader;
}

vector<IMAGE_SECTION_HEADER>& PEParser::getSectionHeaders()
{
	return mSectionHeaders;
}

vector<ExportData>& PEParser::getExportData()
{
	return mExportDirectory;
}

vector<ImportData>& PEParser::getImportData()
{
	return mImportDescriptors;
}

IMAGE_BASE_RELOCATION& PEParser::getReLocation()
{
	return mRelocation;
}
