#pragma once

#include<afx.h>
#include<string>

using namespace std;
class Utils
{

public:
	static CString selectFile(LPCWSTR path) {

		TCHAR szFilter[] = _T("�����ļ�(*.*)|*.*||");
		
		CFileDialog dlg(TRUE,NULL,NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY);
		dlg.m_ofn.lpstrTitle = L"��ѡ���ļ�";
		dlg.m_ofn.lpstrInitialDir = path;

		if (dlg.DoModal()==IDOK)
		{
			CString fileName = dlg.GetPathName();

			printf("ѡ�����ļ�:%s", fileName.GetString());

			return fileName;
		}

		return CString("");
		
		/*
		// TODO: Add your control notification handler code here   
		// ���ù�����   
		TCHAR szFilter[] = _T("�ı��ļ�(*.txt)|*.txt|�����ļ�(*.*)|*.*||");
		// ������ļ��Ի���   
		CFileDialog fileDlg(TRUE, _T("txt"), NULL, 0, szFilter);
		CString strFilePath;

		// ��ʾ���ļ��Ի���   
		if (IDOK == fileDlg.DoModal())
		{
			// ���������ļ��Ի����ϵġ��򿪡���ť����ѡ����ļ�·����ʾ���༭����   
			strFilePath = fileDlg.GetPathName();
		
		}

		return strFilePath; 
		
		*/

		
		
	}
};

