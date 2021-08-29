#include "stdafx.h"

#pragma warning(push, 0)
#include "resource.h"
#include "foobar2000/helpers/atl-misc.h"
#pragma warning(pop)

#include "logging.h"
#include "parsers.h"
#include "lyric_io.h"
#include "sources/lyric_source.h"
#include "win32_util.h"

class ManualLyricSearch : public CDialogImpl<ManualLyricSearch>
{
public:
    // Dialog resource ID
    enum { IDD = IDD_MANUAL_SEARCH };

    ManualLyricSearch(LyricUpdateHandle& update);
    ~ManualLyricSearch() override;

    BEGIN_MSG_MAP_EX(LyricEditor)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_DESTROY(OnDestroyDialog)
        MSG_WM_CLOSE(OnClose)
        MSG_WM_TIMER(OnTimer)
        MSG_WM_NOTIFY(OnNotify)
        COMMAND_HANDLER_EX(IDC_MANUALSEARCH_SEARCH, BN_CLICKED, OnSearchRequested)
        COMMAND_HANDLER_EX(IDC_MANUALSEARCH_CANCEL, BN_CLICKED, OnCancel)
        COMMAND_HANDLER_EX(IDC_MANUALSEARCH_OK, BN_CLICKED, OnOK)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow parent, LPARAM clientData);
    void OnDestroyDialog();
    void OnClose();
    LRESULT OnTimer(WPARAM);
    LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
    void OnCancel(UINT btn_id, int notify_code, CWindow btn);
    void OnOK(UINT btn_id, int notify_code, CWindow btn);
    void OnSearchRequested(UINT btn_id, int notify_code, CWindow btn);

    void start_search();

    LyricUpdateHandle& m_parent_update;
    std::optional<LyricUpdateHandle> m_child_update;
    abort_callback_impl m_child_abort;
    std::list<LyricData> m_all_lyrics;
};

static const UINT_PTR SEARCH_UPDATE_TIMER = 7917213;

ManualLyricSearch::ManualLyricSearch(LyricUpdateHandle& update) :
    m_parent_update(update)
{
}

ManualLyricSearch::~ManualLyricSearch()
{
}

BOOL ManualLyricSearch::OnInitDialog(CWindow /*parent*/, LPARAM /*clientData*/)
{
    LOG_INFO("Initializing manual search window...");

    LVCOLUMN title_column = {};
    title_column.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    title_column.fmt = LVCFMT_LEFT;
    title_column.pszText = _T("Title");
    title_column.cx = 160;
    LRESULT title_index = SendDlgItemMessage(IDC_MANUALSEARCH_RESULTLIST, LVM_INSERTCOLUMN, 0, (LPARAM)&title_column);
    assert(title_index >= 0);

    LVCOLUMN album_column = {};
    album_column.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    album_column.fmt = LVCFMT_LEFT;
    album_column.pszText = _T("Album");
    album_column.cx = 160;
    LRESULT album_index = SendDlgItemMessage(IDC_MANUALSEARCH_RESULTLIST, LVM_INSERTCOLUMN, 1, (LPARAM)&album_column);
    assert(album_index >= 0);

    LVCOLUMN artist_column = {};
    artist_column.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    artist_column.fmt = LVCFMT_LEFT;
    artist_column.pszText = _T("Artist");
    artist_column.cx = 128;
    LRESULT artist_index = SendDlgItemMessage(IDC_MANUALSEARCH_RESULTLIST, LVM_INSERTCOLUMN, 2, (LPARAM)&artist_column);
    assert(artist_index >= 0);

    LVCOLUMN source_column = {};
    source_column.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    source_column.fmt = LVCFMT_LEFT;
    source_column.pszText = _T("Source");
    source_column.cx = 96;
    LRESULT source_index = SendDlgItemMessage(IDC_MANUALSEARCH_RESULTLIST, LVM_INSERTCOLUMN, 3, (LPARAM)&source_column);
    assert(source_index >= 0);

    SetDlgItemText(IDC_MANUALSEARCH_PROGRESS, _T("Searching..."));
    SendDlgItemMessage(IDC_MANUALSEARCH_RESULTLIST, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

    m_parent_update.set_started();

    service_ptr_t<playback_control> playback = playback_control::get();
    metadb_handle_ptr now_playing = nullptr;
    bool playback_success = playback->get_now_playing(now_playing);
    if(playback_success)
    {
        std::string artist = track_metadata(now_playing, "artist");
        std::string album = track_metadata(now_playing, "album");
        std::string title = track_metadata(now_playing, "title");

        std::tstring artist_ui = to_tstring(artist);
        std::tstring album_ui = to_tstring(album);
        std::tstring title_ui = to_tstring(title);

        SetDlgItemText(IDC_MANUALSEARCH_ARTIST, artist_ui.c_str());
        SetDlgItemText(IDC_MANUALSEARCH_ALBUM, album_ui.c_str());
        SetDlgItemText(IDC_MANUALSEARCH_TITLE, title_ui.c_str());

        start_search();
    }
    else
    {
        LOG_WARN("Failed to get the now-playing track for manual lyric searching");
    }

    ShowWindow(SW_SHOW);
    return TRUE;
}

void ManualLyricSearch::OnDestroyDialog()
{
    if(m_child_update.has_value())
    {
        bool completed = m_child_update.value().wait_for_complete(10'000);
        if(!completed)
        {
            LOG_WARN("Failed to complete custom lyric search before closing the window");
        }
    }

    KillTimer(SEARCH_UPDATE_TIMER);
    if(!m_parent_update.is_complete())
    {
        m_parent_update.set_complete();
    }
}

void ManualLyricSearch::OnClose()
{
    m_child_abort.abort();
    DestroyWindow();
}

LRESULT ManualLyricSearch::OnNotify(int /*idCtrl*/, LPNMHDR notify)
{
    if((notify->idFrom != IDC_MANUALSEARCH_RESULTLIST) ||
        (notify->code != LVN_ITEMCHANGED))
    {
        SetMsgHandled(FALSE);
        return 0;
    }

    NMLISTVIEW* change = (NMLISTVIEW*)notify;
    assert(change != nullptr);

    bool now_selected = ((change->uNewState & LVIS_SELECTED) != 0);
    if(now_selected)
    {
        LyricData* item_lyrics = (LyricData*)change->lParam;
        assert(item_lyrics != nullptr);

        std::tstring lyrics_tstr = to_tstring(item_lyrics->text);
        SetDlgItemText(IDC_MANUALSEARCH_PREVIEW, lyrics_tstr.c_str());
    }
    else
    {
        SetDlgItemText(IDC_MANUALSEARCH_PREVIEW, _T(""));
    }

    return 0;
}

void ManualLyricSearch::start_search()
{
    service_ptr_t<playback_control> playback = playback_control::get();
    metadb_handle_ptr now_playing = nullptr;
    bool playback_success = playback->get_now_playing(now_playing);
    if(!playback_success)
    {
        LOG_WARN("Failed to get the now-playing track for manual lyric searching");
        return;
    }

    SetDlgItemText(IDC_MANUALSEARCH_PREVIEW, _T(""));
    SendDlgItemMessage(IDC_MANUALSEARCH_RESULTLIST, LVM_DELETEALLITEMS, 0, 0);
    m_all_lyrics.clear();

    assert(!m_child_update.has_value());
    try
    {
        m_child_update.emplace(m_parent_update.get_type(), now_playing, m_child_abort);
    }
    catch(const std::exception& e)
    {
        LOG_WARN("Failed to add custom search 'child' update, the update was probably cancelled", e.what());
        return;
    }

    TCHAR ui_artist[128] = {};
    TCHAR ui_album[128] = {};
    TCHAR ui_title[128] = {};
    UINT ui_artist_len = GetDlgItemText(IDC_MANUALSEARCH_ARTIST, ui_artist, sizeof(ui_artist)/sizeof(ui_artist[0]));
    UINT ui_album_len = GetDlgItemText(IDC_MANUALSEARCH_ALBUM, ui_album, sizeof(ui_album)/sizeof(ui_album[0]));
    UINT ui_title_len = GetDlgItemText(IDC_MANUALSEARCH_TITLE, ui_title, sizeof(ui_title)/sizeof(ui_title[0]));

    std::string artist = from_tstring(std::tstring_view{ui_artist, ui_artist_len});
    std::string album = from_tstring(std::tstring_view{ui_album, ui_album_len});
    std::string title = from_tstring(std::tstring_view{ui_title, ui_title_len});
    io::search_for_all_lyrics(m_child_update.value(), artist, album, title);

    GetDlgItem(IDC_MANUALSEARCH_SEARCH).EnableWindow(false);
    UINT_PTR result = SetTimer(SEARCH_UPDATE_TIMER, 16, nullptr);
    if (result != SEARCH_UPDATE_TIMER)
    {
        LOG_WARN("Unexpected timer result when starting search update timer");
    }

    SetDlgItemText(IDC_MANUALSEARCH_PROGRESS, _T("Searching..."));
}

void ManualLyricSearch::OnCancel(UINT /*btn_id*/, int /*notification_type*/, CWindow /*btn*/)
{
    m_child_abort.abort();
    DestroyWindow();
}

void ManualLyricSearch::OnOK(UINT /*btn_id*/, int /*notify_code*/, CWindow /*btn*/)
{
    assert(SendDlgItemMessage(IDC_MANUALSEARCH_RESULTLIST, LVM_GETSELECTEDCOUNT, 0, 0) <= 1);
    LRESULT list_selection = SendDlgItemMessage(IDC_MANUALSEARCH_RESULTLIST, LVM_GETSELECTIONMARK, 0, 0);
    if(list_selection == -1)
    {
        m_parent_update.set_complete();
    }
    else
    {
        assert((list_selection >= 0) && (list_selection <= INT_MAX));
        LVITEM selected = {};
        selected.mask = LVIF_PARAM;
        selected.iItem = (int)list_selection;
        LRESULT get_result = SendDlgItemMessage(IDC_MANUALSEARCH_RESULTLIST, LVM_GETITEM, 0, (LPARAM)&selected);
        if(get_result)
        {
            LyricData* selected_lyrics = (LyricData*)selected.lParam;
            assert(selected_lyrics != nullptr);

            m_parent_update.set_result(std::move(*selected_lyrics), true);
        }
        else
        {
            LOG_WARN("Failed to get selected search result list entry");
        }
    }

    m_child_abort.abort();
    DestroyWindow();
}

void ManualLyricSearch::OnSearchRequested(UINT btn_id, int notify_code, CWindow btn)
{
    start_search();
}

LRESULT ManualLyricSearch::OnTimer(WPARAM)
{
    if(!m_child_update.has_value())
    {
        GetDlgItem(IDC_MANUALSEARCH_SEARCH).EnableWindow(true);
        WIN32_OP(KillTimer(SEARCH_UPDATE_TIMER))
        return 0;
    }

    assert(m_child_update.has_value());
    LyricUpdateHandle& child_update = m_child_update.value();
    if(child_update.is_complete() && !child_update.has_result())
    {
        GetDlgItem(IDC_MANUALSEARCH_SEARCH).EnableWindow(true);
        m_child_update.reset();
        SetDlgItemText(IDC_MANUALSEARCH_PROGRESS, _T("Search complete"));
        WIN32_OP(KillTimer(SEARCH_UPDATE_TIMER))
        return 0;
    }

    while(child_update.has_result())
    {
        m_all_lyrics.push_back(child_update.get_result());
        const LyricData& lyrics = m_all_lyrics.back();

        LyricSourceBase* source = LyricSourceBase::get(lyrics.source_id);
        std::tstring source_name;
        if(source != nullptr)
        {
            source_name = std::tstring(source->friendly_name());
        }

        std::tstring ui_artist = to_tstring(lyrics.artist);
        std::tstring ui_album = to_tstring(lyrics.album);
        std::tstring ui_title = to_tstring(lyrics.title);

        LVITEM item = {};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = (int)m_all_lyrics.size(); // Technically the resulting index will be 1 less than this, but as long as this is greater than the current length it'll go at the end
        item.pszText = const_cast<TCHAR*>(ui_title.c_str());
        item.lParam = (LPARAM)&lyrics;
        LRESULT item_index = SendDlgItemMessageW(IDC_MANUALSEARCH_RESULTLIST, LVM_INSERTITEM, 0, (LPARAM)&item);
        assert(item_index >= 0);

        LVITEM subitem_album = {};
        subitem_album.mask = LVIF_TEXT;
        subitem_album.iItem = item_index;
        subitem_album.iSubItem = 1;
        subitem_album.pszText = const_cast<TCHAR*>(ui_album.c_str());
        LRESULT album_success = SendDlgItemMessageW(IDC_MANUALSEARCH_RESULTLIST, LVM_SETITEMTEXT, item_index, (LPARAM)&subitem_album);
        assert(album_success);

        LVITEM subitem_artist = {};
        subitem_artist.mask = LVIF_TEXT;
        subitem_artist.iItem = item_index;
        subitem_artist.iSubItem = 2;
        subitem_artist.pszText = const_cast<TCHAR*>(ui_artist.c_str());
        LRESULT artist_success = SendDlgItemMessageW(IDC_MANUALSEARCH_RESULTLIST, LVM_SETITEMTEXT, item_index, (LPARAM)&subitem_artist);
        assert(artist_success);

        LVITEM subitem_source = {};
        subitem_source.mask = LVIF_TEXT;
        subitem_source.iItem = item_index;
        subitem_source.iSubItem = 3;
        subitem_source.pszText = const_cast<TCHAR*>(source_name.c_str());
        LRESULT source_success = SendDlgItemMessageW(IDC_MANUALSEARCH_RESULTLIST, LVM_SETITEMTEXT, item_index, (LPARAM)&subitem_source);
        assert(source_success);

        bool is_first_entry = (m_all_lyrics.size() == 1);
        if(is_first_entry)
        {
            ListView_SetItemState(GetDlgItem(IDC_MANUALSEARCH_RESULTLIST), item_index, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
        }
    }
    return 0;
}

void SpawnManualLyricSearch(LyricUpdateHandle& update)
{
    LOG_INFO("Spawning manual search window...");
    try
    {
        new CWindowAutoLifetime<ImplementModelessTracking<ManualLyricSearch>>(core_api::get_main_window(), update);
    }
    catch(const std::exception& e)
    {
        popup_message::g_complain("Failed to create manual search dialog", e);
    }
}

