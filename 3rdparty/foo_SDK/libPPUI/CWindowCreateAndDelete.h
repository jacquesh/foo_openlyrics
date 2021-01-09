#pragma once

#include "win32_op.h"

template<typename TClass>
class CWindowCreateAndDelete : public TClass {
public:
	template<typename ... arg_t> CWindowCreateAndDelete(HWND parent, arg_t && ... arg) : TClass(std::forward<arg_t>(arg) ...) { WIN32_OP(this->Create(parent) != NULL); }
private:
	void OnFinalMessage(HWND wnd) { PFC_ASSERT_NO_EXCEPTION(TClass::OnFinalMessage(wnd)); PFC_ASSERT_NO_EXCEPTION(delete this); }
};
