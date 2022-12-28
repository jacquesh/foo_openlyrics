#include "foobar2000-sdk-pch.h"
#include "image.h"
#include "album_art.h"

namespace fb2k {
    bool imageSize_t::equals(imageSize_t const & v1, imageSize_t const & v2) {
        return v1.width == v2.width && v1.height == v2.height;
    }
    bool imageSize_t::greater(const fb2k::imageSize_t &v1, const fb2k::imageSize_t &v2) {
        // v1 > v2
        if (v1.width > v2.width) return true;
        if (v1.width < v2.width) return false;
        return v1.height > v2.height;
    }
    bool imageSize_t::isValid() const {
        return width > 0 && height > 0;
    }

    imageSize_t imageSize_t::fitIn( double longerEdge ) const {
        if (!isValid() || longerEdge <= 0) return empty();
        
        if (height <= longerEdge && width <= longerEdge) return *this;
        
        imageSize_t rv = {};
        if (width > height) {
            rv.width = longerEdge;
            rv.height = longerEdge * height / width;
        } else {
            rv.height = longerEdge;
            rv.width = longerEdge * width / height;
        }
        return rv;
    }
    imageSize_t imageSize_t::fitIn( imageSize_t size ) const {
        if (!isValid() || !size.isValid()) return empty();
        
        if (width <= size.width && height <= size.height) return *this;
        
        imageSize_t rv = {};
        double ratio = size.width / size.height;
        double ourRatio = width / height;
        if (ratio < ourRatio) {
            // fit width
            rv.width = size.width;
            rv.height = height * (size.width / width);
        } else {
            // fit height
            rv.height = size.height;
            rv.width = width * (size.height / height);
        }
        return rv;
    }
    
    void imageSize_t::sanitize() {
        if (width < 0) width = 0;
        if (height < 0) height = 0;
        width = pfc::rint32(width);
        height = pfc::rint32(height);
    }
    image::ptr image::resizeToFit( imageSize_t fitInSize ) {
        fitInSize.sanitize();
        imageSize_t ourSize = size();
        imageSize_t resizeTo = ourSize.fitIn(fitInSize);
        if (resizeTo == ourSize || !resizeTo.isValid()) return this;
        return resize( resizeTo );
    }



	static imageSize_t imageSizeSafe(imageRef img) {
		if (img.is_valid()) return img->size();
		else return imageSize_t::empty();
	}
	imageRef imageCreator::loadImageNamed2( const char * imgName, arg_t & arg ) {
		auto ret = loadImageNamed( imgName, arg.inWantSize );
		arg.outRealSize = imageSizeSafe(ret);
		return ret;
	}
	imageRef imageCreator::loadImageData2( const void * data, size_t size, stringRef sourceURL, arg_t & arg ) {
		auto ret = loadImageData(data, size, sourceURL);
		arg.outRealSize = imageSizeSafe(ret);
		return ret;
	}
	imageRef imageCreator::loadImageData2(memBlockRef block, stringRef sourceURL, arg_t & arg) {
		auto ret = loadImageData(block, sourceURL);
		arg.outRealSize = imageSizeSafe(ret);
		return ret;
	}
	imageRef imageCreator::loadImageFromFile2(const char * filePath, abort_callback & aborter, arg_t & arg) {
		auto ret = loadImageFromFile(filePath, aborter);
		arg.outRealSize = imageSizeSafe(ret);
		return ret;
	}
    imageRef imageCreator::loadImageFromFile3( fsItemFile::ptr fsFile, abort_callback & aborter, arg_t & arg ) {
        return loadImageFromFile2( fsFile->canonicalPath()->c_str(), aborter, arg);
    }

    
    void imageLocation_t::setPath( const char * path ) {
        this->path = makeString(path);
    }
    void imageLocation_t::setPath(stringRef path_) {
        this->path = path_;
    }
    bool imageLocation_t::setTrack2( trackRef track ) {
        return setTrack2( track, album_art_ids::cover_front );
    }
    bool imageLocation_t::setTrack2( trackRef track, const GUID & albumArtID ) {
        if (track.is_empty()) return false;
        setPath( pfc::format("trackimage://", pfc::print_guid(albumArtID), ",", track->get_path()));
        return true;
    }
    void imageLocation_t::setStock( const char * name ) {
        setPath( pfc::format( "stockimage://", name ) );
    }
    stringRef imageLocation_t::getStock( ) const {
        if (path.is_valid()) {
            if (matchProtocol( path->c_str(), "stockimage" ) ) {
                return makeString( strstr( path->c_str(), "://" ) + 3 );
            }
        }
        return nullptr;
    }

    void imageLocation_t::setEmbedded( const char * path, const GUID & albumArtID ) {
        setPath( PFC_string_formatter() << "embedded://" << pfc::print_guid(albumArtID) << "," << path);
    }
    bool imageLocation_t::equals(const fb2k::imageLocation_t &l1, const fb2k::imageLocation_t &l2) {
        if ( l1.path.is_empty() && l2.path.is_empty() ) return true;
        if ( l1.path.is_empty() || l2.path.is_empty() ) return false;
        return l1.path->equals( l2.path );
    }
    bool imageLocation_t::operator==( imageLocation_t const & other) const {
        return equals(*this, other);
    }
    bool imageLocation_t::operator!=( imageLocation_t const & other) const {
        return !equals(*this, other);
    }

    imageRef imageLoader::loadStock( const char * name, arg_t const & arg, abort_callback & aborter ) {
        imageLocation_t loc; loc.setStock( name );
        return loadSynchronously(loc, arg, aborter);
    }

}

pfc::string_base & operator<<(pfc::string_base & p_fmt,const fb2k::imageSize_t & imgSize) {
	auto iw = pfc::rint32(imgSize.width);
	auto ih = pfc::rint32(imgSize.height);
	if (iw == imgSize.width && ih == imgSize.height) {
        return p_fmt << "(" << iw << "x" << ih << ")";
	} else {
        return p_fmt << "(" << imgSize.width << "x" << imgSize.height << ")";
	}
}

