#pragma once

#include<afx.h>
#include<string>

using namespace std;
class Utils
{

public:
	static CString selectFile(LPCWSTR path) {

		TCHAR szFilter[] = _T("所有文件(*.*)|*.*||");
		
		CFileDialog dlg(TRUE,NULL,NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY);
		dlg.m_ofn.lpstrTitle = L"请选择文件";
		dlg.m_ofn.lpstrInitialDir = path;

		if (dlg.DoModal()==IDOK)
		{
			CString fileName = dlg.GetPathName();

			printf("选择了文件:%s", fileName.GetString());

			return fileName;
		}

		return CString("");
		
		/*
		// TODO: Add your control notification handler code here   
		// 设置过滤器   
		TCHAR szFilter[] = _T("文本文件(*.txt)|*.txt|所有文件(*.*)|*.*||");
		// 构造打开文件对话框   
		CFileDialog fileDlg(TRUE, _T("txt"), NULL, 0, szFilter);
		CString strFilePath;

		// 显示打开文件对话框   
		if (IDOK == fileDlg.DoModal())
		{
			// 如果点击了文件对话框上的“打开”按钮，则将选择的文件路径显示到编辑框里   
			strFilePath = fileDlg.GetPathName();
		
		}

		return strFilePath; 
		
		*/

		
		
	}
};

