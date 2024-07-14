#include "shared.h"


static bool is_rect_null(const RECT * r)
{
	return r->right <= r->left || r->bottom <= r->top;
}

UINT SHARED_EXPORT uGetTextHeight(HDC dc)
{
	TEXTMETRIC tm;
	POINT pt[2];
	GetTextMetrics(dc,&tm);
	pt[0].x = 0;
	pt[0].y = tm.tmHeight;
	pt[1].x = 0;
	pt[1].y = 0;
	LPtoDP(dc,pt,2);

	int ret = pt[0].y - pt[1].y;
	return ret > 1 ? (unsigned)ret : 1;
}

static int get_text_width(HDC dc,const TCHAR * src,int len)
{
	if (len<=0) return 0;
	else
	{
		SIZE goatse;
		GetTextExtentPoint32(dc,src,len,&goatse);
		return goatse.cx;
	}
}

//GetTextExtentPoint32 wrapper, removes color marks
static int get_text_width_color(HDC dc,const TCHAR * src,int len)
{
	int ptr = 0;
	int start = 0;
	int rv = 0;
	if (len<0) len = (int) _tcslen(src);
	while(ptr<len)
	{
		if (src[ptr]==3)
		{
			rv += get_text_width(dc,src+start,ptr-start);
			ptr++;
			while(ptr<len && src[ptr]!=3) ptr++;
			if (ptr<len) ptr++;
			start = ptr;
		}
		else ptr++;
	}
	rv += get_text_width(dc,src+start,ptr-start);
	return rv;
}


static BOOL text_out_colors(HDC dc,const TCHAR * src,int len,int pos_x,int pos_y,const RECT * clip,bool selected,DWORD default_color)
{
	if (clip)
	{
		if (is_rect_null(clip) || clip->right<=pos_x || clip->bottom<=pos_y) return TRUE;
	}
	SetTextAlign(dc,TA_LEFT);
	SetBkMode(dc,TRANSPARENT);
	SetTextColor(dc,selected ? 0xFFFFFF - default_color : default_color);
	
	int title_ptr = 0;
	int textout_start = 0;
	int position = pos_x;//item.left+BORDER;
	
	for(;;)
	{
		if (title_ptr>=len || src[title_ptr]==3)
		{
			if (title_ptr>textout_start)
			{
				int width = get_text_width(dc,src+textout_start,title_ptr-textout_start);
				ExtTextOut(dc,position,pos_y,clip ? ETO_CLIPPED : 0,clip,src+textout_start,title_ptr-textout_start,0);
				position += width;
				textout_start = title_ptr;
			}
			if (title_ptr>=len) break;
		}
		if (src[title_ptr]==3)
		{
			DWORD new_color;
			DWORD new_inverted;
			bool have_inverted = false;

			if (src[title_ptr+1]==3) {new_color=default_color;title_ptr+=2;}
			else
			{
				title_ptr++;
				new_color = _tcstoul(src+title_ptr,0,16);
				while(title_ptr<len && src[title_ptr]!=3)
				{
					if (!have_inverted && src[title_ptr-1]=='|')
					{
						new_inverted = _tcstoul(src+title_ptr,0,16);
						have_inverted = true;
					}
					title_ptr++;
				}
				if (title_ptr<len) title_ptr++;
			}
			if (selected) new_color = have_inverted ? new_inverted : 0xFFFFFF - new_color;
			SetTextColor(dc,new_color);
			textout_start = title_ptr;
		}
		else
		{
			title_ptr = (int)( CharNext(src+title_ptr)-src );
		}
	}
	return TRUE;
}

static BOOL text_out_colors_tab(HDC dc,const TCHAR * display,int display_len,const RECT * item,int border,const RECT * base_clip,bool selected,DWORD default_color,bool columns)
{
	RECT clip;
	if (base_clip)
		IntersectRect(&clip,base_clip,item);
	else clip = *item;

	if (is_rect_null(&clip)) return TRUE;

	int pos_y = item->top + (item->bottom-item->top - (int)uGetTextHeight(dc)) / 2;
	
	int n_tabs = 0;
	int total_width = 0;
	{
		int start = 0;
		int n;
		for(n=0;n<display_len;n++)
		{
			if (display[n]=='\t')
			{
				if (start<n) total_width += get_text_width_color(dc,display+start,n-start) + 2*border;
				start = n+1;
				n_tabs++;
			}
		}
		if (start<display_len)
		{
			total_width += get_text_width_color(dc,display+start,display_len-start) + 2*border;
		}
	}
	
	int tab_total = item->right - item->left;
	if (!columns) tab_total -= total_width;
	int ptr = display_len;
	int tab_ptr = 0;
	int written = 0;
	int clip_x = item->right;
	do
	{
		int ptr_end = ptr;
		while(ptr>0 && display[ptr-1]!='\t') ptr--;
		const TCHAR * t_string = display + ptr;
		int t_length = ptr_end - ptr;
		if (t_length>0)
		{
			int t_width = get_text_width_color(dc,t_string,t_length) + border*2;

			int pos_x;
			int pos_x_right;
			
			if (!columns)
			{
				pos_x_right = item->right - MulDiv(tab_ptr,tab_total,n_tabs) - written;
			}
			else
			{
				if (tab_ptr==0) pos_x_right = item->right;
				else pos_x_right = item->right - MulDiv(tab_ptr,tab_total,n_tabs) + t_width;
			}

			if (ptr==0) 
			{
				pos_x = item->left;
			}
			else
			{			
				pos_x = pos_x_right - t_width ;
				if (pos_x<item->left) pos_x = item->left;
			}
			
			RECT t_clip = clip;

			if (t_clip.right > clip_x) t_clip.right = clip_x;

			text_out_colors(dc,t_string,t_length,pos_x+border,pos_y,&t_clip,selected,default_color);

			if (clip_x>pos_x) clip_x = pos_x;
			
			written += t_width;
		}
		
		if (ptr>0)
		{
			ptr--;//tab char
			tab_ptr++;
		}
	}
	while(ptr>0);
	
	return TRUE;
}

extern "C" {

BOOL SHARED_EXPORT uTextOutColors(HDC dc,const char * p_text,UINT len,int x,int y,const RECT * clip,BOOL is_selected,DWORD default_color)
{
	try {
		pfc::stringcvt::string_os_from_utf8 temp(p_text);
		return text_out_colors(dc,temp,pfc::downcast_guarded<int>(temp.length()),x,y,clip,!!is_selected,default_color);
	} catch(...) {return FALSE;}
}

BOOL SHARED_EXPORT uTextOutColorsTabbed(HDC dc,const char * p_text,UINT len,const RECT * item,int border,const RECT * clip,BOOL selected,DWORD default_color,BOOL use_columns)
{
	try {
		pfc::stringcvt::string_os_from_utf8 temp(p_text);
		return text_out_colors_tab(dc,temp,pfc::downcast_guarded<int>(temp.length()),item,border,clip,!!selected,default_color,!!use_columns);
	} catch(...) {return FALSE;}
}

}