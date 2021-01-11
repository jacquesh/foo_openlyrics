#include "stdafx.h"

#include "localfiles.h"
#include "winstr_util.h"

// TODO: Consult IO.cpp and ui_and_threads.cpp for examples on doing async processing. Surely this is something we should be doing?

pfc::string8 sources::localfiles::GetLyricsDir()
{
    pfc::string8 lyricDirPath(core_api::get_profile_path());
    lyricDirPath.add_filename("lyrics");
    return lyricDirPath;
}

pfc::string8 sources::localfiles::Query(const pfc::string8& title, pfc::string8& out_lyrics)
{
    /* TODO: Make this async?
    fb2k::splitTask([shared_ptr_to_shared_data](){
            // Use the shared data
    });
    */
    abort_callback_dummy noAbort; // TODO: What should this be instead?

    pfc::string8 lyric_path_prefix = GetLyricsDir();
    lyric_path_prefix.add_filename(title);

    const char* extensions[] = { ".txt", ".lrc" };
    for (const char* ext : extensions)
    {
        pfc::string8 file_path = lyric_path_prefix;
        file_path.add_string(ext);

        if (filesystem::g_exists(file_path.c_str(), noAbort))
        {
            file_ptr file;
            filesystem::g_open_read(file, file_path.c_str(), noAbort);

            pfc::string8 file_contents;
            file->read_string_raw(file_contents, noAbort);

            out_lyrics = file_contents;
            return file_path;
        }
    }

    return pfc::string8();
}

void sources::localfiles::SaveLyrics(const pfc::string& title, LyricFormat format, const pfc::string8& lyrics)
{
    abort_callback_dummy noAbort; // TODO: What should this be instead?

    // TODO: Validate the filename to remove any bad chars. What are bad chars though?
    //       Maybe this list is a reasonable start:      |*?<>"\/:
    pfc::string8 output_path = GetLyricsDir();
    output_path.add_filename(title.c_str());
    switch(format)
    {
        case LyricFormat::Plaintext: output_path.add_string(".txt"); break;
        case LyricFormat::Timestamped: output_path.add_string(".lrc"); break;

        case LyricFormat::Unknown:
        default:
            console::printf("ERROR-OpenLyrics: Failed to compute output file path for title %s and format %d", title.c_str(), (int)format);
            return;
    }

    TCHAR temp_path_str[MAX_PATH+1];
    DWORD temp_path_str_len = GetTempPath(MAX_PATH+1, temp_path_str);
    pfc::string8 tmp_path = tchar_to_string(temp_path_str, temp_path_str_len);
    tmp_path.add_filename(title.c_str());
    console::printf("Saving lyrics to %s -> %s", tmp_path.c_str(), output_path.c_str());

    try
    {
        // TODO: NOTE: Scoping to close the file and flush writes to disk (hopefully preventing "file in use" errors)
        {
            file_ptr tmp_file;
            filesystem::g_open_write_new(tmp_file, tmp_path.c_str(), noAbort);
            tmp_file->write_string_raw(lyrics.c_str(), noAbort);
        }

        service_ptr_t<filesystem> fs = filesystem::get(output_path.c_str());
        if(fs->is_our_path(tmp_path.c_str()))
        {
            fs->move_overwrite(tmp_path.c_str(), output_path.c_str(), noAbort);
            console::printf("Successfully saved lyrics to %s", output_path.c_str());
        }
        else
        {
            console::printf("WARN-OpenLyrics: Cannot save lyrics file. Temp path (%s) and output path (%s) are on different filesystems", tmp_path.c_str(), output_path.c_str());
        }
    }
    catch(std::exception& e)
    {
        console::print(PFC_string_formatter() << "ERROR-OpenLyrics: Failed to write lyrics file to disk" << e);
    }
}
