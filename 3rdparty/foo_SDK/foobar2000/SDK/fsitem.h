#pragma once

// fsItem API
// Alternate, object-based way of accessing the local filesystem.
// It is recommended to use fsItem methods to operate on user-specified media folders.
// In some cases, most notably Android and UWP, accessing user's documents/media over paths requires expensive resolving of objects representing them.
// fsItem can cache platform specific resources necessary to manipulate the files, allowing efficient opening of files returned by directory enumeration.

#include "file.h"
#include "commonObjects.h"

namespace foobar2000_io {

	namespace createMode {
		enum {
			failIfExists = 0,
			allowExisting,
			generateUniqueName
		};
	}
	namespace listMode {
		enum {
			//! Return files
			files = 1,
			//! Return folders
			folders = 2,
			//! Return both files and flders
			filesAndFolders = files | folders,
			//! Return hidden files
			hidden = 4,
			//! Do not hand over filestats unless they come for free with folder enumeration
			suppressStats = 8,
		};
	}

	class fsItemBase; class fsItemFile; class fsItemFolder;
	typedef service_ptr_t<fsItemBase> fsItemPtr;
	typedef service_ptr_t<fsItemFile> fsItemFilePtr;
	typedef service_ptr_t<fsItemFolder> fsItemFolderPtr;
	class filesystem;

	//! Base class for filesystem items, files or folders. All fsItemBase objects actually belong to one of the main subclasses fsItemFile or fsItemFolder.
	class fsItemBase : public service_base {
		FB2K_MAKE_SERVICE_INTERFACE(fsItemBase, service_base);
	public:
		//! Returns possibly incomplete or outdated stats that are readily available without blocking. \n
		//! The stats may have been fetched at the the object is created (as a part of directory enumeration).
		virtual t_filestats2 getStatsOpportunist();

		//! Returns this item's canonical path.
		virtual fb2k::stringRef canonicalPath() = 0;

		//! Returns filename+extension / last path component
		virtual fb2k::stringRef nameWithExt() = 0;


		//! Shortened display name. By default same as nameWithExt().
		virtual fb2k::stringRef shortName();

		//! Copies this item to the specified folder. \n 
		//! If this is a folder, it will be copied recursively. \n
		//! This may be overridden for a specific filesystem to provide an optimized version; the default implementation walks and copies files using regular file APIs.
		virtual fsItemPtr copyTo(fsItemFolderPtr folder, const char* desiredName, unsigned createMode, abort_callback& aborter);
		//! See copyTo(4) above. \n
		//! Uses source item's name (last path component, filename+ext or foldername).
		fsItemPtr copyTo(fsItemFolderPtr folder, unsigned createMode, abort_callback& aborter);

		//! Moves this item to the specified folder. \n 
		//! If this is a folder, it will be moved recursively. \n
		//! This may be overridden for a specific filesystem to provide an optimized version; the default implementation walks and moves files using regular file APIs.
		virtual fsItemPtr moveTo(fsItemFolderPtr folder, const char* desiredName, unsigned createMode, abort_callback& aborter);
		//! See moveTo(4) above. \n
		//! Uses source item's name (last path component, filename+ext or foldername).
		fsItemPtr moveTo(fsItemFolderPtr folder, unsigned createMode, abort_callback& aborter);

		//! Removes this item.
		virtual void remove(abort_callback& aborter);

		//! Does represent remote object or local?
		virtual bool isRemote() = 0;

		virtual t_filestats2 getStats2(uint32_t s2flags, abort_callback& aborter) = 0;

		virtual service_ptr_t<filesystem> getFS() = 0;

		static fsItemBase::ptr fromPath(const char* path, abort_callback& aborter);
	};

	class fsItemFile : public fsItemBase {
		FB2K_MAKE_SERVICE_INTERFACE(fsItemFile, fsItemBase);
	public:
		virtual file::ptr open(uint32_t openMode, abort_callback& aborter) = 0;

		file::ptr openRead(abort_callback& aborter);
		file::ptr openWriteExisting(abort_callback& aborter);
		file::ptr openWriteNew(abort_callback& aborter);

		//! Transfer this file's content and properties to another file.
		virtual void copyToOther(fsItemFilePtr other, abort_callback& aborter);

		//! Returns the file's content type, if provided by the filesystem. \n
		//! Null if not provided by the filesystem.
		virtual fb2k::stringRef getContentType(abort_callback&) { return nullptr; }

		static fsItemFile::ptr fromPath(const char* path, abort_callback& aborter);

		virtual fb2k::memBlockRef readWhole(size_t sizeSanity, abort_callback& aborter);
	};

	class fsItemFolder : public fsItemBase {
		FB2K_MAKE_SERVICE_INTERFACE(fsItemFolder, fsItemBase);
	public:
		virtual fb2k::arrayRef listContents(unsigned listMode, abort_callback& aborter) = 0;
		virtual fsItemBase::ptr findChild(const char* fileName, abort_callback& aborter) = 0;
		virtual fsItemFile::ptr findChildFile(const char* fileName, abort_callback& aborter) = 0;
		virtual fsItemFolder::ptr findChildFolder(const char* fileName, abort_callback& aborter) = 0;
		virtual fsItemFile::ptr createFile(const char* fileName, unsigned createMode, abort_callback& aborter) = 0;
		virtual fsItemFolder::ptr createFolder(const char* fileName, unsigned createMode, abort_callback& aborter) = 0;

		//! Removes this folder recursively.
		virtual void removeRecur(abort_callback& aborter);

		static fsItemFolder::ptr fromPath(const char* path, abort_callback& aborter);
	};

}
