#include "stdafx.h"

#pragma warning(push, 0)
#include <ShObjIdl_core.h>
#include "resource.h"
#include "foobar2000/helpers/atl-misc.h"
#include "foobar2000/SDK/coreDarkMode.h"
#pragma warning(pop)

#include "config/config_auto.h"
#include "logging.h"
#include "preferences.h"
#include "tag_util.h"

static const GUID GUID_PREFERENCES_PAGE_SRC_LOCALFILES = { 0x9f2e83b6, 0xccc3, 0x4033, { 0xb5, 0x84, 0xf3, 0x84, 0x2d, 0xad, 0xfb, 0x40 } };

static const GUID GUID_CFG_SAVE_DIR_CLASS = { 0xcf49878d, 0xe2ea, 0x4682, { 0x98, 0xb, 0x8f, 0xc1, 0xf3, 0x80, 0x46, 0x7b } };
static const GUID GUID_CFG_SAVE_FILENAME_FORMAT = { 0x1f7a3804, 0x7147, 0x4b64, { 0x9d, 0x51, 0x4c, 0xdd, 0x90, 0xa7, 0x6d, 0xd6 } };
static const GUID GUID_CFG_SAVE_PATH_CUSTOM = { 0x84ac099b, 0xa00b, 0x4713, { 0x8f, 0x1c, 0x30, 0x7e, 0x31, 0xc0, 0xa1, 0xdf } };

static cfg_auto_combo_option<SaveDirectoryClass> save_dir_class_options[] =
{
    {_T("foobar2000 configuration directory"), SaveDirectoryClass::ConfigDirectory},
    {_T("Same directory as the track"), SaveDirectoryClass::TrackFileDirectory},
    {_T("Custom directory"), SaveDirectoryClass::Custom},
};

static cfg_auto_combo<SaveDirectoryClass, 3> cfg_save_dir_class(GUID_CFG_SAVE_DIR_CLASS, IDC_SAVE_DIRECTORY_CLASS, SaveDirectoryClass::ConfigDirectory, save_dir_class_options);

// TODO: The change below makes the default behaviour to swap our '/' with some other character so that we don't create directories
//       As per https://github.com/jacquesh/foo_openlyrics/issues/244
// TODO: Check what we replace other/invalid characters with, and then do the same here for consistency
// static cfg_auto_string                       cfg_save_filename_format(GUID_CFG_SAVE_FILENAME_FORMAT, IDC_SAVE_FILENAME_FORMAT, "[%artist% - ][%title%]");
static cfg_auto_string                       cfg_save_filename_format(GUID_CFG_SAVE_FILENAME_FORMAT, IDC_SAVE_FILENAME_FORMAT, "$replace([%artist% - ][%title%],/,-)");

static cfg_auto_string                       cfg_save_path_custom(GUID_CFG_SAVE_PATH_CUSTOM, IDC_SAVE_CUSTOM_PATH, "C:\\Lyrics\\%artist%");

static cfg_auto_property* g_root_auto_properties[] =
{
    &cfg_save_dir_class,
    &cfg_save_filename_format,
    &cfg_save_path_custom,
};

// Windows struggles to deal with directories whose names contain trailing spaces or dots.
// In both cases explorer fails to delete such a directory and in the trailing-dot case
// explorer won't even navigate into it.
// We therefore avoid these problems entirely by just removing trailing spaces and dots
// from all directory names in paths that we write to.
static bool trim_bad_trailing_directory_chars(pfc::string8& path)
{
    pfc::string parent = pfc::io::path::getParent(path);
    if(parent == "file://\\\\")
    {
        // If the parent path is "file://\\" then dir_path is something like "file:://\\machine_name" (a path on a network filesystem).
        // filesystem::g_exists fails (at least in SDK version 20200728 with fb2k version 1.6.7) on a path like that.
        // This is fine because we couldn't "create that directory" anyway, if it decided it didn't exist, so we'll just return here
        // as if it does and then either the existence checks further down the path tree will fail, or the actual file write will fail.
        // Both are guarded against IO failure, so that'd be fine (we'd just do slightly more work).
        return false;
    }

    bool parent_modified = false;
    if(!parent.isEmpty())
    {
        parent_modified = trim_bad_trailing_directory_chars(parent);
    }

    if(parent_modified || path.endsWith(' ') || path.endsWith('.'))
    {
        pfc::string filename = pfc::io::path::getFileName(path);
        while(!filename.is_empty()
            && ((filename[filename.length()-1] == ' ')
                || filename[filename.length()-1] == '.')
            )
        {
            filename.truncate(filename.length()-1);
        }

        path = parent;
        path.add_filename(filename);
        return true;
    }
    else
    {
        return parent_modified;
    }
}

std::string preferences::saving::filename(metadb_handle_ptr track, const metadb_v2_rec_t& track_info)
{
    const char* name_format_str = cfg_save_filename_format.c_str();
    titleformat_object::ptr name_format_script;
    bool name_compile_success = titleformat_compiler::get()->compile(name_format_script, name_format_str);
    if(!name_compile_success)
    {
        LOG_WARN("Failed to compile save file format: %s", name_format_str);
        return "";
    }

    pfc::string8 formatted_name;
    track->formatTitle_v2_(track_info, nullptr, formatted_name, name_format_script, nullptr);

    // We specifically replace illegal *path* chars here (rather than illegal *name* chars) to
    // allow users to put directory separators into their name format.
    // e.g users can set the save directory to "the config directory" and then have artist
    // subdirectories by specifying the name format to be '%artist%\%title%'
    formatted_name = pfc::io::path::replaceIllegalPathChars(formatted_name);

    std::string dir_class_name = "(Unknown)";
    pfc::string8 formatted_directory;
    SaveDirectoryClass dir_class = SaveDirectoryClass::DEPRECATED_None;
    if(track_is_remote(track))
    {
        dir_class = SaveDirectoryClass::ConfigDirectory;
    }
    else
    {
        dir_class = cfg_save_dir_class.get_value();
    }

    switch(dir_class)
    {
        case SaveDirectoryClass::ConfigDirectory:
        {
            dir_class_name = "ConfigDirectory";
            formatted_directory = core_api::get_profile_path();
            formatted_directory += "\\lyrics\\";
        } break;

        case SaveDirectoryClass::TrackFileDirectory:
        {
            dir_class_name = "TrackFileDirectory";
            const char* path = track->get_path();
            pfc::string tmp = path;
            tmp = pfc::io::path::getParent(tmp);
            formatted_directory = pfc::string8(tmp.c_str(), tmp.length());
        } break;

        case SaveDirectoryClass::Custom:
        {
            const char* path_format_str = cfg_save_path_custom.get_ptr();
            dir_class_name = std::string("Custom('") + path_format_str + "')";

            titleformat_object::ptr dir_format_script;
            bool dir_compile_success = titleformat_compiler::get()->compile(dir_format_script, path_format_str);
            if(!dir_compile_success)
            {
                LOG_WARN("Failed to compile save path format: %s", path_format_str);
                return "";
            }

            bool dir_format_success = track->format_title(nullptr, formatted_directory, dir_format_script, nullptr);
            if(!dir_format_success)
            {
                LOG_WARN("Failed to format save path using format: %s", path_format_str);
                return "";
            }
        } break;

        case SaveDirectoryClass::DEPRECATED_None:
        default:
            LOG_WARN("Unrecognised save path class: %d", (int)dir_class);
            return "";
    }

    pfc::string8 result = formatted_directory;
    result.add_filename(formatted_name);
    trim_bad_trailing_directory_chars(result);
    LOG_INFO("Save file name format '%s' with directory class '%s' evaluated to '%s'", name_format_str, dir_class_name.c_str(), result.c_str());

    if(formatted_directory.is_empty() || formatted_name.is_empty())
    {
        LOG_WARN("Invalid save path: %s", result.c_str());
        return "";
    }

    return std::string(result.c_str(), result.length());
}

SaveDirectoryClass preferences::saving::raw::directory_class()
{
    return cfg_save_dir_class.get_value();
}


class PreferencesSrcLocalfiles : public CDialogImpl<PreferencesSrcLocalfiles>, public auto_preferences_page_instance, private play_callback_impl_base
{
public:
    // Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
    PreferencesSrcLocalfiles(preferences_page_callback::ptr callback) : auto_preferences_page_instance(callback, g_root_auto_properties) {}

    // Dialog resource ID - Required by WTL/Create()
    enum {IDD = IDD_PREFERENCES_SRC_LOCALFILES};

    BEGIN_MSG_MAP_EX(PreferencesSrcLocalfiles)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_HANDLER_EX(IDC_SAVE_FILENAME_FORMAT, EN_CHANGE, OnSaveNameFormatChange)
        COMMAND_HANDLER_EX(IDC_SAVE_DIRECTORY_CLASS, CBN_SELCHANGE, OnDirectoryClassChange)
        COMMAND_HANDLER_EX(IDC_SAVE_CUSTOM_PATH, EN_CHANGE, OnCustomPathFormatChange)
        COMMAND_HANDLER_EX(IDC_SAVE_CUSTOM_PATH_BROWSE, BN_CLICKED, OnCustomPathBrowse)
        NOTIFY_HANDLER_EX(IDC_SAVE_SYNTAX_HELP, NM_CLICK, OnSyntaxHelpClicked)
    END_MSG_MAP()

private:
    void on_playback_new_track(metadb_handle_ptr track) override;

    BOOL OnInitDialog(CWindow, LPARAM);
    void OnUIChange(UINT, int, CWindow);
    void OnSaveNameFormatChange(UINT, int, CWindow);
    void OnDirectoryClassChange(UINT, int, CWindow);
    void OnCustomPathFormatChange(UINT, int, CWindow);
    void OnCustomPathBrowse(UINT, int, CWindow);
    LRESULT OnSyntaxHelpClicked(NMHDR*);

    void UpdateFormatPreview(int edit_id, int preview_id);
    void SetCustomPathEnabled();

    fb2k::CCoreDarkModeHooks m_dark;
};

void PreferencesSrcLocalfiles::on_playback_new_track(metadb_handle_ptr /*track*/)
{
    UpdateFormatPreview(IDC_SAVE_FILENAME_FORMAT, IDC_SAVE_FILE_NAME_PREVIEW);
    SetCustomPathEnabled();
}

BOOL PreferencesSrcLocalfiles::OnInitDialog(CWindow, LPARAM)
{
    m_dark.AddDialogWithControls(m_hWnd);

    init_auto_preferences();
    SetCustomPathEnabled();

    return FALSE;
}

void PreferencesSrcLocalfiles::OnUIChange(UINT, int, CWindow)
{
    on_ui_interaction();
}

void PreferencesSrcLocalfiles::OnSaveNameFormatChange(UINT, int, CWindow)
{
    UpdateFormatPreview(IDC_SAVE_FILENAME_FORMAT, IDC_SAVE_FILE_NAME_PREVIEW);
    on_ui_interaction();
}

void PreferencesSrcLocalfiles::OnDirectoryClassChange(UINT, int, CWindow)
{
    SetCustomPathEnabled();
    on_ui_interaction();
}

void PreferencesSrcLocalfiles::OnCustomPathFormatChange(UINT, int, CWindow)
{
    UpdateFormatPreview(IDC_SAVE_CUSTOM_PATH, IDC_SAVE_CUSTOM_PATH_PREVIEW);
    on_ui_interaction();
}

void PreferencesSrcLocalfiles::OnCustomPathBrowse(UINT, int, CWindow)
{
    std::wstring result;
    bool success = false;
    IFileDialog* dialog = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    if(SUCCEEDED(hr))
    {
        DWORD flags;
        hr = dialog->GetOptions(&flags);
        if(SUCCEEDED(hr))
        {
            hr = dialog->SetOptions(flags | FOS_FORCEFILESYSTEM | FOS_PICKFOLDERS);
            if(SUCCEEDED(hr))
            {
                hr = dialog->Show(nullptr);
                if(SUCCEEDED(hr))
                {
                    IShellItem* selected_item;
                    hr = dialog->GetResult(&selected_item);
                    if(SUCCEEDED(hr))
                    {
                        TCHAR* selected_path = nullptr;
                        hr = selected_item->GetDisplayName(SIGDN_FILESYSPATH, &selected_path);
                        if(SUCCEEDED(hr))
                        {
                            result = std::wstring(selected_path);
                            CoTaskMemFree(selected_path);
                            success = true;
                        }

                        selected_item->Release();
                    }
                }
                else if(hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
                {
                    success = true;
                }
            }
        }

        dialog->Release();
    }

    if(success)
    {
        if(!result.empty())
        {
            SetDlgItemText(IDC_SAVE_CUSTOM_PATH, result.c_str());
        }
    }
    else
    {
        LOG_INFO("Failure to get a path from the directory-select dialog");
    }
}

LRESULT PreferencesSrcLocalfiles::OnSyntaxHelpClicked(NMHDR* /*notify_msg*/)
{
    standard_commands::main_titleformat_help();
    return 0;
}

void PreferencesSrcLocalfiles::UpdateFormatPreview(int edit_id, int preview_id)
{
    CWindow preview_item = GetDlgItem(preview_id);
    assert(preview_item != nullptr);

    LRESULT format_text_length_result = SendDlgItemMessage(edit_id, WM_GETTEXTLENGTH, 0, 0);
    if(format_text_length_result > 0)
    {
        size_t format_text_length = (size_t)format_text_length_result;
        TCHAR* format_text_buffer = new TCHAR[format_text_length+1]; // +1 for null-terminator
        GetDlgItemText(edit_id, format_text_buffer, int(format_text_length+1));
        std::string format_text = from_tstring(std::tstring_view{format_text_buffer, format_text_length});
        delete[] format_text_buffer;

        titleformat_object::ptr format_script;
        bool compile_success = titleformat_compiler::get()->compile(format_script, format_text.c_str());
        if(compile_success)
        {
            metadb_handle_ptr preview_track = nullptr;

            service_ptr_t<playback_control> playback = playback_control::get();
            if(playback->get_now_playing(preview_track))
            {
                LOG_INFO("Playback is currently active, using the now-playing track for format preview");
            }
            else
            {
                pfc::list_t<metadb_handle_ptr> selection;

                service_ptr_t<playlist_manager> playlist = playlist_manager::get();
	            playlist->activeplaylist_get_selected_items(selection);

                if(selection.get_count() > 0)
                {
                    LOG_INFO("Using the first selected item for format preview");
                    preview_track = selection[0];
                }
                else if(playlist->activeplaylist_get_item_handle(preview_track, 0))
                {
                    LOG_INFO("No selection available, using the first playlist item for format preview");
                }
                else
                {
                    LOG_INFO("No selection available & no active playlist. There will be no format preview");
                }
            }

            if(preview_track != nullptr)
            {
                pfc::string8 formatted;
                bool format_success = preview_track->format_title(nullptr, formatted, format_script, nullptr);
                if(format_success)
                {
                    std::tstring preview = to_tstring(formatted);
                    preview_item.SetWindowText(preview.c_str());
                }
                else
                {
                    preview_item.SetWindowText(_T("<Unexpected formatting error>"));
                }
            }
            else
            {
                preview_item.SetWindowText(_T(""));
            }
        }
        else
        {
            preview_item.SetWindowText(_T("<Invalid format>"));
        }
    }
    else
    {
        preview_item.SetWindowText(_T(""));
    }
}

void PreferencesSrcLocalfiles::SetCustomPathEnabled()
{
    // NOTE: the auto-combo config sets item-data to the integral representation of that option's enum value
    LRESULT ui_index = SendDlgItemMessage(IDC_SAVE_DIRECTORY_CLASS, CB_GETCURSEL, 0, 0);
    LRESULT logical_value = SendDlgItemMessage(IDC_SAVE_DIRECTORY_CLASS, CB_GETITEMDATA, ui_index, 0);
    assert(logical_value != CB_ERR);
    SaveDirectoryClass dir_class = static_cast<SaveDirectoryClass>(logical_value);
    bool has_custom_path = (dir_class == SaveDirectoryClass::Custom);

    GetDlgItem(IDC_SAVE_CUSTOM_PATH).EnableWindow(has_custom_path);
    GetDlgItem(IDC_SAVE_CUSTOM_PATH_BROWSE).EnableWindow(has_custom_path);
    if(has_custom_path)
    {
        UpdateFormatPreview(IDC_SAVE_CUSTOM_PATH, IDC_SAVE_CUSTOM_PATH_PREVIEW);
    }
    else
    {
        GetDlgItem(IDC_SAVE_CUSTOM_PATH_PREVIEW).SetWindowText(_T(""));
    }
}


class PreferencesSrcLocalfilesImpl : public preferences_page_impl<PreferencesSrcLocalfiles>
{
public:
    const char* get_name() { return "Local files"; }
    GUID get_guid() { return GUID_PREFERENCES_PAGE_SRC_LOCALFILES; }
    GUID get_parent_guid() { return GUID_PREFERENCES_PAGE_SEARCH; }
};
static preferences_page_factory_t<PreferencesSrcLocalfilesImpl> g_preferences_page_factory;
