#pragma once

namespace dsp_sample_common {
    static constexpr GUID guid = { 0x890827b, 0x67df, 0x4c27, { 0xba, 0x1a, 0x4f, 0x95, 0x8d, 0xf, 0xb5, 0xd0 } };

    static void make_preset(float gain, dsp_preset & out) {
        dsp_preset_builder builder; builder << gain; builder.finish(guid, out);
    }
    static float parse_preset(const dsp_preset & in) {
        try {
            float gain;
            dsp_preset_parser parser(in); parser >> gain;
            return gain;
        } catch(exception_io_data const &) {return 0;}
    }

}

using namespace dsp_sample_common;
