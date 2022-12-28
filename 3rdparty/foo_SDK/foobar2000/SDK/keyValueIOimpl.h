#pragma once
#include "keyValueIO.h"

#include <string>
#include <map>

namespace fb2k {
    class keyValueIOimpl : public keyValueIO {
    public:
        fb2k::stringRef get(const char * name) {
            auto i = m_content.find(name);
            if (i == m_content.end()) return fb2k::makeString("");
            return fb2k::makeString(i->second.c_str());
        }
        void put(const char * name, const char * value) {
            m_content[name] = value;
        }
        // OVERRIDE ME
        void commit() {}
        // OVERRIDE ME
        void reset() {}
        // OVERRIDE ME
        void dismiss(bool bOK) {}

    protected:
        std::map<std::string, std::string> m_content;
    };
}
