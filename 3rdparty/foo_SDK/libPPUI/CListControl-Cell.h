#pragma once
#include <uxtheme.h> // HTHEME
#include <functional>

class CListCell {
public:
	typedef uint32_t cellState_t;
	enum {
		cellState_none = 0,
		cellState_hot = 1 << 0,
		cellState_pressed = 1 << 1,
		cellState_disabled = 1 << 2,
	};

	struct DrawContentArg_t {
		DWORD hdrFormat = 0;
		cellState_t cellState = 0;
		CRect subItemRect;
		CDCHandle dc;
		const wchar_t * text = nullptr;
		bool allowColors = true;
		CRect rcHot;
		CRect rcText;
		HTHEME theme = NULL;
		uint32_t colorHighlight = 0;
		CWindow thisWnd;
		std::function<void(CDCHandle, const CRect&) > imageRenderer;
		bool darkMode = false;
	};
	virtual void DrawContent( DrawContentArg_t const & arg ) = 0;
	virtual const char * Theme() { return nullptr; }
	virtual bool ApplyTextStyle( LOGFONT & font, double scale, uint32_t state );
	virtual bool IsInteractive() { return false; }
	virtual bool TracksMouseMove() { return false; }
	virtual bool SuppressRowSelect() { return false; }
	virtual bool IsToggle() { return false; }
	virtual bool IsRadioToggle() { return false; }
	virtual bool AllowTypeFind() { return true; }
	virtual CRect HotRect( CRect rc ) { return rc; }
	virtual HCURSOR HotCursor() { return NULL; }
	virtual bool AllowDrawThemeText() { return false; }
	virtual LONG AccRole();
	virtual uint32_t EditFlags() { return 0; }
	virtual bool ClickToEdit() { return false; }
};
