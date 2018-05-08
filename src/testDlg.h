
// testDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"


// CtestDlg dialog
class CtestDlg : public CDialog
{
// Construction
public:
	CtestDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_TEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CWnd plot;
    afx_msg void OnBnClickedButton1();
    CWnd plot_fft;
    CWnd plot_noised;
    afx_msg void OnBnClickedButton2();
    afx_msg void OnBnClickedButton3();
    CSliderCtrl x_scale_slider;
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnBnClickedButton4();
};
