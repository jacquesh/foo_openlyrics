#include "stdafx.h"

#include "playback_tempo.h"
#include "logging.h"
#include "mvtf/mvtf.h"
#include "preferences.h"
#include "tag_util.h"
#include "ui_hooks.h"

// GUID for foo_dsp_effect "Tempo Shift": { 0x44bcaca2, 0x9edd, 0x493a, { 0xbb, 0x8f, 0x94, 0x74, 0xf4, 0xb5, 0xa7, 0x6b } }
static const GUID GUID_TEMPO_SHIFT = { 0x44bcaca2, 0x9edd, 0x493a, { 0xbb, 0x8f, 0x94, 0x74, 0xf4, 0xb5, 0xa7, 0x6b } };

// GUID for foo_dsp_effect "Playback Rate Shift": { 0x5d125ad0, 0x4745, 0x4d16, { 0xa4, 0xc7, 0x99, 0xd4, 0x23, 0x83, 0x30, 0xb9 } }
static const GUID GUID_RATE_SHIFT = { 0x5d125ad0, 0x4745, 0x4d16, { 0xa4, 0xc7, 0x99, 0xd4, 0x23, 0x83, 0x30, 0xb9 } };

double parse_tempo_dsp_preset(const uint8_t* data, size_t size)
{
    if(size < 4 || data == nullptr) return 1.0;

    const bool enabled = (size < 5) || (data[4] != 0);
    if(!enabled) return 1.0;

    float tempo_val = 0.0f;
    memcpy(&tempo_val, data, sizeof(float));
    if(std::isfinite(tempo_val) && tempo_val > -99.0f && tempo_val < 1000.0f)
    {
        return 1.0 + (double(tempo_val) / 100.0);
    }
    return 1.0;
}

double parse_rate_dsp_preset(const uint8_t* data, size_t size)
{
    if(size < 4 || data == nullptr) return 1.0;

    const bool enabled = (size < 5) || (data[4] != 0);
    if(!enabled) return 1.0;

    float rate_val = 0.0f;
    memcpy(&rate_val, data, sizeof(float));
    if(std::isfinite(rate_val) && rate_val > -99.0f && rate_val < 1000.0f)
    {
        return 1.0 + (double(rate_val) / 100.0);
    }
    return 1.0;
}

double get_playback_tempo_multiplier(const metadb_v2_rec_t& track_info)
{
    double tempo_mult = 1.0;
    double rate_mult = 1.0;

    service_ptr_t<dsp_config_manager> dsp_mgr = dsp_config_manager::tryGet();
    if(dsp_mgr.is_valid())
    {
        dsp_chain_config_impl cfg;
        dsp_mgr->get_core_settings(cfg);

        for(t_size n = 0; n < cfg.get_count(); n++)
        {
            const dsp_preset& entry = cfg.get_item(n);
            const GUID owner = entry.get_owner();
            if(owner == GUID_TEMPO_SHIFT)
            {
                tempo_mult *= parse_tempo_dsp_preset(reinterpret_cast<const uint8_t*>(entry.get_data()), entry.get_data_size());
            }
            else if(owner == GUID_RATE_SHIFT)
            {
                rate_mult *= parse_rate_dsp_preset(reinterpret_cast<const uint8_t*>(entry.get_data()), entry.get_data_size());
            }
        }
    }

    // Check metadata tags (foo_dsp_effect overrides preset with "tempo_amt" and "pbrate_amt" tags if present)
    if(track_info.info.is_valid())
    {
        const file_info& info = track_info.info->info();
        const char* tempo_tag = info.meta_get("tempo_amt", 0);
        if(tempo_tag != nullptr)
        {
            try
            {
                double tag_tempo = std::stod(tempo_tag);
                if(std::isfinite(tag_tempo) && tag_tempo > -99.0 && tag_tempo < 1000.0)
                {
                    tempo_mult = 1.0 + (tag_tempo / 100.0);
                }
            }
            catch(...) {}
        }

        const char* rate_tag = info.meta_get("pbrate_amt", 0);
        if(rate_tag != nullptr)
        {
            try
            {
                double tag_rate = std::stod(rate_tag);
                if(std::isfinite(tag_rate) && tag_rate > -99.0 && tag_rate < 1000.0)
                {
                    rate_mult = 1.0 + (tag_rate / 100.0);
                }
            }
            catch(...) {}
        }
    }

    double total = tempo_mult * rate_mult;
    if(!std::isfinite(total) || total <= 0.05)
    {
        return 1.0;
    }
    return total;
}

namespace
{
    class TempoDspConfigCallback : public dsp_config_callback
    {
    public:
        void on_core_settings_change(const dsp_chain_config& /*new_data*/) override
        {
            core_api::ensure_main_thread();
            notify_active_panels_tempo_changed();
        }
    };

    static service_factory_single_t<TempoDspConfigCallback> g_tempo_dsp_callback_factory;
}

#if MVTF_TESTS_ENABLED
MVTF_TEST(parse_tempo_dsp_preset_positive)
{
    float val = 25.0f;
    uint8_t data[5] = {};
    memcpy(data, &val, 4);
    data[4] = 1;
    double mult = parse_tempo_dsp_preset(data, 5);
    ASSERT(std::abs(mult - 1.25) < 0.0001);
}

MVTF_TEST(parse_tempo_dsp_preset_negative)
{
    float val = -20.0f;
    uint8_t data[5] = {};
    memcpy(data, &val, 4);
    data[4] = 1;
    double mult = parse_tempo_dsp_preset(data, 5);
    ASSERT(std::abs(mult - 0.80) < 0.0001);
}

MVTF_TEST(parse_tempo_dsp_preset_disabled)
{
    float val = 50.0f;
    uint8_t data[5] = {};
    memcpy(data, &val, 4);
    data[4] = 0; // disabled
    double mult = parse_tempo_dsp_preset(data, 5);
    ASSERT(mult == 1.0);
}
#endif
