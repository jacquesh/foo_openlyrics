#include "stdafx.h"
#include "wtl-pp.h"

void CEditPPHooks::DeleteLastWord( CEdit wnd, bool bForward ) {
	if ( wnd.GetWindowLong(GWL_STYLE) & ES_READONLY ) return;
	CString buffer;
	if ( wnd.GetWindowText(buffer) <= 0 ) return;
	const int len = buffer.GetLength();
	int selStart = len, selEnd = len;
	wnd.GetSel(selStart, selEnd);
	if ( selStart < 0 || selStart > len ) selStart = len; // sanity
	if ( selEnd < selStart ) selEnd = selStart; // sanity
	int work = selStart;
	if ( work == selEnd ) {
		// Only do our stuff if there is nothing yet selected. Otherwise first delete selection.
		if (bForward) {
			// go forward (ctrl+del)
			if (work < len && isSpecial(buffer[work])) { // linebreaks etc?
				do ++ work; while( work < len && isSpecial(buffer[work]));
			} else {
				// delete apparent spacing
				while ( work < len && isWordDelimiter(buffer[work])) ++ work;
				// delete apparent word
				while ( work < len && (!isWordDelimiter(buffer[work]) && !isSpecial(buffer[work]))) ++ work;
			}

			if ( selEnd < work ) {
				wnd.SetSel(selEnd, work, TRUE );
				wnd.ReplaceSel( TEXT(""), TRUE );
			}
		} else {
			// go backward (ctrl+backspace)
			if ( work > 0 && isSpecial(buffer[work-1])) { // linebreaks etc?
				do --work; while( work > 0 && isSpecial(buffer[work-1]));
			} else {
				// delete apparent spacing
				while( work > 0 && isWordDelimiter(buffer[work-1]) ) --work;
				// delete apparent word
				while( work > 0 && (!isWordDelimiter(buffer[work-1]) && !isSpecial(buffer[work-1]))) --work;
			}
			if ( selEnd > work ) {
				wnd.SetSel(work, selEnd, TRUE );
				wnd.ReplaceSel( TEXT(""), TRUE );
			}
		}
	}
}
