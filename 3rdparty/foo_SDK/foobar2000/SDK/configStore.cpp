#include "foobar2000-sdk-pch.h"
#include "configStore.h"
#ifdef FOOBAR2000_MOBILE
#include "appearance.h"
#endif

using namespace fb2k;

namespace {
	class configStoreNotifyImpl : public configStoreNotify {
	public:
		void onVarChange() { f(); }
		std::function< void() > f;
	};
}

objRef configStore::addNotify( const char * name, std::function<void () > f ) {
	auto nameRef = makeString(name);
	ptr self = this;
	auto obj = new configStoreNotifyImpl;
	obj->f = f;
	this->addNotify(name, obj );
	return callOnRelease([self, nameRef, obj] {
		self->removeNotify( nameRef->c_str(), obj );
		delete obj;
	});
}
void configStore::addPermanentNotify( const char * name, std::function<void () > f ) {
	auto obj = new configStoreNotifyImpl;
	obj->f = f;
	this->addNotify(name, obj );
}

bool configStore::getConfigBool( const char * name, bool defVal ) {
    return this->getConfigInt( name, defVal ? 1 : 0) != 0;
}

bool configStore::toggleConfigBool (const char * name, bool defVal ) {
	bool newVal = ! getConfigBool(name, defVal);
    setConfigBool( name, newVal );
	return newVal;
}

void configStore::setConfigBool( const char * name, bool val ) {
    this->setConfigInt( name, val ? 1 : 0 );
}
void configStore::deleteConfigBool( const char * name ) {
    this->deleteConfigInt( name );
}

GUID configStore::getConfigGUID(const char* name, const GUID& defVal) {
    auto str = this->getConfigString(name, nullptr);
    if (str.is_empty()) return defVal;
    try {
        return pfc::GUID_from_text(str->c_str());
    } catch (...) {
        return defVal;
    }
}

void configStore::setConfigGUID(const char* name, const GUID& val) {
    this->setConfigString(name, pfc::print_guid(val));
}

void configStore::deleteConfigGUID(const char* name) {
    this->deleteConfigString(name);
}


configEventHandle_t configEvent::operator+=(std::function< void() > f) const {
	auto obj = new configStoreNotifyImpl;
	obj->f = f;
	try {
		static_api_ptr_t<configStore>()->addNotify(m_name, obj);
	} catch (...) {
		delete obj; throw;
	}
	return reinterpret_cast<configEventHandle_t>(obj);
}

void configEvent::operator-=(configEventHandle_t h) const {
	auto obj = reinterpret_cast<configStoreNotifyImpl*>(h);
	static_api_ptr_t<configStore>()->removeNotify(m_name, obj);
	delete obj;
}

configEventRef & configEventRef::operator<< ( const char * name ) { m_name = name; return *this; }

configEventRef & configEventRef::operator<< ( std::function<void () > f ) {
    setFunction(f);
    return *this;
}

configEventRef::~configEventRef() {
    try {
        clear();
    } catch(...) {
        // corner case: called from worker while app is shutting down, don't crash
    }
}

void configEventRef::clear() {
    if (m_handle != nullptr) {
        configEvent( m_name ) -= m_handle;
        m_handle = nullptr;
    }
}
void configEventRef::set( const char * name, std::function<void () > f ) {
    clear();
    m_name = name;
    m_handle = configEvent( m_name ) += f;
}

void configEventRef::setName( const char * name ) {
    clear();
    m_name = name;
}

void configEventRef::setFunction(std::function<void ()> f) {
    clear();
    PFC_ASSERT( m_name.length() > 0 );
    m_handle = configEvent( m_name ) += f;
}

#ifdef FOOBAR2000_MOBILE
namespace {
    class appearanceChangeNoitfyImpl : public appearanceChangeNoitfy {
    public:
        
        void onAppearanceChange() {
            f();
        }

        std::function<void () > f;
    };
}

appearance::notifyHandle_t appearance::addNotify2( std::function<void () > f) {
    auto ret = new appearanceChangeNoitfyImpl();
    ret->f = f;
    addNotify(ret);
    return reinterpret_cast<notifyHandle_t>(ret);
}
void appearance::removeNotify2( appearance::notifyHandle_t h ) {
    auto obj = reinterpret_cast<appearanceChangeNoitfyImpl*>(h);
    removeNotify(obj);
    delete obj;
}

void appearanceChangeNotifyRef::set( std::function<void()> f ) {
    clear();
    handle = static_api_ptr_t<appearance>()->addNotify2(f);
}
void appearanceChangeNotifyRef::clear() {
    if (isSet()) {
        static_api_ptr_t<appearance>()->removeNotify2(handle);
        handle = nullptr;
    }
}
#endif
