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


class BulkLyricSearch : public CDialogImpl<BulkLyricSearch>
{
public:
    // Dialog resource ID
    enum { IDD = IDD_BULK_SEARCH };

    BulkLyricSearch(std::vector<metadb_handle_ptr> tracks_to_search);
    ~BulkLyricSearch() override;

    BEGIN_MSG_MAP(BulkLyricSearch)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_DESTROY(OnDestroyDialog)
        MSG_WM_CLOSE(OnClose)
        MSG_WM_TIMER(OnTimer)
        COMMAND_HANDLER_EX(IDC_BULKSEARCH_CLOSE, BN_CLICKED, OnCancel)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow parent, LPARAM clientData);
    void OnDestroyDialog();
    void OnClose();
    LRESULT OnTimer(WPARAM);
    void OnCancel(UINT btn_id, int notify_code, CWindow btn);

    void update_status_text();

    std::optional<LyricUpdateHandle> m_child_update;
    abort_callback_impl m_child_abort;

    struct TrackAndInfo
    {
        metadb_handle_ptr track;
        metadb_v2_rec_t track_info;
    };
    std::vector<TrackAndInfo> m_tracks_to_search;
    size_t m_next_search_index;
};

static const UINT_PTR BULK_SEARCH_UPDATE_TIMER = 290110919;

BulkLyricSearch::BulkLyricSearch(std::vector<metadb_handle_ptr> tracks_to_search)
    : m_next_search_index(0)
{
    const size_t track_count = tracks_to_search.size();
    m_tracks_to_search.reserve(track_count);
    for(metadb_handle_ptr handle : tracks_to_search)
    {
        m_tracks_to_search.push_back(TrackAndInfo{handle, {}});
    }

    metadb_v2::ptr meta;
    if(metadb_v2::tryGet(meta)) // metadb_v2 is only available from fb2k v2.0 onwards
    {
        pfc::list_t<metadb_handle_ptr> track_info_query_list;
        track_info_query_list.prealloc(track_count);
        for(metadb_handle_ptr handle : tracks_to_search)
        {
            track_info_query_list.add_item(handle);
        }
        const auto fill_track_info = [this](size_t idx, const metadb_v2_rec_t& rec) { m_tracks_to_search[idx].track_info = rec; };
        meta->queryMultiParallel_(track_info_query_list, fill_track_info);
    }
    else
    {
        for(size_t i=0; i<track_count; i++)
        {
            m_tracks_to_search[i].track_info = get_full_metadata(tracks_to_search[i]);
        }
    }
}

BulkLyricSearch::~BulkLyricSearch()
{
}

BOOL BulkLyricSearch::OnInitDialog(CWindow /*parent*/, LPARAM /*clientData*/)
{
    LOG_INFO("Initializing bulk search window...");

    LVCOLUMN title_column = {};
    title_column.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    title_column.fmt = LVCFMT_LEFT;
    title_column.pszText = _T("Title");
    title_column.cx = 190;
    LRESULT title_index = SendDlgItemMessage(IDC_BULKSEARCH_LIST, LVM_INSERTCOLUMN, 0, (LPARAM)&title_column);
    assert(title_index >= 0);

    LVCOLUMN artist_column = {};
    artist_column.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    artist_column.fmt = LVCFMT_LEFT;
    artist_column.pszText = _T("Artist");
    artist_column.cx = 168;
    LRESULT artist_index = SendDlgItemMessage(IDC_BULKSEARCH_LIST, LVM_INSERTCOLUMN, 1, (LPARAM)&artist_column);
    assert(artist_index >= 0);

    LVCOLUMN status_column = {};
    status_column.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    status_column.fmt = LVCFMT_LEFT;
    status_column.pszText = _T("Status");
    status_column.cx = 80;
    LRESULT status_index = SendDlgItemMessage(IDC_BULKSEARCH_LIST, LVM_INSERTCOLUMN, 2, (LPARAM)&status_column);
    assert(status_index >= 0);

    SendDlgItemMessage(IDC_BULKSEARCH_LIST, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
    SendDlgItemMessage(IDC_BULKSEARCH_PROGRESS, PBM_SETSTEP, 1, 0);
    SendDlgItemMessage(IDC_BULKSEARCH_PROGRESS, PBM_SETRANGE32, 0, m_tracks_to_search.size());

    for(const TrackAndInfo& track : m_tracks_to_search)
    {
        std::tstring ui_title = to_tstring(track_metadata(track.track_info, "title"));
        std::tstring ui_artist = to_tstring(track_metadata(track.track_info, "artist"));

        LVITEM item = {};
        item.mask = LVIF_TEXT;
        item.iItem = (int)m_tracks_to_search.size(); // As long as this is greater than the current length it'll go at the end
        item.pszText = const_cast<TCHAR*>(ui_title.c_str());
        LRESULT item_index = SendDlgItemMessageW(IDC_BULKSEARCH_LIST, LVM_INSERTITEM, 0, (LPARAM)&item);
        assert(item_index >= 0);

        LVITEM subitem_artist = {};
        subitem_artist.mask = LVIF_TEXT;
        subitem_artist.iItem = item_index;
        subitem_artist.iSubItem = 1;
        subitem_artist.pszText = const_cast<TCHAR*>(ui_artist.c_str());
        LRESULT artist_success = SendDlgItemMessageW(IDC_BULKSEARCH_LIST, LVM_SETITEMTEXT, item_index, (LPARAM)&subitem_artist);
        assert(artist_success);

        const TCHAR* status_string = ((item_index == 0) ? _T("Searching...") : _T(""));
        LVITEM subitem_status = {};
        subitem_status.mask = LVIF_TEXT;
        subitem_status.iItem = item_index;
        subitem_status.iSubItem = 2;
        subitem_status.pszText = const_cast<TCHAR*>(status_string);
        LRESULT status_success = SendDlgItemMessageW(IDC_BULKSEARCH_LIST, LVM_SETITEMTEXT, item_index, (LPARAM)&subitem_status);
        assert(status_success);
    }

    UINT_PTR result = SetTimer(BULK_SEARCH_UPDATE_TIMER, 0, nullptr);
    if (result != BULK_SEARCH_UPDATE_TIMER)
    {
        LOG_WARN("Unexpected timer result when initially starting bulk search update timer");
    }

    update_status_text();

    ShowWindow(SW_SHOW);
    return TRUE;
}

void BulkLyricSearch::OnDestroyDialog()
{
    if(m_child_update.has_value())
    {
        bool completed = m_child_update.value().wait_for_complete(10'000);
        if(!completed)
        {
            LOG_WARN("Failed to complete custom lyric search before closing the window");
        }

        OnTimer(0); // Process the result, if we have one
    }

    KillTimer(BULK_SEARCH_UPDATE_TIMER);
}

void BulkLyricSearch::OnClose()
{
    m_child_abort.abort();
    DestroyWindow();
}

void BulkLyricSearch::OnCancel(UINT /*btn_id*/, int /*notification_type*/, CWindow /*btn*/)
{
    OnClose();
}

void BulkLyricSearch::update_status_text()
{
    TCHAR buffer[64] = {};
    const size_t buffer_len = sizeof(buffer)/sizeof(buffer[0]);
    _sntprintf_s(buffer, buffer_len, _T("Searching %zu/%zu"), m_next_search_index+1, m_tracks_to_search.size());
    SetDlgItemText(IDC_BULKSEARCH_STATUS, buffer);
}

LRESULT BulkLyricSearch::OnTimer(WPARAM)
{
    if(!m_child_update.has_value())
    {
        assert((m_next_search_index >= 0) && (m_next_search_index < m_tracks_to_search.size()));

        const TrackAndInfo& track = m_tracks_to_search[m_next_search_index];
        m_child_update.emplace(LyricUpdateHandle::Type::ManualSearch, track.track, track.track_info, m_child_abort);

        io::search_for_lyrics(m_child_update.value(), false);

        UINT_PTR result = SetTimer(BULK_SEARCH_UPDATE_TIMER, 16, nullptr);
        if (result != BULK_SEARCH_UPDATE_TIMER)
        {
            LOG_WARN("Unexpected timer result when starting bulk search update timer to check for results");
        }
        return 0;
    }

    assert(m_child_update.has_value());
    LyricUpdateHandle& update = m_child_update.value();
    bool were_remote_sources_searched = update.has_searched_remote_sources();
    if(!update.is_complete())
    {
        return 0;
    }

    std::optional<LyricData> lyrics = io::process_available_lyric_update(update);
    m_child_update.reset();

    SendDlgItemMessage(IDC_BULKSEARCH_PROGRESS, PBM_STEPIT, 0, 0);
    if(m_next_search_index < m_tracks_to_search.size())
    {
        const bool lyrics_found = lyrics.has_value() && !lyrics.value().IsEmpty();
        const TCHAR* status_text = lyrics_found ? _T("Found") : _T("Not found");
        LVITEM subitem_status = {};
        subitem_status.mask = LVIF_TEXT;
        subitem_status.iItem = m_next_search_index;
        subitem_status.iSubItem = 2;
        subitem_status.pszText = const_cast<TCHAR*>(status_text);
        LRESULT status_success = SendDlgItemMessageW(IDC_BULKSEARCH_LIST, LVM_SETITEMTEXT, m_next_search_index, (LPARAM)&subitem_status);
        assert(status_success);

        m_next_search_index++;
    }

    if(m_next_search_index >= m_tracks_to_search.size())
    {
        WIN32_OP(KillTimer(BULK_SEARCH_UPDATE_TIMER))
        SetDlgItemText(IDC_BULKSEARCH_STATUS, _T("Done"));
        SetDlgItemText(IDC_BULKSEARCH_CLOSE, _T("Close"));
    }
    else
    {
        LVITEM subitem_status = {};
        subitem_status.mask = LVIF_TEXT;
        subitem_status.iItem = m_next_search_index;
        subitem_status.iSubItem = 2;
        subitem_status.pszText = _T("Searching...");
        LRESULT status_success = SendDlgItemMessageW(IDC_BULKSEARCH_LIST, LVM_SETITEMTEXT, m_next_search_index, (LPARAM)&subitem_status);
        assert(status_success);

        update_status_text();

        // NOTE: We wait for 10 seconds between every search so as to not generate huge quantities of network
        //       traffic for the lyric servers in a very short time when we're searching for many tracks.
        //       When this timer expires, the callback will see that there is no child update active and
        //       will start a new search for the next track.
        DWORD sleep_ms = 10000;
        if(!were_remote_sources_searched)
        {
            // NOTE: If we found lyrics did not need to search a remote source in the process,
            //       then we need not add the extra delay since we aren't worried about flooding a website.
            sleep_ms = 1;
        }

        assert(!m_child_update.has_value());
        UINT_PTR result = SetTimer(BULK_SEARCH_UPDATE_TIMER, sleep_ms, nullptr);
        if (result != BULK_SEARCH_UPDATE_TIMER)
        {
            LOG_WARN("Unexpected timer result when starting bulk search update timer to check for results");
        }
    }

    return 0;
}

HWND SpawnBulkLyricSearch(std::vector<metadb_handle_ptr> tracks_to_search)
{
    if(tracks_to_search.empty())
    {
        return nullptr;;
    }

    LOG_INFO("Spawning bulk search window...");
    HWND result = nullptr;
    try
    {
        auto new_window = new CWindowAutoLifetime<ImplementModelessTracking<BulkLyricSearch>>(core_api::get_main_window(), tracks_to_search);
        result = new_window->m_hWnd;
    }
    catch(const std::exception& e)
    {
        popup_message::g_complain("Failed to create bulk search dialog", e);
    }
    return result;
}

