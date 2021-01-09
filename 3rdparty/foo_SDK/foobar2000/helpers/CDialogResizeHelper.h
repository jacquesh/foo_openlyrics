#pragma once

#include "WindowPositionUtils.h"

#include <libPPUI/CDialogResizeHelper.h>

template<typename TTracker> class CDialogResizeHelperTracking : public CDialogResizeHelper {
public:
	template<typename TParam, t_size paramCount, typename TCfgVar> CDialogResizeHelperTracking(const TParam (& src)[paramCount],CRect const& minMaxRange, TCfgVar & cfgVar) : CDialogResizeHelper(src, minMaxRange), m_tracker(cfgVar) {}
	
	BEGIN_MSG_MAP_EX(CDialogResizeHelperST)
		CHAIN_MSG_MAP(CDialogResizeHelper)
		CHAIN_MSG_MAP_MEMBER(m_tracker)
	END_MSG_MAP()

private:
	TTracker m_tracker;
};

typedef CDialogResizeHelperTracking<cfgDialogSizeTracker> CDialogResizeHelperST;
typedef CDialogResizeHelperTracking<cfgDialogPositionTracker> CDialogResizeHelperPT;
typedef CDialogResizeHelperTracking<cfgWindowSizeTracker2> CDialogResizeHelperST2;

#define REDRAW_DIALOG_ON_RESIZE() if (uMsg == WM_SIZE) RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
