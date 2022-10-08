#pragma once
#include "../SDK/packet_decoder.h"

/*
Helper code with common AAC packet_decoder functionality. Primarily meant for foo_input_std-internal use.
*/

class packet_decoder_aac_common : public packet_decoder {
public:
    static pfc::array_t<uint8_t> parseDecoderSetup( const GUID & p_owner,t_size p_param1,const void * p_param2,t_size p_param2size);
    static bool testDecoderSetup( const GUID & p_owner, t_size p_param1, const void * p_param2, t_size p_param2size );
    static size_t skipADTSHeader( const uint8_t * data,size_t size );
    
	static unsigned get_max_frame_dependency_()
	{
		return 2;
	}
	static double get_max_frame_dependency_time_()
	{
		return 1024.0 / 8000.0;
	}
    
    static void make_ESDS( pfc::array_t<uint8_t> & outESDS, const void * inCodecPrivate, size_t inCodecPrivateSize );

	struct audioSpecificConfig_t {
		unsigned m_objectType;
		unsigned m_sampleRate;
        unsigned m_channels;
		unsigned m_sbrRate;
		bool m_shortWindow;
		bool m_explicitSBR, m_explicitPS;
	};

	static audioSpecificConfig_t parseASC(const void *, size_t);

	static unsigned get_ASC_object_type(const void *, size_t);
};
