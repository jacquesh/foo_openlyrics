#pragma once

#include <functional>

namespace fb2k {
    
	class NOVTABLE fileDialogNotify : public service_base {
		FB2K_MAKE_SERVICE_INTERFACE( fileDialogNotify, service_base );
	public:
        //! Called when user has cancelled the dialog.
		virtual void dialogCancelled() = 0;
        //! Called when the user has dismissed the dialog having selected some content.
        //! @param fsItems Array of fsItemBase objects or strings, depending on the platform. Should accept either form. Typically, file dialogs will handle fsItems but Add Location will handle path strings. Special case: playlist format chooser sends chosen format name as a string (one array item).
		virtual void dialogOK2( arrayRef items ) = 0;

		static fileDialogNotify::ptr create( std::function<void (arrayRef) > recv );
	};	

	class NOVTABLE fileDialogSetup : public service_base {
		FB2K_MAKE_SERVICE_INTERFACE( fileDialogSetup, service_base );
	public:
		
		virtual void setTitle( const char * title ) = 0;
		virtual void setAllowsMultiple(bool bValue) = 0;
		//! Sets allowed file types - in uGetOpenFileName format, eg. "Crash logs|*.txt"
		virtual void setFileTypes( const char * fileTypeStr ) = 0;
		virtual void setDefaultType( uint32_t indexInList ) = 0;
		//! Helper, calls setFileTypes() with a mask matching all known file types
		void setAudioFileTypes();
		//! Sets default extension, dot-less
		virtual void setDefaultExtension( const char * defaultExt ) = 0;
		virtual void setInitialDirectory( const char * initDirectory ) = 0;

		virtual void setInitialValue( const char * initValue ) = 0;

        virtual void setParent(fb2k::hwnd_t wndParent) = 0;


		enum {
			locNotSet = 0,
			locComputer,
			locDownloads,
			locMusic,
			locDocuments,
			locPictures,
			locVideos,
		};
		virtual void setInitialLocation(unsigned identifier) = 0;

		//! Runs the dialog. \n
		//! The dialog may run synchronously (block run() and the whole app UI) or asynchronously, depending on the platform. \n
		//! For an example, on Windows most filedialogs work synchronously while on OSX all of them work asynchronously.
		//! @param notify Notify object invoked upon dialog completion.
		virtual void run(fileDialogNotify::ptr notify) = 0;
	};

	class NOVTABLE fileDialog : public service_base {
		FB2K_MAKE_SERVICE_COREAPI( fileDialog );
	public:
		virtual fileDialogSetup::ptr setupOpen() = 0;
		virtual fileDialogSetup::ptr setupSave() = 0;
		virtual fileDialogSetup::ptr setupOpenFolder() = 0;
		virtual fileDialogSetup::ptr setupOpenURL() = 0;
		virtual fileDialogSetup::ptr setupChoosePlaylistFormat() = 0;
	};
};
