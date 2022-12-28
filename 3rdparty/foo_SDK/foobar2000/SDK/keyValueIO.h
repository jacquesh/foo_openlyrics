#pragma once

namespace fb2k {
    // Generic interface for writing frontends in high level programming languages
    class NOVTABLE keyValueIO : public service_base {
        FB2K_MAKE_SERVICE_INTERFACE(keyValueIO, service_base);
    public:
        virtual fb2k::stringRef get(const char * name) = 0;
        virtual void put(const char * name, const char * value) = 0;
        virtual void commit() = 0;
        virtual void reset() = 0;
        virtual void dismiss(bool bOK) = 0;
        
        int getInt( const char * name );
        void putInt( const char * name, int val );
    };
}

