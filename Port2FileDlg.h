// Port2FileDlg.h : header file
//

#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#include "RedirectPort.h"
#include <vector>
#include "label.h"

// CRedirectDlg dialog
class CRedirectDlg : public CDialog
{
// Construction
public:
	CRedirectDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_PORT2FILE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

private:
	CListCtrl m_ListPorts;
	CImageList m_IconList; 
	CLabel m_HomeHlink;

	CString m_sErrorStatus;
	CString m_sLogFile;
	CString m_sRecvBytes;
	CString m_sTotalFileSize;

	CComboBox m_cbBaudrate;
	CComboBox m_cbDataBits;
	CComboBox m_cbParity;
	CComboBox m_cbStopBits;
	CComboBox m_cbFC;

	BOOL m_bFileAppend;
	int  m_iCurItemSel;   
	 
		    
	std::vector<CRedirectPort*> m_Ports;
	
	void GetAvailablePorts();
	void UpdateSettings   ();
	//void StartLogging     ( int iPort );
		
	LRESULT OnWriteUpdate ( WPARAM, LPARAM );
	LRESULT OnWriteStatus ( WPARAM, LPARAM );
	
// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	
	afx_msg void OnBnClickedBtView();
	afx_msg void OnBnClickedBtStartlog();
	afx_msg void OnBnClickedCheckLog();
	afx_msg void OnBnClickedBtHelp();

protected:
	virtual void OnOK();
	virtual void OnCancel();

protected:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	void OnCustomdrawList ( NMHDR* pNMHDR, LRESULT* pResult );
public:
	afx_msg void OnHdnGetdispinfoListPorts (NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemActivateListPorts(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedListPorts (NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangingListPorts(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCbnSelchangeCbBaudrate  ();
	afx_msg void OnCbnSelchangeCbDatabits  ();
	afx_msg void OnCbnSelchangeCbParity    ();
	afx_msg void OnCbnSelchangeCbStopbits  ();
	afx_msg void OnCbnSelchangeCbFc        ();
	afx_msg void OnBnClickedCheckAppend    ();
	afx_msg void OnEnChangeEdLogfile       ();
};
