#if !defined(AFX_MDCCHECKBOX_H__F2632DA0_E6ED_11D3_94CF_00C0F030989B__INCLUDED_)
#define AFX_MDCCHECKBOX_H__F2632DA0_E6ED_11D3_94CF_00C0F030989B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Machine generated IDispatch wrapper class(es) created by Microsoft Visual C++

// NOTE: Do not modify the contents of this file.  If this class is regenerated by
//  Microsoft Visual C++, your modifications will be overwritten.

/////////////////////////////////////////////////////////////////////////////
// CMdcCheckBox wrapper class

class CMdcCheckBox : public CWnd
{
protected:
	DECLARE_DYNCREATE(CMdcCheckBox)
public:
	CLSID const& GetClsid()
	{
		static CLSID const clsid
			= { 0x8bd21d40, 0xec42, 0x11ce, { 0x9e, 0xd, 0x0, 0xaa, 0x0, 0x60, 0x2, 0xf3 } };
		return clsid;
	}
	virtual BOOL Create(LPCTSTR lpszClassName,
		LPCTSTR lpszWindowName, DWORD dwStyle,
		const RECT& rect,
		CWnd* pParentWnd, UINT nID,
		CCreateContext* pContext = NULL)
	{ return CreateControl(GetClsid(), lpszWindowName, dwStyle, rect, pParentWnd, nID); }

    BOOL Create(LPCTSTR lpszWindowName, DWORD dwStyle,
		const RECT& rect, CWnd* pParentWnd, UINT nID,
		CFile* pPersist = NULL, BOOL bStorage = FALSE,
		BSTR bstrLicKey = NULL)
	{ return CreateControl(GetClsid(), lpszWindowName, dwStyle, rect, pParentWnd, nID,
		pPersist, bStorage, bstrLicKey); }

// Attributes
public:

// Operations
public:
	void SetAccelerator(LPCTSTR lpszNewValue);
	CString GetAccelerator();
	void SetAlignment(long nNewValue);
	long GetAlignment();
	void SetAutoSize(BOOL bNewValue);
	BOOL GetAutoSize();
	void SetBackColor(long nNewValue);
	long GetBackColor();
	void SetBackStyle(long nNewValue);
	long GetBackStyle();
	void SetCaption(LPCTSTR lpszNewValue);
	CString GetCaption();
	void SetEnabled(BOOL bNewValue);
	BOOL GetEnabled();
	void SetRefFont(LPDISPATCH newValue);
	LPDISPATCH GetFont();
	void SetForeColor(long nNewValue);
	long GetForeColor();
	void SetLocked(BOOL bNewValue);
	BOOL GetLocked();
	void SetMouseIcon(LPDISPATCH newValue);
	void SetRefMouseIcon(LPDISPATCH newValue);
	LPDISPATCH GetMouseIcon();
	void SetMousePointer(long nNewValue);
	long GetMousePointer();
	void SetPicture(LPDISPATCH newValue);
	void SetRefPicture(LPDISPATCH newValue);
	LPDISPATCH GetPicture();
	void SetPicturePosition(long nNewValue);
	long GetPicturePosition();
	void SetSpecialEffect(long nNewValue);
	long GetSpecialEffect();
	void SetTripleState(BOOL bNewValue);
	BOOL GetTripleState();
	void SetValue(VARIANT* newValue);
	VARIANT GetValue();
	void SetWordWrap(BOOL bNewValue);
	BOOL GetWordWrap();
	void SetGroupName(LPCTSTR lpszNewValue);
	CString GetGroupName();
	void SetTextAlign(long nNewValue);
	long GetTextAlign();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MDCCHECKBOX_H__F2632DA0_E6ED_11D3_94CF_00C0F030989B__INCLUDED_)
