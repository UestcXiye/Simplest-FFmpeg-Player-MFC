
// Simplest FFmpeg Player MFCDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"


// CSimplestFFmpegPlayerMFCDlg �Ի���
class CSimplestFFmpegPlayerMFCDlg : public CDialogEx
{
// ����
public:
	CSimplestFFmpegPlayerMFCDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SIMPLESTFFMPEGPLAYERMFC_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedAbout();
	CEdit m_url;
	afx_msg void OnBnClickedFiledialog();
	afx_msg void OnBnClickedPlay();
	afx_msg void OnBnClickedPause();
	afx_msg void OnBnClickedStop();
	afx_msg void OnBnClickedClose();
};
