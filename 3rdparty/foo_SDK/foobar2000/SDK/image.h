#pragma once
#include "imageLoaderLite.h"
#include "filesystem.h"
#include "tracks.h"

namespace fb2k {

    //! \since 2.0
    struct imageSize_t {
        double width, height;
        static imageSize_t empty() { imageSize_t s = {}; return s; }
        
        static bool equals(imageSize_t const & v1, imageSize_t const & v2);
        
        bool isValid() const;
        imageSize_t fitIn( double longerEdge ) const;
        imageSize_t fitIn( imageSize_t size ) const;
        bool operator==(const imageSize_t & other) const { return equals(*this, other); }
        bool operator!=(const imageSize_t & other) const { return !equals(*this, other); }
        
        void sanitize();

        //! Helper to allow imageSize_t objects to be used in various sorted contexts.
        static bool greater(imageSize_t const & v1, imageSize_t const & v2);
        bool operator>(const imageSize_t & other) const { return greater(*this, other); }
        bool operator<(const imageSize_t & other) const { return greater(other, *this); }
    };
	inline imageSize_t imageSizeMake(double w, double h) { imageSize_t s = { w, h }; return s; }

    //! \since 2.0
    class image : public service_base {
        FB2K_MAKE_SERVICE_INTERFACE( image, service_base );
    public:

        //! Source URL of this image. May be null if not known or available.
        virtual stringRef sourceURL() = 0;
        //! Source data of this image. May be null if not known or available.
        virtual memBlockRef sourceData() = 0;
        //! Resize the image to the specified size.
        virtual imageRef resize(imageSize_t toSize) = 0;

		//! Returns image size.
        virtual imageSize_t size() = 0;
        
		image::ptr resizeToFit( imageSize_t fitInSize );


		//! Saves as PNG.
        virtual void saveAsPNG( const char * pathSaveAt ) = 0;
		//! Saves as JPEG. Quality represented in 0..1 scale
        virtual void saveAsJPEG( const char * pathSaveAt, float jpegQuality) = 0;
        
        //! Returns if the image has alpha channel or not
        virtual bool hasAlpha() { return false; }
        
		virtual bool isFileBacked() { return false; }

		//! Returns platform-specific native data. The data remains owned by this image object.
		virtual nativeImage_t getNative() = 0;
		//! Detaches platform-specific native data from this image object. The caller becomes the owner of the native data and is responsible for its deletion.
		virtual nativeImage_t detachNative() = 0;

		static image::ptr empty() { return NULL; }
	};

	struct imageCreatorArg_t {
		imageSize_t inWantSize;
		imageSize_t outRealSize;

		static imageCreatorArg_t empty() {
			imageCreatorArg_t arg = {};
			return arg;
		}
	};

    //! \since 2.0
    //! Provides access to OS specific image object creation facilities. \n
	//! By convention, imageCreator methods return nullptr when loading the image fails, rather than throw exceptions.
    class imageCreator : public service_base {
        FB2K_MAKE_SERVICE_COREAPI( imageCreator );
	public:
		typedef imageCreatorArg_t arg_t;
		
		virtual imageRef loadImageNamed(const char * imgName, imageSize_t sizeHint = imageSize_t::empty()) = 0;
		virtual imageRef loadImageData(const void * data, size_t size, stringRef sourceURL = nullptr) = 0;
		virtual imageRef loadImageData(memBlockRef block, stringRef sourceURL = nullptr) = 0;
		virtual imageRef loadImageFromFile(const char * filePath, abort_callback & aborter) = 0;

        //! Opportunistic image loader helper. Returns immediately without doing any file access and an existing instance of an image object is ready for reuse. Returns null if no such object is available at this time.
        virtual imageRef tryReuseImageInstance( const char * filePath ) = 0;


		virtual imageRef loadImageNamed2( const char * imgName, arg_t & arg );
		virtual imageRef loadImageData2( const void * data, size_t size, stringRef sourceURL, arg_t & arg );
		virtual imageRef loadImageData2(memBlockRef block, stringRef sourceURL, arg_t & arg);
		virtual imageRef loadImageFromFile2(const char * filePath, abort_callback & aborter, arg_t & arg);
        virtual imageRef loadImageFromFile3( fsItemFile::ptr fsFile, abort_callback & aborter, arg_t & arg );
	};
    
    inline imageRef imageWithData( const void * data, size_t size ) {return imageCreator::get()->loadImageData( data, size ); }
    inline imageRef imageWithData( memBlockRef data ) {return imageCreator::get()->loadImageData(data); }
     

    struct imageLocation_t {

        bool isValid() const { return path.is_valid(); }
        bool isEmpty() const { return path.is_empty(); }
        void setPath( const char * path );
        void setPath( stringRef path );
        bool setTrack2( trackRef track );
        bool setTrack2( trackRef track, const GUID & albumArtID );
        void setStock( const char * name );
        void setEmbedded( const char * path, const GUID & albumArtID );

        //! Returns stock image name if this is a stock image reference, nullptr otherwise.
        stringRef getStock( ) const;
        
        static bool equals( const imageLocation_t & l1, const imageLocation_t & l2 );
            
        bool operator==( imageLocation_t const & other) const;
        bool operator!=( imageLocation_t const & other) const;

        stringRef path;
        t_filetimestamp knownTimeStamp = filetimestamp_invalid;// hint
    };

    //! \since 2.0
    //! Image loader service.
    class imageLoader : public service_base {
        FB2K_MAKE_SERVICE_COREAPI( imageLoader );
    public:
        
        struct arg_t {
            arg_t() { wantSize = imageSize_t::empty(); }
            arg_t( imageSize_t const & size ) : wantSize(size) {}
            imageSize_t wantSize;
            imageRef bigImageHint; // optional, provide if you have big non resized version available
        };
        
        static arg_t defArg() { arg_t r; return r; }
        
        virtual imageRef tryLoadFromCache( imageLocation_t const & loc, arg_t const & arg ) = 0;
        virtual imageRef loadSynchronously( imageLocation_t const & loc, arg_t const & arg, abort_callback & aborter ) = 0;
        virtual objRef beginLoad( imageLocation_t const & loc, arg_t const & arg, objReceiverRef receiver ) = 0;
        //! Retrieves image cache path for the specified location. The file at the returned path may or may not extist. \n
        //! Caller must provide valid URL and timestamp in imageLocation_t.
        virtual stringRef cacheLocation( imageLocation_t const & loc, arg_t const & arg) = 0;

        //! Similar to beginLoad; completes synchronously if the image is already in cache, passing the image to the receiver.
        objRef beginLoadEx( imageLocation_t const & loc, arg_t const & arg, objReceiverRef receiver );

        //! Helper; loads stock image synchronously.
        imageRef loadStock( const char * name, arg_t const & arg, abort_callback & aborter );
        
        //! Returns array of possible folder.jpg and alike file names to check for folder-pic. \n
        //! Provided to avoid hardcoding the list (of folder.jpg, cover.jpg, etc) everywhere. \n
        //! The returned strings are guaranteed lowercase.
        virtual arrayRef folderPicNames() = 0;
    };
}

pfc::string_base & operator<<(pfc::string_base & p_fmt,const fb2k::imageSize_t & imgSize);
