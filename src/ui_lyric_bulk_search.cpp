#include "stdafx.h"

#pragma warning(push, 0)
#include "foobar2000/SDK/coreDarkMode.h"
#include "foobar2000/helpers/atl-misc.h"
#include "resource.h"
#pragma warning(pop)

#include "logging.h"
#include "lyric_io.h"
#include "lyric_metadata.h"
#include "metrics.h"
#include "tag_util.h"
#include "win32_util.h"
#include "metadb_index_search_avoidance.h"

class BulkLyricSearch;
static BulkLyricSearch* g_active_bulk_search_panel = nullptr;

class BulkLyricSearch : public CDialogImpl<BulkLyricSearch>
{
public:
    // Dialog resource ID
    enum
    {
        IDD = IDD_BULK_SEARCH
    };

    BulkLyricSearch(const std::vector<metadb_handle_ptr>& tracks_to_search);
    ~BulkLyricSearch() override;

    BEGIN_MSG_MAP(BulkLyricSearch)
    MSG_WM_INITDIALOG(OnInitDialog)
    MSG_WM_DESTROY(OnDestroyDialog)
    MSG_WM_CLOSE(OnClose)
    MSG_WM_TIMER(OnTimer)
    COMMAND_HANDLER_EX(IDC_BULKSEARCH_CLOSE, BN_CLICKED, OnCancel)
    END_MSG_MAP()

    void add_tracks(const std::vector<metadb_handle_ptr>& tracks_to_search);

private:
    struct TrackAndInfo
    {
        metadb_handle_ptr track;
        metadb_v2_rec_t track_info;
    };

    BOOL OnInitDialog(CWindow parent, LPARAM clientData);
    void OnDestroyDialog();
    void OnClose();
    LRESULT OnTimer(WPARAM);
    void OnCancel(UINT btn_id, int notify_code, CWindow btn);

    void update_status_text();
    void add_tracks_to_ui(const std::vector<TrackAndInfo>& new_tracks);

    std::optional<LyricSearchHandle> m_child_search;
    abort_callback_impl m_child_abort;

    std::vector<TrackAndInfo> m_tracks_to_search;
    int m_next_search_index;

    fb2k::CCoreDarkModeHooks m_dark;
};

static const UINT_PTR BULK_SEARCH_UPDATE_TIMER = 290110919;

BulkLyricSearch::BulkLyricSearch(const std::vector<metadb_handle_ptr>& tracks_to_search)
    : m_next_search_index(0)
{
    add_tracks(tracks_to_search);
}

BulkLyricSearch::~BulkLyricSearch() {}

BOOL BulkLyricSearch::OnInitDialog(CWindow /*parent*/, LPARAM /*clientData*/)
{
    LOG_INFO("Initializing bulk search window...");
    metrics::log_used_bulk_search();

    // TODO: We can't enable dark mode for this dialog because it adds items to a list
    //       after initialisation and that causes failures in the darkmode code, which doesn't
    //       fully support list UIs.
    // m_dark.AddDialogWithControls(m_hWnd);

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
    artist_column.cx = 144;
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

    add_tracks_to_ui(m_tracks_to_search);

    UINT_PTR result = SetTimer(BULK_SEARCH_UPDATE_TIMER, 0, nullptr);
    if(result != BULK_SEARCH_UPDATE_TIMER)
    {
        LOG_WARN("Unexpected timer result when starting bulk search update timer");
    }

    ShowWindow(SW_SHOW);
    return TRUE;
}

void BulkLyricSearch::OnDestroyDialog()
{
    core_api::ensure_main_thread();
    assert(g_active_bulk_search_panel == this);
    g_active_bulk_search_panel = nullptr;

    if(m_child_search.has_value())
    {
        bool completed = m_child_search.value().wait_for_complete(10'000);
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

void BulkLyricSearch::add_tracks(const std::vector<metadb_handle_ptr>& tracks_to_search)
{
    const size_t track_count = tracks_to_search.size();
    std::vector<TrackAndInfo> new_tracks;
    new_tracks.reserve(track_count);
    for(metadb_handle_ptr handle : tracks_to_search)
    {
        new_tracks.push_back(TrackAndInfo { handle, {} });
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
        const auto fill_track_info = [&new_tracks](size_t idx, const metadb_v2_rec_t& rec)
        { new_tracks[idx].track_info = rec; };
        meta->queryMultiParallel_(track_info_query_list, fill_track_info);
    }
    else
    {
        for(size_t i = 0; i < track_count; i++)
        {
            new_tracks[i].track_info = get_full_metadata(tracks_to_search[i]);
        }
    }

    assert(m_tracks_to_search.size() + new_tracks.size() <= INT_MAX);
    const bool had_finished = (m_next_search_index >= int(m_tracks_to_search.size()));
    m_tracks_to_search.insert(m_tracks_to_search.end(), new_tracks.begin(), new_tracks.end());
    if(m_hWnd != nullptr)
    {
        add_tracks_to_ui(new_tracks);

        // Only start the timer if we were still busy searching for a track.
        // This prevents users from skipping the 10 second extra wait time between remote
        // searches by adding tracks one-by-one.
        if(had_finished)
        {
            LVITEM subitem_status = {};
            subitem_status.mask = LVIF_TEXT;
            subitem_status.iItem = m_next_search_index;
            subitem_status.iSubItem = 2;
            subitem_status.pszText = _T("Searching...");
            LRESULT status_success = SendDlgItemMessageW(IDC_BULKSEARCH_LIST,
                                                         LVM_SETITEMTEXT,
                                                         m_next_search_index,
                                                         (LPARAM)&subitem_status);
            assert(status_success);

            UINT_PTR result = SetTimer(BULK_SEARCH_UPDATE_TIMER, 0, nullptr);
            if(result != BULK_SEARCH_UPDATE_TIMER)
            {
                LOG_WARN("Unexpected timer result when starting bulk search update timer");
            }
        }
    }
}

void BulkLyricSearch::add_tracks_to_ui(const std::vector<TrackAndInfo>& new_tracks)
{
    if(new_tracks.empty()) return;

    for(const TrackAndInfo& track : new_tracks)
    {
        std::tstring ui_title = to_tstring(track_metadata(track.track_info, "title"));
        std::tstring ui_artist = to_tstring(track_metadata(track.track_info, "artist"));

        LVITEM item = {};
        item.mask = LVIF_TEXT;
        item.iItem = (int)m_tracks_to_search
                         .size(); // As long as this is greater than the current length it'll go at the end
        item.pszText = const_cast<TCHAR*>(ui_title.c_str());
        LRESULT item_index = SendDlgItemMessageW(IDC_BULKSEARCH_LIST, LVM_INSERTITEM, 0, (LPARAM)&item);
        assert(item_index >= 0);
        assert(item_index <= INT_MAX);

        LVITEM subitem_artist = {};
        subitem_artist.mask = LVIF_TEXT;
        subitem_artist.iItem = int(item_index);
        subitem_artist.iSubItem = 1;
        subitem_artist.pszText = const_cast<TCHAR*>(ui_artist.c_str());
        LRESULT artist_success = SendDlgItemMessageW(IDC_BULKSEARCH_LIST,
                                                     LVM_SETITEMTEXT,
                                                     item_index,
                                                     (LPARAM)&subitem_artist);
        assert(artist_success);

        const TCHAR* status_string = ((item_index == 0) ? _T("Searching...") : _T(""));
        LVITEM subitem_status = {};
        subitem_status.mask = LVIF_TEXT;
        subitem_status.iItem = int(item_index);
        subitem_status.iSubItem = 2;
        subitem_status.pszText = const_cast<TCHAR*>(status_string);
        LRESULT status_success = SendDlgItemMessageW(IDC_BULKSEARCH_LIST,
                                                     LVM_SETITEMTEXT,
                                                     item_index,
                                                     (LPARAM)&subitem_status);
        assert(status_success);
    }

    update_status_text();
    SendDlgItemMessage(IDC_BULKSEARCH_PROGRESS, PBM_SETRANGE32, 0, m_tracks_to_search.size());
    SetDlgItemText(IDC_BULKSEARCH_CLOSE, _T("Cancel"));
}

void BulkLyricSearch::update_status_text()
{
    TCHAR buffer[64] = {};
    const size_t buffer_len = sizeof(buffer) / sizeof(buffer[0]);
    _sntprintf_s(buffer, buffer_len, _T("Searching %d/%zu"), m_next_search_index + 1, m_tracks_to_search.size());
    SetDlgItemText(IDC_BULKSEARCH_STATUS, buffer);
}

LRESULT BulkLyricSearch::OnTimer(WPARAM)
{
    if(!m_child_search.has_value())
    {
        assert((m_next_search_index >= 0) && (m_next_search_index < int(m_tracks_to_search.size())));

        const TrackAndInfo& track = m_tracks_to_search[m_next_search_index];
        m_child_search.emplace(LyricUpdate::Type::ManualSearch, track.track, track.track_info, m_child_abort);
        const SearchAvoidanceReason avoid_reason = false // todo change
                                                       ? SearchAvoidanceReason::Allowed
                                                       : search_avoidance_allows_search(track.track, track.track_info);
        const bool search_local_only = (avoid_reason != SearchAvoidanceReason::Allowed);
        if(search_local_only)
        {
            LOG_INFO("Search avoidance skipped remote sources for this track: %s",
                     search_avoid_reason_to_string(avoid_reason));
        }
        io::search_for_lyrics(m_child_search.value(), search_local_only);

        UINT_PTR result = SetTimer(BULK_SEARCH_UPDATE_TIMER, 16, nullptr);
        if(result != BULK_SEARCH_UPDATE_TIMER)
        {
            LOG_WARN("Unexpected timer result when starting bulk search update timer to check for results");
        }
        return 0;
    }

    assert(m_child_search.has_value());
    LyricSearchHandle& update = m_child_search.value();
    bool were_remote_sources_searched = update.has_searched_remote_sources();
    if(!update.is_complete())
    {
        return 0;
    }

    std::optional<LyricData> lyrics;
    if(update.has_result())
    {
        lyrics = io::process_available_lyric_update(
            { update.get_result(), update.get_track(), update.get_track_info(), update.get_type() });
    }
    m_child_search.reset();

    if(lyrics.has_value())
    {
        lyric_metadata_log_retrieved(update.get_track_info(), lyrics.value());
    }

    SendDlgItemMessage(IDC_BULKSEARCH_PROGRESS, PBM_STEPIT, 0, 0);
    if(m_next_search_index < int(m_tracks_to_search.size()))
    {
        const bool lyrics_found = lyrics.has_value() && !lyrics.value().IsEmpty();
        const TCHAR* status_text = lyrics_found ? _T("Found") : _T("Not found");
        LVITEM subitem_status = {};
        subitem_status.mask = LVIF_TEXT;
        subitem_status.iItem = m_next_search_index;
        subitem_status.iSubItem = 2;
        subitem_status.pszText = const_cast<TCHAR*>(status_text);
        LRESULT status_success = SendDlgItemMessageW(IDC_BULKSEARCH_LIST,
                                                     LVM_SETITEMTEXT,
                                                     m_next_search_index,
                                                     (LPARAM)&subitem_status);
        assert(status_success);

        m_next_search_index++;
    }

    if(m_next_search_index >= int(m_tracks_to_search.size()))
    {
        WIN32_OP(KillTimer(BULK_SEARCH_UPDATE_TIMER))
        SetDlgItemText(IDC_BULKSEARCH_STATUS, _T("Done"));
        SetDlgItemText(IDC_BULKSEARCH_CLOSE, _T("Close"));
    }
    else
    {
        // We mark the row as "Searching" here, rather than when we start the actual search so
        // that there is always one row that says it's searching, even during the extra delay
        // time between searches of online sources.
        // This is just to make it look better to the user, who might easily think it's broken
        // if it visibly just sits for several seconds between searches (which...admittedly it
        // does do, but it's by design, it's not a bug).
        LVITEM subitem_status = {};
        subitem_status.mask = LVIF_TEXT;
        subitem_status.iItem = m_next_search_index;
        subitem_status.iSubItem = 2;
        subitem_status.pszText = _T("Searching...");
        LRESULT status_success = SendDlgItemMessageW(IDC_BULKSEARCH_LIST,
                                                     LVM_SETITEMTEXT,
                                                     m_next_search_index,
                                                     (LPARAM)&subitem_status);
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

        assert(!m_child_search.has_value());
        UINT_PTR result = SetTimer(BULK_SEARCH_UPDATE_TIMER, sleep_ms, nullptr);
        if(result != BULK_SEARCH_UPDATE_TIMER)
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
        return nullptr;
    }

    core_api::ensure_main_thread();
    if(g_active_bulk_search_panel != nullptr)
    {
        g_active_bulk_search_panel->add_tracks(tracks_to_search);
        return g_active_bulk_search_panel->m_hWnd;
    }

    LOG_INFO("Spawning bulk search window...");
    HWND result = nullptr;
    try
    {
        auto new_window = new CWindowAutoLifetime<ImplementModelessTracking<BulkLyricSearch>>(
            core_api::get_main_window(),
            tracks_to_search);
        g_active_bulk_search_panel = new_window;
        result = new_window->m_hWnd;
    }
    catch(const std::exception& e)
    {
        popup_message::g_complain("Failed to create bulk search dialog", e);
    }
    return result;
}
