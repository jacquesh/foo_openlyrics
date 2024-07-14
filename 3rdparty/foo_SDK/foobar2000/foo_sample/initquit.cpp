#include "stdafx.h"

namespace {
    // Sample initquit implementation. See also: initquit class documentation in relevant header.
    class myinitquit : public initquit {
    public:
        void on_init() {
            console::print("Sample component: on_init()");
        }
        void on_quit() {
            console::print("Sample component: on_quit()");
        }
    };

    FB2K_SERVICE_FACTORY( myinitquit );

} // namespace
