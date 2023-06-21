#include "pch.h"
#include "PEParser.h"

bool PEParser::isPEFile()
{
	//基址
	if (!mImageBase)
	{
		return false;
	}
	//判断是否为MZ
	auto pDosHeader = (PIMAGE_DOS_HEADER)(mImageBase);
	if (pDosHeader->e_magic!=IMAGE_DOS_SIGNATURE)
	{
		return false;
	}
	//判断是否为PE格式
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
	//拷贝DOS头
	auto dosSize = sizeof(IMAGE_DOS_HEADER);
	memcpy_s(&mDosHeader, dosSize, mImageBase, dosSize);

	//拷贝NT头
	auto ntSize = sizeof(IMAGE_NT_HEADERS);
	auto mNtBase = mImageBase + mDosHeader.e_lfanew;
	memcpy_s(&mNtHeader, ntSize, mNtBase, ntSize);

	//解析区块表头
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
		
		//定位所在区块，计算文件偏移
		int sectionLoc = -1;
		for (size_t j = 0; j < sectionCount; j++)
		{
			auto sectionHeader = pSectionHeader + j;
			//小于一个区块的起始位置就表示当前区块在前一个区块中
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
		//定位到所在区块
		pSectionHeader += sectionLoc;
		//计算计算RVA与文件偏移的差值
		DWORD delta = pSectionHeader->VirtualAddress - pSectionHeader->PointerToRawData;
		//计算文件偏移
		DWORD fileOffset = pDataDirectory->VirtualAddress - delta;

		//0、输出表
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
				//先把所有的函数地址都导出
				for (size_t i = 0; i < numberOfFunction; i++)
				{
					PEFunction peFunction;
					peFunction.address = addressOfFunctions[i];
					exportData.functions.push_back(peFunction);
				}
				//然后在导出名称
				for (size_t i = 0; i < numberOfName; i++)
				{
					WORD ordinal = addressOfNameOrdinals[i];
					exportData.functions[ordinal].name = CString(mImageBase + (addressOfNames[i] - delta));
				}

				mExportDirectory.push_back(exportData);
			
			
			
		}
		//1、输入表
		else if (i==IMAGE_DIRECTORY_ENTRY_IMPORT)
		{
			PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(mImageBase + fileOffset);
			//遍历输入表，每个IMAGE_IMPORT_DESCRIPTOR结构对应一个动态链接库
			while (pImportDescriptor->FirstThunk)
			{

				DWORD offsetToOriginalFirstThunk = pImportDescriptor->OriginalFirstThunk - delta;
				DWORD offsetToFirstThunk = pImportDescriptor->FirstThunk - delta;
				PIMAGE_THUNK_DATA pOriginalThunkData =(PIMAGE_THUNK_DATA)(mImageBase + offsetToOriginalFirstThunk);
				PIMAGE_THUNK_DATA pThunkData = (PIMAGE_THUNK_DATA)(mImageBase + offsetToFirstThunk);

				ImportData importData;
				importData.moudleName = CString(mImageBase + (pImportDescriptor->Name - delta));

				int sizeOfDWORD = sizeof(DWORD);
				//遍历IMAGE_THUNK_DATA结构数组，每个结构对应一个输入函数
				for (;;)
				{
					//判断OriginalThunk是否结束,数组以一个内容为0的IMAGE_THUNK_DATA结构结束
					if (*(ULONGLONG*)pOriginalThunkData==0)
					{
						break;
					}
					else {
						PEFunction peFunction;
						//最高为1以序号导入
						if (pOriginalThunkData->u1.Ordinal >> 63) {
							peFunction.ordinal = (pOriginalThunkData->u1.Ordinal << 1) >> 1;
						}
						//为0表示以函数名称导入
						else
						{
							peFunction.name = CString(mImageBase + (pOriginalThunkData->u1.ForwarderString - delta) + sizeof(WORD));
						}
						//获取函数地址RVA
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
		//2、资源表
		else if (i==IMAGE_DIRECTORY_ENTRY_RESOURCE)
		{

		}
		//3、异常表
		else if (i==IMAGE_DIRECTORY_ENTRY_EXCEPTION)
		{

		}
		//4、安全表
		else if (i==IMAGE_DIRECTORY_ENTRY_SECURITY)
		{

		}
		//5、重定位表
		else if (i==IMAGE_DIRECTORY_ENTRY_BASERELOC)
		{

		}
		//6、调试表
		else if (i==IMAGE_DIRECTORY_ENTRY_DEBUG)
		{

		}
#ifdef _WIN64
		//x64没有这个表
		else if (i == 7)
		{
			continue;
		}
#else
		//7、CopyRight
		else if (i == IMAGE_DIRECTORY_ENTRY_COPYRIGHT)
		{

		}
#endif // X64
		//8、Global Ptr
		else if (i==IMAGE_DIRECTORY_ENTRY_GLOBALPTR)
		{

		}
		//9、TLS
		else if (i==IMAGE_DIRECTORY_ENTRY_TLS)
		{

		}
		//10、load config
		else if (i==IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG)
		{

		}
		//11、bound import
		else if (i==IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT)
		{

		}
		//12、IAT
		else if (i==IMAGE_DIRECTORY_ENTRY_IAT)
		{

		}
		//13、Delay import
		else if (i==IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT)
		{

		}
		//14、COM descriptor
		else if (i==IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR)
		{

		}
		//15、0
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
