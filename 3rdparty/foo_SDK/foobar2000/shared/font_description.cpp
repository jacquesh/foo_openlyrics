#include "shared.h"


static unsigned query_dpi()
{
	unsigned ret;
	HDC dc = GetDC(0);
	ret = GetDeviceCaps(dc,LOGPIXELSY);
	ReleaseDC(0,dc);
	return ret;
}


#if 0
struct t_font_description
{
	enum {m_facename_length = LF_FACESIZE*2};

	t_uint32 m_height;
	t_uint32 m_weight;
	t_uint8 m_italic;
	t_uint8 m_charset;
	char m_facename[m_facename_length];
}
#endif

static void make_logfont(LOGFONT & p_logfont,const t_font_description & p_desc)
{
	p_logfont.lfHeight = - MulDiv(p_desc.m_height, query_dpi(), t_font_description::m_height_dpi);
	p_logfont.lfWidth = 0;
	p_logfont.lfEscapement = 0;
	p_logfont.lfOrientation = 0;
	p_logfont.lfWeight = p_desc.m_weight;
	p_logfont.lfItalic = p_desc.m_italic;
	p_logfont.lfUnderline = 0;
	p_logfont.lfStrikeOut = 0;
	p_logfont.lfCharSet = p_desc.m_charset;
	p_logfont.lfOutPrecision = 3;
	p_logfont.lfClipPrecision = 2;
	p_logfont.lfQuality = 1;
	p_logfont.lfPitchAndFamily = 34;
	pfc::stringToBuffer(p_logfont.lfFaceName,pfc::stringcvt::string_os_from_utf8(p_desc.m_facename,tabsize(p_desc.m_facename)));
}

static void make_description(t_font_description & p_desc,const LOGFONT & p_logfont)
{
	p_desc.m_height = MulDiv(pfc::abs_t(p_logfont.lfHeight), t_font_description::m_height_dpi, query_dpi());
	p_desc.m_weight = p_logfont.lfWeight;
	p_desc.m_italic = p_logfont.lfItalic;
	p_desc.m_charset = p_logfont.lfCharSet;
	pfc::stringToBuffer(p_desc.m_facename,pfc::stringcvt::string_utf8_from_os(p_logfont.lfFaceName,tabsize(p_logfont.lfFaceName)));
}

HFONT SHARED_EXPORT t_font_description::create() const
{
	LOGFONT temp;
	make_logfont(temp,*this);
	return CreateFontIndirect(&temp);
}

static UINT_PTR CALLBACK choose_font_hook(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			CHOOSEFONT * cf = reinterpret_cast<CHOOSEFONT*>(lp);
			reinterpret_cast<modal_dialog_scope*>(cf->lCustData)->initialize(FindOwningPopup(wnd));
		}
		return 0;
	default:
		return 0;
	}
}

bool SHARED_EXPORT t_font_description::popup_dialog(HWND p_parent)
{
	modal_dialog_scope scope;

	LOGFONT logfont;
	make_logfont(logfont,*this);

	CHOOSEFONT cf = {};
	cf.lStructSize = sizeof(cf);
	cf.hwndOwner = p_parent;
	cf.lpLogFont = &logfont;
	cf.Flags = CF_SCREENFONTS|CF_FORCEFONTEXIST|CF_INITTOLOGFONTSTRUCT|CF_ENABLEHOOK;
	cf.nFontType = SCREEN_FONTTYPE;
	cf.lCustData = reinterpret_cast<LPARAM>(&scope);
	cf.lpfnHook = choose_font_hook;
	if (ChooseFont(&cf))
	{
		make_description(*this,logfont);
		return true;
	}
	else return false;
}


void SHARED_EXPORT t_font_description::from_font(HFONT p_font)
{
	LOGFONT logfont;
	PFC_ASSERT_SUCCESS( GetObject((HGDIOBJ) p_font, sizeof(logfont), &logfont) != 0 );
	make_description(*this,logfont);
}

t_font_description SHARED_EXPORT t_font_description::g_from_font(HFONT p_font)
{
	t_font_description temp;
	temp.from_font(p_font);
	return temp;
}


t_font_description SHARED_EXPORT t_font_description::g_from_logfont(LOGFONT const & lf) {
	t_font_description ret; make_description(ret, lf); return ret;
}

t_font_description SHARED_EXPORT t_font_description::g_from_system(int id) {
	LOGFONT lf;
	if (FAILED( GetThemeSysFont(NULL, id, &lf) ) ) {
		PFC_ASSERT(!"Should not get here!");
		return g_from_font( (HFONT) GetStockObject(DEFAULT_GUI_FONT) );
	}
	return g_from_logfont(lf);
}
