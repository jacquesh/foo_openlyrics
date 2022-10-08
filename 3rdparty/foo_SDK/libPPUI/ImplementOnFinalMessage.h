#pragma once

template<typename class_t>
class ImplementOnFinalMessage : public class_t {
public:
	template<typename ... arg_t> ImplementOnFinalMessage(arg_t && ... arg) : class_t(std::forward<arg_t>(arg) ...) {}
	void OnFinalMessage(HWND wnd) override { PFC_ASSERT_NO_EXCEPTION(class_t::OnFinalMessage(wnd)); PFC_ASSERT_NO_EXCEPTION(delete this); }
};
