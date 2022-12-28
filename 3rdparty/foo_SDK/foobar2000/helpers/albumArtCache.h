#pragma once

#include <functional>
#include <SDK/titleformat.h>
#include <SDK/timer.h>

namespace fb2k {
    class albumArtCache {
    public:
        albumArtCache( imageSize_t s = imageSizeMake(0,0) ) : m_tfSplit(titleformat_patterns::patternAlbumSplit()), imageSize(s), timeOut(0) {
        }
		bool isLoading() const { return m_imageLoader.is_valid(); }
        bool isLoadingExpired() const { return timeOut > 0 && m_imageLoader.is_valid() && m_timeOutTimer.is_empty(); }
        bool isLoadingNonExpired() const { return isLoading() && !isLoadingExpired(); }
        pfc::string8 albumIDOfTrack( trackRef trk ) {
            pfc::string8 pattern;
            if (trk.is_valid()) {
                trk->format_title(nullptr, pattern, m_tfSplit, nullptr);
            }
            
            return pattern;
        }

        std::function<void () > onLoaded;
        imageSize_t imageSize;
        double timeOut;
        imageRef getImage( trackRef trk, std::function<void (imageRef)> asyncRecv = nullptr ) {
            auto key = albumIDOfTrack( trk );
            if (strcmp(key, m_imageKey) != 0) {
                m_timeOutTimer.release();
                m_imageKey = key;
                m_image.release();
                m_imageLoader.release();
                
                imageLocation_t loc;
                if (loc.setTrack2( trk ) ) {
                    auto recv = makeObjReceiver( [this, asyncRecv] (objRef obj) {
                        m_image ^= obj;
						if (m_imageLoader.is_valid()) {
							m_imageLoader.release();
                            m_timeOutTimer.release();
							if (onLoaded) onLoaded();
							if (asyncRecv) asyncRecv(m_image);
						}
                    } );
                    m_imageLoader = imageLoader::get()->beginLoad(loc, this->imageSize,  recv);
                    
                    if (m_imageLoader.is_valid() && timeOut > 0) {
                        m_timeOutTimer = fb2k::registerTimer( timeOut, [=] {
                            m_timeOutTimer.release();
                            if (onLoaded) onLoaded();
                            if (asyncRecv) asyncRecv(nullptr);
                        } );
                    }
                }
            }
            return m_image;
        }
        void getImage2( trackRef trk, std::function<void (imageRef)> recv ) {
            auto img = getImage( trk, recv );
            if (! this->isLoading() ) recv( img );
        }
        void reset() {
            m_imageKey = "";
            m_image.release();
            m_imageLoader.release();
            m_timeOutTimer.release();
        }
        imageRef current() const { return m_image; }
    private:
 
        titleformat_object_cache m_tfSplit;
        pfc::string8 m_imageKey;
        imageRef m_image;
        objRef m_imageLoader, m_timeOutTimer;
    };
}
