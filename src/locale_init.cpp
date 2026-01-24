#include "stdafx.h"
#include <locale>

class LocaleInit : public initquit
{
public:
    void on_init()
    {
        std::setlocale(LC_ALL, "");
    }
};

static initquit_factory_t<LocaleInit> g_locale_init_factory;
