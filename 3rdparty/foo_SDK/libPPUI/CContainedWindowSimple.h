#pragma once

template<typename TBase> class CContainedWindowSimpleT : public CContainedWindowT<TBase>, public CMessageMap {
public:
	CContainedWindowSimpleT() : CContainedWindowT<TBase>(this) {}
	BEGIN_MSG_MAP(CContainedWindowSimpleT)
	END_MSG_MAP()
};

typedef CContainedWindowSimpleT<CWindow> CContainedWindowSimple;