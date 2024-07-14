#pragma once

//! Service for handling commandline arguments passed to foobar2000.exe
class NOVTABLE commandline_handler : public service_base
{
public:
	enum result
	{
		RESULT_NOT_OURS,//not our command
		RESULT_PROCESSED,//command processed
		RESULT_PROCESSED_EXPECT_FILES,//command processed, we want to takeover file urls after this command
	};
	virtual result on_token(const char * token)=0;
	virtual void on_file(const char * url) {};//optional
	virtual void on_files_done() {};//optional
	virtual bool want_directories() {return false;}

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(commandline_handler);
};

//! Helper automatically turning passed file locations into metadb_handle objects (audio track references)
class commandline_handler_metadb_handle : public commandline_handler {
protected:
	void on_file(const char * url) override final;
	bool want_directories() override {return true;}
public:
	//! Override me
	virtual result on_token(const char * token) override = 0;
	//! Override me
	virtual void on_files_done() override {};
	//! Override me
	virtual void on_file(const metadb_handle_ptr & ptr) = 0;
};

/*

how commandline_handler is used:

	scenario #1:
		creation => on_token() => deletion
	scenario #2:
		creation => on_token() returning RESULT_PROCESSED_EXPECT_FILES => on_file(), on_file().... => on_files_done() => deletion
*/

template<typename T>
class commandline_handler_factory_t : public service_factory_t<T> {};
