#include "stdafx.h"

#include "packet_decoder_aac_common.h"

#include "../SDK/filesystem_helper.h"
#include "bitreader_helper.h"

size_t packet_decoder_aac_common::skipADTSHeader( const uint8_t * data,size_t size ) {
    if ( size < 7 ) throw exception_io_data();
    PFC_ASSERT( bitreader_helper::extract_int(data, 0, 12) == 0xFFF);
    if (bitreader_helper::extract_bit(data, 12+1+2)) {
        return 7; // ABSENT flag
    }
    if (size < 9) throw exception_io_data();
    return 9;
}

pfc::array_t<uint8_t> packet_decoder_aac_common::parseDecoderSetup( const GUID & p_owner,t_size p_param1,const void * p_param2,t_size p_param2size) {

    if ( p_owner == owner_ADTS ) {
        pfc::array_t<uint8_t> ret;
        ret.resize( 2 );
        ret[0] = 0; ret[1] = 0;
        // ret:
        // 5 bits AOT
        // 4 bits freqindex
        // 4 bits channelconfig
        
        // source:
        // 12 bits 0xFFF
        // 4 bits disregard
        // 2 bits AOT-1         @ 16
        // 4 bits freqindex     @ 18
        // 1 bit disregard
        // 3 bits channelconfig @ 23
        // 26 bits total, 4 bytes minimum
        
        
        if ( p_param2size < 4 ) throw exception_io_data();
        const uint8_t * source = (const uint8_t*) p_param2;
        if ( bitreader_helper::extract_int(source, 0, 12) != 0xFFF ) throw exception_io_data();
        size_t aot = bitreader_helper::extract_int(source, 16, 2) + 1;
        if ( aot >= 31 ) throw exception_io_data();
        size_t freqindex = bitreader_helper::extract_int(source, 18, 4);
        if ( freqindex > 12 ) throw exception_io_data();
        size_t channelconfig = bitreader_helper::extract_bits( source, 23, 3);
        
        bitreader_helper::write_int(ret.get_ptr(), 0, 5, aot);
        bitreader_helper::write_int(ret.get_ptr(), 5, 4, freqindex);
        bitreader_helper::write_int(ret.get_ptr(), 9, 4, channelconfig);
        return ret;
    } else if ( p_owner == owner_ADIF ) {
        // bah
    } else if ( p_owner == owner_MP4 )
    {
        if ( p_param1 == 0x40 || p_param1 == 0x66 || p_param1 == 0x67 || p_param1 == 0x68 ) {
            pfc::array_t<uint8_t> ret;
            ret.set_data_fromptr( (const uint8_t*) p_param2, p_param2size);
            return ret;
        }
    }
    else if ( p_owner == owner_matroska )
    {
        const matroska_setup * setup = ( const matroska_setup * ) p_param2;
        if ( p_param2size == sizeof(*setup) )
        {
            if ( !strcmp(setup->codec_id, "A_AAC") || !strncmp(setup->codec_id, "A_AAC/", 6) ) {
                pfc::array_t<uint8_t> ret;
                ret.set_data_fromptr( (const uint8_t*) setup->codec_private, setup->codec_private_size );
                return ret;
            }
        }
    }
    throw exception_io_data();
}

#if 0
bool packet_decoder_aac_common::parseDecoderSetup(const GUID &p_owner, t_size p_param1, const void *p_param2, t_size p_param2size, const void *&outCodecPrivate, size_t &outCodecPrivateSize) {
    outCodecPrivate = NULL;
    outCodecPrivateSize = 0;
    
    if ( p_owner == owner_ADTS ) { return true; }
    else if ( p_owner == owner_ADIF ) { return true; }
    else if ( p_owner == owner_MP4 )
    {
        if ( p_param1 == 0x40 || p_param1 == 0x66 || p_param1 == 0x67 || p_param1 == 0x68 ) {
            outCodecPrivate = p_param2; outCodecPrivateSize = p_param2size;
            return true;
        }
    }
    else if ( p_owner == owner_matroska )
    {
        const matroska_setup * setup = ( const matroska_setup * ) p_param2;
        if ( p_param2size == sizeof(*setup) )
        {
            if ( !strcmp(setup->codec_id, "A_AAC") || !strncmp(setup->codec_id, "A_AAC/", 6) ) {
                outCodecPrivate = (const uint8_t *) setup->codec_private;
                outCodecPrivateSize = setup->codec_private_size;
                return true;
            }
        }
    }
    return false;
    
}
#endif

bool packet_decoder_aac_common::testDecoderSetup( const GUID & p_owner, t_size p_param1, const void * p_param2, t_size p_param2size ) {
    if ( p_owner == owner_ADTS ) { return true; }
    else if ( p_owner == owner_ADIF ) { return true; }
    else if ( p_owner == owner_MP4 )
    {
        if ( p_param1 == 0x40 || p_param1 == 0x66 || p_param1 == 0x67 || p_param1 == 0x68 ) {
            return true;
        }
    }
    else if ( p_owner == owner_matroska )
    {
        const matroska_setup * setup = ( const matroska_setup * ) p_param2;
        if ( p_param2size == sizeof(*setup) )
        {
            if ( !strcmp(setup->codec_id, "A_AAC") || !strncmp(setup->codec_id, "A_AAC/", 6) ) {
                return true;
            }
        }
    }
    return false;
}


namespace {
    class esds_maker : public stream_writer_buffer_simple {
    public:
        void write_esds_obj( uint8_t code, const void * data, size_t size, abort_callback & aborter ) {
            if ( size >= ( 1 << 28 ) ) throw pfc::exception_overflow();
            write_byte(code, aborter);
            for ( int i = 3; i >= 0; -- i ) {
                uint8_t c = (uint8_t)( 0x7F & ( size >> 7 * i ) );
                if ( i > 0 ) c |= 0x80;
                write_byte(c, aborter);
            }
            write( data, size, aborter );
        }
        void write_esds_obj( uint8_t code, esds_maker const & other, abort_callback & aborter ) {
            write_esds_obj( code, other.m_buffer.get_ptr(), other.m_buffer.get_size(), aborter );
        }
        void write_byte( uint8_t byte , abort_callback & aborter ) {
            write( &byte, 1, aborter );
        }
    };
}

void packet_decoder_aac_common::make_ESDS( pfc::array_t<uint8_t> & outESDS, const void * inCodecPrivate, size_t inCodecPrivateSize ) {
    if (inCodecPrivateSize > 1024*1024) throw exception_io_data(); // sanity
	auto & p_abort = fb2k::noAbort;

    esds_maker esds4;
    
    const uint8_t crap[] = {0x40, 0x15, 0x00, 0x00, 0x00, 0x00, 0x05, 0x34, 0x08, 0x00, 0x02, 0x3D, 0x55};
    esds4.write( crap, sizeof(crap), p_abort );
    
    {
        esds_maker esds5;
        esds5.write( inCodecPrivate, inCodecPrivateSize, p_abort );
        esds4.write_esds_obj(5, esds5, p_abort);
    }
    
    
    esds_maker esds3;
    
    esds3.write_byte( 0, p_abort );
    esds3.write_byte( 1, p_abort );
    esds3.write_byte( 0, p_abort );
    esds3.write_esds_obj(4, esds4, p_abort);
    
    // esds6 after esds4, but doesn't seem that important
    
    esds_maker final;
    final.write_esds_obj(3, esds3, p_abort);
    outESDS.set_data_fromptr( final.m_buffer.get_ptr(), final.m_buffer.get_size() );
    
    /*
     static const uint8_t esdsTemplate[] = {
     0x03, 0x80, 0x80, 0x80, 0x25, 0x00, 0x01, 0x00, 0x04, 0x80, 0x80, 0x80, 0x17, 0x40, 0x15, 0x00,
     0x00, 0x00, 0x00, 0x05, 0x34, 0x08, 0x00, 0x02, 0x3D, 0x55, 0x05, 0x80, 0x80, 0x80, 0x05, 0x12,
     0x30, 0x56, 0xE5, 0x00, 0x06, 0x80, 0x80, 0x80, 0x01, 0x02
     };
     */
    
    // ESDS: 03 80 80 80 25 00 01 00 04 80 80 80 17 40 15 00 00 00 00 05 34 08 00 02 3D 55 05 80 80 80 05 12 30 56 E5 00 06 80 80 80 01 02
    // For: 12 30 56 E5 00
    
}

const uint32_t aac_sample_rates[] = {
	96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350
};

static unsigned readSamplingFreq(bitreader_helper::bitreader_limited& r) {
    unsigned samplingRateIndex = (unsigned)r.read(4);
    if (samplingRateIndex == 15) {
        return (unsigned)r.read(24);
    } else {
        if (samplingRateIndex >= PFC_TABSIZE(aac_sample_rates)) throw exception_io_data();
        return aac_sample_rates[samplingRateIndex];
    }
}

packet_decoder_aac_common::audioSpecificConfig_t packet_decoder_aac_common::parseASC(const void * p_, size_t s) {
	// Source: https://wiki.multimedia.cx/index.php?title=MPEG-4_Audio
	bitreader_helper::bitreader_limited r((const uint8_t*)p_, 0, s * 8);

	audioSpecificConfig_t cfg = {};
	cfg.m_objectType = (unsigned) r.read(5);
	if (cfg.m_objectType == 31) {
		cfg.m_objectType = 32 + (unsigned) r.read(6);
	}

    cfg.m_sampleRate = readSamplingFreq(r);

    cfg.m_channels = (unsigned) r.read( 4 );

    if (cfg.m_objectType == 5 || cfg.m_objectType == 29) {
        cfg.m_explicitSBR = true;
        cfg.m_explicitPS = (cfg.m_objectType == 29);
        cfg.m_sbrRate = readSamplingFreq(r);
        cfg.m_objectType = (unsigned)r.read(5);
    }

    switch (cfg.m_objectType) {
    case 1: case 2: case 3: case 4: case 17: case 23:
        cfg.m_shortWindow = (r.read(1) != 0);
        break;
    }
        
	return cfg;
}

unsigned packet_decoder_aac_common::get_ASC_object_type(const void * p_, size_t s) {
	// Source: https://wiki.multimedia.cx/index.php?title=MPEG-4_Audio
	bitreader_helper::bitreader_limited r((const uint8_t*)p_, 0, s * 8);
	unsigned objectType = (unsigned) r.read(5);
	if (objectType == 31) {
		objectType = 32 + (unsigned) r.read(6);
	}
	return objectType;
}
