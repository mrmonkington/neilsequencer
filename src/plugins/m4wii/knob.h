// Machine generated IDispatch wrapper class(es) created with ClassWizard
/////////////////////////////////////////////////////////////////////////////
// CKnob wrapper class

class CKnob : public CDialog
{
public:
	CKnob() {}		// Calls COleDispatchDriver default constructor
	//CKnob(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	//CKnob(const CKnob& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:
	short GetValue();
	void SetValue(short);
	short GetMaxValue();
	void SetMaxValue(short);
	short GetMinValue();
	void SetMinValue(short);

// Operations
public:
	void AboutBox();
};
/////////////////////////////////////////////////////////////////////////////
// CKnobEvents wrapper class

class CKnobEvents : public COleDispatchDriver
{
public:
	CKnobEvents() {}		// Calls COleDispatchDriver default constructor
	//CKnobEvents(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	//CKnobEvents(const CKnobEvents& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	void ValueChanged(short val);
};
