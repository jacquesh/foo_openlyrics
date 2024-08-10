#include "stdafx.h"

#pragma warning(push, 0)
#include "resource.h"
#include "foobar2000/helpers/atl-misc.h"
#include "foobar2000/SDK/coreDarkMode.h"
#pragma warning(pop)

#include "logging.h"
#include "metrics.h"
#include "parsers.h"
#include "lyric_io.h"
#include "sources/lyric_source.h"
#include "tag_util.h"
#include "win32_util.h"


static const GUID GUID_CFG_TITLE_COLUMN_WIDTH = { 0x18d967fe, 0xad07, 0x464c, { 0x82, 0xf8, 0xbc, 0xeb, 0x1d, 0x67, 0xa5, 0x84 } };
static const GUID GUID_CFG_ALBUM_COLUMN_WIDTH = { 0x7ac61807, 0x57a2, 0x4880, { 0xba, 0x6d, 0xb2, 0x35, 0xf9, 0x88, 0x5a, 0x60 } };
static const GUID GUID_CFG_ARTIST_COLUMN_WIDTH = { 0xdebf2d4e, 0xfb93, 0x4a3f, { 0xb6, 0x3e, 0x84, 0x3f, 0x89, 0xa9, 0x86, 0xba } };
static const GUID GUID_CFG_SOURCE_COLUMN_WIDTH = { 0x44350ccb, 0xf62a, 0x4d47, { 0x9a, 0xeb, 0x5a, 0x7a, 0xc4, 0xce, 0x75, 0xa1 } };

// Without enforcing a minimum width, we could end up with columns being 0px wide
// (either as a result of user resizing or a bug that breaks the width persistence).
// That always looks like a bug, so instead we just enforce some arbitrary non-zero
// minimum column width.
constexpr int MIN_COLUMN_WIDTH_PX = 32;
static cfg_int_t<int> cfg_title_column_width(GUID_CFG_TITLE_COLUMN_WIDTH, 160);
static cfg_int_t<int> cfg_album_column_width(GUID_CFG_ALBUM_COLUMN_WIDTH, 160);
static cfg_int_t<int> cfg_artist_column_width(GUID_CFG_ARTIST_COLUMN_WIDTH, 128);
static cfg_int_t<int> cfg_source_column_width(GUID_CFG_SOURCE_COLUMN_WIDTH, 96);

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
        COMMAND_HANDLER_EX(IDC_MANUALSEARCH_APPLY, BN_CLICKED, OnApply)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow parent, LPARAM clientData);
    void OnDestroyDialog();
    void OnClose();
    LRESULT OnTimer(WPARAM);
    LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
    void OnCancel(UINT btn_id, int notify_code, CWindow btn);
    void OnOK(UINT btn_id, int notify_code, CWindow btn);
    void OnApply(UINT btn_id, int notify_code, CWindow btn);
    void OnSearchRequested(UINT btn_id, int notify_code, CWindow btn);

    void start_search();
    void save_selected_item();

    LyricUpdateHandle& m_parent_update;
    std::optional<LyricUpdateHandle> m_child_update;
    abort_callback_impl m_child_abort;
    std::list<LyricData> m_all_lyrics;

    int m_sort_column_index;
    bool m_sort_ascending;

    fb2k::CCoreDarkModeHooks m_dark;
};

static const UINT_PTR MANUAL_SEARCH_UPDATE_TIMER = 7917213;

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
    // TODO: We can't enable dark mode for this dialog because it adds items to a list
    //       after initialisation and that causes failures in the darkmode code, which doesn't
    //       fully support list UIs.
    //m_dark.AddDialogWithControls(m_hWnd);
    m_sort_column_index = -1;

    LVCOLUMN title_column = {};
    title_column.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    title_column.fmt = LVCFMT_LEFT;
    title_column.pszText = _T("Title");
    title_column.cx = max(MIN_COLUMN_WIDTH_PX, cfg_title_column_width);
    LRESULT title_index = SendDlgItemMessage(IDC_MANUALSEARCH_RESULTLIST, LVM_INSERTCOLUMN, 0, (LPARAM)&title_column);
    assert(title_index >= 0);

    LVCOLUMN album_column = {};
    album_column.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    album_column.fmt = LVCFMT_LEFT;
    album_column.pszText = _T("Album");
    album_column.cx = max(MIN_COLUMN_WIDTH_PX, cfg_album_column_width);
    LRESULT album_index = SendDlgItemMessage(IDC_MANUALSEARCH_RESULTLIST, LVM_INSERTCOLUMN, 1, (LPARAM)&album_column);
    assert(album_index >= 0);

    LVCOLUMN artist_column = {};
    artist_column.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    artist_column.fmt = LVCFMT_LEFT;
    artist_column.pszText = _T("Artist");
    artist_column.cx = max(MIN_COLUMN_WIDTH_PX, cfg_artist_column_width);
    LRESULT artist_index = SendDlgItemMessage(IDC_MANUALSEARCH_RESULTLIST, LVM_INSERTCOLUMN, 2, (LPARAM)&artist_column);
    assert(artist_index >= 0);

    LVCOLUMN source_column = {};
    source_column.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    source_column.fmt = LVCFMT_LEFT;
    source_column.pszText = _T("Source");
    source_column.cx = max(MIN_COLUMN_WIDTH_PX, cfg_source_column_width);
    LRESULT source_index = SendDlgItemMessage(IDC_MANUALSEARCH_RESULTLIST, LVM_INSERTCOLUMN, 3, (LPARAM)&source_column);
    assert(source_index >= 0);

    SetDlgItemText(IDC_MANUALSEARCH_PROGRESS, _T("Searching..."));
    SendDlgItemMessage(IDC_MANUALSEARCH_RESULTLIST, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

    m_parent_update.set_started();

    const metadb_v2_rec_t& initial_track_info = m_parent_update.get_track_info();
    std::string artist = track_metadata(initial_track_info, "artist");
    std::string album = track_metadata(initial_track_info, "album");
    std::string title = track_metadata(initial_track_info, "title");

    std::tstring artist_ui = to_tstring(artist);
    std::tstring album_ui = to_tstring(album);
    std::tstring title_ui = to_tstring(title);

    SetDlgItemText(IDC_MANUALSEARCH_ARTIST, artist_ui.c_str());
    SetDlgItemText(IDC_MANUALSEARCH_ALBUM, album_ui.c_str());
    SetDlgItemText(IDC_MANUALSEARCH_TITLE, title_ui.c_str());

    start_search();

    ShowWindow(SW_SHOW);
    return TRUE;
}

void ManualLyricSearch::OnDestroyDialog()
{
    TCHAR title_buffer[64] = {};
    LVCOLUMN column_data = {};
    column_data.mask = LVCF_TEXT | LVCF_WIDTH;
    column_data.pszText = title_buffer;
    column_data.cchTextMax = sizeof(title_buffer)/sizeof(title_buffer[0]);

    ListView_GetColumn(GetDlgItem(IDC_MANUALSEARCH_RESULTLIST), 0, &column_data);
    assert(std::tstring_view(column_data.pszText) == _T("Title"));
    cfg_title_column_width = column_data.cx;

    ListView_GetColumn(GetDlgItem(IDC_MANUALSEARCH_RESULTLIST), 1, &column_data);
    assert(std::tstring_view(column_data.pszText) == _T("Album"));
    cfg_album_column_width = column_data.cx;

    ListView_GetColumn(GetDlgItem(IDC_MANUALSEARCH_RESULTLIST), 2, &column_data);
    assert(std::tstring_view(column_data.pszText) == _T("Artist"));
    cfg_artist_column_width = column_data.cx;

    ListView_GetColumn(GetDlgItem(IDC_MANUALSEARCH_RESULTLIST), 3, &column_data);
    assert(std::tstring_view(column_data.pszText) == _T("Source"));
    cfg_source_column_width = column_data.cx;

    m_child_abort.abort();
    if(m_child_update.has_value())
    {
        bool completed = m_child_update.value().wait_for_complete(10'000);
        if(!completed)
        {
            LOG_WARN("Failed to complete custom lyric search before closing the window");
        }
    }

    KillTimer(MANUAL_SEARCH_UPDATE_TIMER);
    if(!m_parent_update.is_complete())
    {
        m_parent_update.set_complete();
    }
}

void ManualLyricSearch::OnClose()
{
    DestroyWindow();
}

static int CALLBACK column_sort_fn(LPARAM lparam1, LPARAM lparam2, LPARAM sort_data)
{
    LyricData* item1 = (LyricData*)lparam1;
    LyricData* item2 = (LyricData*)lparam2;
    if((item1 == nullptr) || (item2 == nullptr))
    {
        LOG_ERROR("Attempt to sort column with invalid item");
        return -1;
    }

    int sort_column_index = sort_data & 0xFF;
    bool sort_ascending = (((sort_data & 0x1FF) >> 8) != 0);
    int order_factor = sort_ascending ? 1 : -1;

    switch(sort_column_index)
    {
        case 0: return order_factor * item1->title.compare(item2->title);
        case 1: return order_factor * item1->album.compare(item2->album);
        case 2: return order_factor * item1->artist.compare(item2->artist);
        case 3:
        {
            LyricSourceBase* source1 = LyricSourceBase::get(item1->source_id);
            LyricSourceBase* source2 = LyricSourceBase::get(item2->source_id);
            if((source1 != nullptr) && (source2 != nullptr))
            {
                return order_factor * source1->friendly_name().compare(source2->friendly_name());
            }
            else
            {
                LOG_ERROR("No source available for search result");
                return -1;
            }
        } break;

        default:
            LOG_ERROR("Unexpected sort column index %d", sort_column_index);
            return 0;
    }
}

LRESULT ManualLyricSearch::OnNotify(int /*idCtrl*/, LPNMHDR notify)
{
    if(notify->idFrom != IDC_MANUALSEARCH_RESULTLIST)
    {
        SetMsgHandled(FALSE);
        return 0;
    }

    switch(notify->code)
    {
        case LVN_COLUMNCLICK:
        {
            NMLISTVIEW* list = (NMLISTVIEW*)notify;
            const int selected_column_index = list->iSubItem;
            HWND header = ListView_GetHeader(GetDlgItem(IDC_MANUALSEARCH_RESULTLIST));
            HDITEM header_item = {
                .mask = HDI_FORMAT
            };
            if(selected_column_index == m_sort_column_index)
            {
                m_sort_ascending = !m_sort_ascending;

                Header_GetItem(header, selected_column_index, &header_item);
                header_item.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
                header_item.fmt |= (m_sort_ascending ? HDF_SORTUP : HDF_SORTDOWN);
                Header_SetItem(header, selected_column_index, &header_item);
            }
            else
            {
                Header_GetItem(header, m_sort_column_index, &header_item);
                header_item.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
                Header_SetItem(header, m_sort_column_index, &header_item);

                m_sort_column_index = selected_column_index;
                m_sort_ascending = true;

                Header_GetItem(header, selected_column_index, &header_item);
                header_item.fmt |= HDF_SORTUP;
                Header_SetItem(header, selected_column_index, &header_item);
            }
            int sort_data = (m_sort_column_index & 0xFF) | ((m_sort_ascending ? 1 : 0) << 8);
            SendDlgItemMessage(IDC_MANUALSEARCH_RESULTLIST, LVM_SORTITEMS, sort_data, (LPARAM)column_sort_fn);
        } break;

        case LVN_ITEMCHANGED:
        {
            NMLISTVIEW* change = (NMLISTVIEW*)notify;
            assert(change != nullptr);

            bool now_selected = ((change->uNewState & LVIS_SELECTED) != 0);
            if(now_selected)
            {
                LyricData* item_lyrics = (LyricData*)change->lParam;
                assert(item_lyrics != nullptr);
                if(item_lyrics != nullptr)
                {
                    std::tstring lyrics_tstr = parsers::lrc::expand_text(*item_lyrics, false);
                    SetDlgItemText(IDC_MANUALSEARCH_PREVIEW, lyrics_tstr.c_str());
                }
            }
            else
            {
                SetDlgItemText(IDC_MANUALSEARCH_PREVIEW, _T(""));
            }
        } break;

        case NM_DBLCLK:
        {
            // The currently-selected item has already been selected (on the first click)
            // so this will just apply it as the "accepted" option
            OnOK(0, 0, nullptr);
        } break;

        default:
        {
            SetMsgHandled(FALSE);
            return 0;
        } break;
    }

    return 0;
}

void ManualLyricSearch::start_search()
{
    metrics::log_used_manual_search();

    SetDlgItemText(IDC_MANUALSEARCH_PREVIEW, _T(""));
    SendDlgItemMessage(IDC_MANUALSEARCH_RESULTLIST, LVM_DELETEALLITEMS, 0, 0);
    m_all_lyrics.clear();

    assert(!m_child_update.has_value());
    try
    {
        m_child_update.emplace(m_parent_update.get_type(), m_parent_update.get_track(), m_parent_update.get_track_info(), m_child_abort);
    }
    catch(const std::exception& e)
    {
        LOG_WARN("Failed to add custom search 'child' update, the update was probably cancelled: %s", e.what());
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
    UINT_PTR result = SetTimer(MANUAL_SEARCH_UPDATE_TIMER, 16, nullptr);
    if (result != MANUAL_SEARCH_UPDATE_TIMER)
    {
        LOG_WARN("Unexpected timer result when starting manual search update timer");
    }

    SetDlgItemText(IDC_MANUALSEARCH_PROGRESS, _T("Searching..."));
}

void ManualLyricSearch::save_selected_item()
{
    assert(SendDlgItemMessage(IDC_MANUALSEARCH_RESULTLIST, LVM_GETSELECTEDCOUNT, 0, 0) <= 1);
    LRESULT list_selection = SendDlgItemMessage(IDC_MANUALSEARCH_RESULTLIST, LVM_GETSELECTIONMARK, 0, 0);
    if(list_selection >= 0)
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

            // NOTE: We need to take a copy here because otherwise if we click "Apply" then the
            //       preview for the applied lyrics will be empty (and so will the applied lyrics)
            //       if you click "Apply" again.
            LyricData lyrics_copy = *selected_lyrics;
            m_parent_update.set_result(std::move(lyrics_copy), false);
        }
        else
        {
            LOG_WARN("Failed to get selected search result list entry");
        }
    }
}

void ManualLyricSearch::OnCancel(UINT /*btn_id*/, int /*notification_type*/, CWindow /*btn*/)
{
    DestroyWindow();
}

void ManualLyricSearch::OnOK(UINT /*btn_id*/, int /*notify_code*/, CWindow /*btn*/)
{
    save_selected_item();

    DestroyWindow();
}

void ManualLyricSearch::OnApply(UINT /*btn_id*/, int /*notify_code*/, CWindow /*btn*/)
{
    save_selected_item();
}

void ManualLyricSearch::OnSearchRequested(UINT /*btn_id*/, int /*notify_code*/, CWindow /*btn*/)
{
    start_search();
}

LRESULT ManualLyricSearch::OnTimer(WPARAM)
{
    if(!m_child_update.has_value())
    {
        GetDlgItem(IDC_MANUALSEARCH_SEARCH).EnableWindow(true);
        WIN32_OP(KillTimer(MANUAL_SEARCH_UPDATE_TIMER))
        return 0;
    }

    assert(m_child_update.has_value());
    LyricUpdateHandle& child_update = m_child_update.value();
    if(child_update.is_complete() && !child_update.has_result())
    {
        GetDlgItem(IDC_MANUALSEARCH_SEARCH).EnableWindow(true);
        m_child_update.reset();
        SetDlgItemText(IDC_MANUALSEARCH_PROGRESS, _T("Search complete"));
        WIN32_OP(KillTimer(MANUAL_SEARCH_UPDATE_TIMER))
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
        const LRESULT item_index_result = SendDlgItemMessageW(IDC_MANUALSEARCH_RESULTLIST, LVM_INSERTITEM, 0, (LPARAM)&item);
        assert(item_index_result >= 0);
        assert(item_index_result <= INT_MAX);
        const int item_index = int(item_index_result);

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

    bool aborting = false;
    try
    {
        if(m_parent_update.get_checked_abort().is_aborting())
        {
            aborting = true;
        }
    }
    catch(const std::exception& ex)
    {
        LOG_WARN("Error when checking the child update handle, it was probably cancelled so cancelling our own handle: %s", ex.what());
        aborting = true;
    }
    if(aborting && !m_child_abort.is_aborting())
    {
        m_child_abort.abort();
    }

    return 0;
}

HWND SpawnManualLyricSearch(LyricUpdateHandle& update)
{
    LOG_INFO("Spawning manual search window...");
    HWND result = nullptr;
    try
    {
        auto new_window = fb2k::newDialog<ManualLyricSearch>(update);
        result = new_window->m_hWnd;
    }
    catch(const std::exception& e)
    {
        popup_message::g_complain("Failed to create manual search dialog", e);
    }
    return result;
}

