#pragma once

namespace fb2k {
    //! \since 2.0
    struct audioEncoderSetup_t {
        audio_chunk::spec_t spec;
        uint32_t bps;
        double durationHint; // Optional; if > 0 signals the encoder expected PCM stream length in advance to avoid rewriting headers
    };
    //! \since 2.0
    class audioEncoderInstance : public service_base {
        FB2K_MAKE_SERVICE_INTERFACE( audioEncoderInstance, service_base);
    public:
        virtual void addChunk( const audio_chunk & chunk, abort_callback & aborter ) = 0;
        virtual void finalize( abort_callback & aborter ) = 0;
    };
    
    //! \since 2.0
    class audioEncoder : public service_base {
        FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT( audioEncoder );
    public:
        virtual const char * formatExtension() = 0;
        virtual audioEncoderInstance::ptr open( file::ptr outFile, audioEncoderSetup_t const & spec, abort_callback & aborter ) = 0;
        
        static audioEncoderInstance::ptr g_open( const char * targetPath, file::ptr fileHint, audioEncoderSetup_t const & spec, abort_callback & aborter );
    };
}
