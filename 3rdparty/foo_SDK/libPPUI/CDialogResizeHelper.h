#pragma once

#include "CDialogResizeHelperCompat.h"
#include <vector>

class CDialogResizeHelper : public CMessageMap {
public:

	typedef CDialogResizeHelperCompat::param ParamOld;

	struct Param {
		uint32_t id;
		float snapLeft, snapTop, snapRight, snapBottom;
	};
private:
	void AddSizeGrip();
public:
	inline void set_min_size(unsigned x, unsigned y) { min_x = x; min_y = y; }
	inline void set_max_size(unsigned x, unsigned y) { max_x = x; max_y = y; }


	BEGIN_MSG_MAP_EX(CDialogResizeHelper)
		if (uMsg == WM_INITDIALOG) OnInitDialog(hWnd);
		MSG_WM_SIZE(OnSize)
		MSG_WM_DESTROY(OnDestroy)
		MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
	END_MSG_MAP()

	template<typename TParam, size_t paramCount> CDialogResizeHelper(const TParam(&src)[paramCount], CRect const& minMaxRange = CRect(0, 0, 0, 0)) {
		InitTable(src, paramCount);
		InitMinMax(minMaxRange);
	}

	void InitTable(const Param* table, size_t tableSize);
	void InitTable(const ParamOld * table, size_t tableSize);
	void InitMinMax(const CRect & range);

	bool EvalRect(UINT id, CRect & out) const;

	//! initData.id may be null, if so specify a non null window handle.
	void AddControl( Param const & initData, CWindow wnd = NULL );
	bool RemoveControl( CWindow wnd );
	bool HaveControl(CWindow wnd) const;
private:
	struct runtime_t {
		HWND userHWND = 0;
		CRect origRect;
		CSize origWindowSize;
		Param initData = {};
	};

	CSize CurrentSize() const;
	CWindow ResolveWnd( runtime_t const & rt ) const;

	CRect _EvalRect(runtime_t const & rt, CSize wndSize) const;
	void OnGetMinMaxInfo(LPMINMAXINFO lpMMI) const;
	void OnSize(UINT nType, CSize size);
	void OnInitDialog(CWindow thisWnd);
	void OnDestroy();


	std::vector<runtime_t> m_runtime;
	std::vector<Param> m_initData;

	CWindow m_thisWnd, m_sizeGrip;
	unsigned min_x, min_y, max_x, max_y;
};