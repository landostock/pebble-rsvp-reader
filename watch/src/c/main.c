#include <pebble.h>
#include <stdlib.h>

#define STATE_KEY 1
#define SETTINGS_KEY 2
#define BOOKMARKS_KEY 3
#define DEFAULT_WPM 300
#define MIN_WPM 50
#define MAX_WPM 900
#define LOW_WPM_MAX 100
#define LOW_WPM_STEP 10
#define HIGH_WPM_STEP 25
#define MESSAGE_KEY_PACKET 1
#define PROTOCOL_VERSION 1
#define HEADER_LENGTH 46
#define MSG_DATA 1
#define MSG_ACK 2
#define MSG_NACK 3
#define MSG_REQUEST 4
#define MSG_PROGRESS 5
#define MSG_SETTINGS 6
#define READER_WINDOW_VERSION 3
#define READER_WINDOW_HEADER_BYTES 49
#define READER_WINDOW_RECORD_HEADER_BYTES 4
#define READER_WINDOW_MAX_WORDS 80
#define READER_WINDOW_MAX_WORD_BYTES 64
#define READER_WINDOW_MAX_TITLE_BYTES 32
#define READER_WINDOW_BUFFER_BYTES 16384
#define LIBRARY_MAX_BOOKS 12
#define LIBRARY_HEADER_BYTES 12
#define LIBRARY_VERSION 6
#define LIBRARY_ENTRY_BYTES 1068
#define LIBRARY_TITLE_BYTES 48
#define LIBRARY_AUTHOR_BYTES 24
#define LIBRARY_COVER_ROW_BYTES 18
#define LIBRARY_COVER_WIDTH 36
#define LIBRARY_COVER_HEIGHT 52
#define LIBRARY_COVER_BYTES (LIBRARY_COVER_ROW_BYTES * LIBRARY_COVER_HEIGHT)
#define LIBRARY_COVER_PALETTE_BYTES 16
#define CHAPTER_MAX_COUNT 64
#define CHAPTER_HEADER_BYTES 8
#define CHAPTER_ENTRY_BYTES 29
#define CHAPTER_TITLE_BYTES 24
#define STATE_MAGIC 0x50525350UL
#define STATE_VERSION 7
#define SETTINGS_MAGIC 0x50525343UL
#define SETTINGS_VERSION 11
#define PROGRESS_INTERVAL_SECONDS 15
#define MAX_ACTIVE_INTERVAL_MS 60000UL
#define CLOCK_WRAP_SECONDS (UINT32_MAX / 1000UL)
#define LOADING_TIMEOUT_SECONDS 12
#define WPM_FEEDBACK_MS 900
#define PROGRESS_FEEDBACK_MS 1200
#define TRANSPORT_FOLLOWUP_MS 300
#define PREFETCH_TIMEOUT_MS 5000
#define BOOKMARK_MAGIC 0x50424d4bUL
#define BOOKMARK_VERSION 4
#define BOOKMARK_MAX_COUNT 28
#define BOOKMARKS_PER_CHUNK 7
#define BOOKMARK_CHUNK_COUNT 4

typedef enum ReaderProfile {
  READER_PROFILE_FOCUS = 0,
  READER_PROFILE_INFORMATIVE = 1,
  READER_PROFILE_CUSTOM = 2,
} ReaderProfile;

typedef enum AppLanguage {
  APP_LANGUAGE_ENGLISH = 0,
  APP_LANGUAGE_GERMAN = 1,
} AppLanguage;

typedef enum CustomDisplayFlag {
  CUSTOM_SHOW_TITLE = 1 << 0,
  CUSTOM_SHOW_WPM = 1 << 1,
  CUSTOM_SHOW_PROGRESS = 1 << 2,
  CUSTOM_SHOW_GUIDES = 1 << 3,
  CUSTOM_HIGHLIGHT_ORP = 1 << 4,
} CustomDisplayFlag;

typedef enum ReaderBehaviorFlag {
  READER_BEHAVIOR_SENTENCE_RESUME = 1 << 0,
  READER_BEHAVIOR_ADAPTIVE_WORDS = 1 << 1,
  READER_BEHAVIOR_TOUCH = 1 << 2,
  READER_BEHAVIOR_ATTENTION_GUARD = 1 << 3,
  READER_BEHAVIOR_CURRENT_SENTENCE_FIRST = 1 << 6,
  READER_BEHAVIOR_PAUSED_CONTEXT = 1 << 7,
} ReaderBehaviorFlag;

typedef enum OrpColor {
  ORP_COLOR_RED = 0,
  ORP_COLOR_WHITE = 1,
  ORP_COLOR_YELLOW = 2,
  ORP_COLOR_CYAN = 3,
  ORP_COLOR_GREEN = 4,
  ORP_COLOR_COUNT = 5,
} OrpColor;

#define LEGACY_ORP_COLOR_SHIFT 3
#define LEGACY_ORP_COLOR_MASK (7 << LEGACY_ORP_COLOR_SHIFT)

typedef enum MenuDestination {
  MENU_CHAPTERS = 0,
  MENU_BOOKMARKS = 1,
  MENU_LIBRARY = 2,
  MENU_SETTINGS = 3,
  MENU_DISPLAY = 4,
  MENU_STATISTICS = 5,
  MENU_DESTINATION_COUNT = 6,
} MenuDestination;

typedef enum SleepMode {
  SLEEP_MODE_OFF = 0,
  SLEEP_MODE_TIMER = 1,
  SLEEP_MODE_ATTENTION = 2,
  SLEEP_MODE_COUNT = 3,
} SleepMode;

typedef enum SleepBookmarkTiming {
  SLEEP_BOOKMARK_START = 0,
  SLEEP_BOOKMARK_END = 1,
} SleepBookmarkTiming;

typedef enum AttentionConfirmMode {
  ATTENTION_CONFIRM_ANY = 0,
  ATTENTION_CONFIRM_TOUCH = 1,
  ATTENTION_CONFIRM_BUTTON = 2,
  ATTENTION_CONFIRM_SHAKE = 3,
  ATTENTION_CONFIRM_COUNT = 4,
} AttentionConfirmMode;

typedef enum SleepRow {
  SLEEP_ROW_MODE = 0,
  SLEEP_ROW_DURATION = 1,
  SLEEP_ROW_GRACE = 2,
  SLEEP_ROW_CONFIRM = 3,
  SLEEP_ROW_BOOKMARK = 4,
  SLEEP_ROW_START = 5,
  SLEEP_ROW_COUNT = 6,
} SleepRow;

typedef enum SettingRow {
  SETTING_LANGUAGE = 0,
  SETTING_LIBRARY_SORT = 1,
  SETTING_WPM = 2,
  SETTING_RESUME = 3,
  SETTING_LONG_WORDS = 4,
  SETTING_TOUCH = 5,
  SETTING_REWIND = 6,
  SETTING_ATTENTION_GUARD = 7,
  SETTING_SLEEP_MODES = 8,
  SETTING_RETURN_ORIGIN = 9,
  SETTING_ROW_COUNT = 10,
} SettingRow;

typedef enum DisplayRow {
  DISPLAY_PROFILE = 0,
  DISPLAY_ORP_COLOR = 1,
  DISPLAY_TITLE = 2,
  DISPLAY_WPM = 3,
  DISPLAY_PROGRESS = 4,
  DISPLAY_GUIDES = 5,
  DISPLAY_ORP_VISIBLE = 6,
  DISPLAY_BACKLIGHT = 7,
  DISPLAY_PAUSED_CONTEXT = 8,
  DISPLAY_ROW_COUNT = 9,
} DisplayRow;

typedef enum BacklightMode {
  BACKLIGHT_SMART = 0,
  BACKLIGHT_ALWAYS = 1,
  BACKLIGHT_SYSTEM = 2,
  BACKLIGHT_MODE_COUNT = 3,
} BacklightMode;

typedef enum ProgressDisplayMode {
  PROGRESS_DISPLAY_PERCENT = 0,
  PROGRESS_DISPLAY_WORDS = 1,
  PROGRESS_DISPLAY_REMAINING = 2,
  PROGRESS_DISPLAY_MODE_COUNT = 3,
} ProgressDisplayMode;

typedef enum LibrarySort {
  LIBRARY_SORT_LAST_READ = 0,
  LIBRARY_SORT_TITLE = 1,
  LIBRARY_SORT_AUTHOR = 2,
  LIBRARY_SORT_RECENT = 3,
  LIBRARY_SORT_PROGRESS = 4,
  LIBRARY_SORT_COUNT = 5,
} LibrarySort;

typedef enum WindowTarget {
  WINDOW_TARGET_CURRENT = 0,
  WINDOW_TARGET_NEXT = 1,
  WINDOW_TARGET_AFTER = 2,
  WINDOW_TARGET_PREVIOUS = 3,
} WindowTarget;

typedef enum LoadingPresentation {
  LOADING_PRESENTATION_CORNER = 0,
  LOADING_PRESENTATION_BOOK = 1,
  LOADING_PRESENTATION_CHAPTER = 2,
} LoadingPresentation;

typedef struct PersistedState {
  uint32_t magic;
  uint8_t version;
  uint8_t reserved;
  uint16_t wpm;
  uint8_t book_id[16];
  uint32_t book_revision;
  uint32_t absolute_word_index;
  uint32_t furthest_word_index;
  uint32_t total_word_count;
  uint32_t progress_updated_at;
  uint32_t total_words_read;
  uint32_t reading_seconds;
  uint32_t session_count;
  uint32_t last_read_day;
  uint16_t current_streak;
  uint16_t longest_streak;
  uint32_t active_day;
  uint32_t active_day_seconds;
  uint16_t active_day_count;
  uint16_t books_started;
  uint16_t books_completed;
  uint16_t reserved_stats;
  uint32_t started_book_bits_low;
  uint32_t started_book_bits_high;
  uint32_t completed_book_bits_low;
  uint32_t completed_book_bits_high;
} PersistedState;

typedef PersistedState PersistedStateV6;

typedef struct PersistedStateV5 {
  uint32_t magic;
  uint8_t version;
  uint8_t reserved;
  uint16_t wpm;
  uint8_t book_id[16];
  uint32_t book_revision;
  uint32_t absolute_word_index;
  uint32_t total_word_count;
  uint32_t progress_updated_at;
  uint32_t total_words_read;
  uint32_t reading_seconds;
  uint32_t session_count;
  uint32_t last_read_day;
  uint16_t current_streak;
  uint16_t longest_streak;
  uint32_t today_words;
} PersistedStateV5;

typedef struct PersistedStateV4 {
  uint32_t magic;
  uint8_t version;
  uint8_t reserved;
  uint16_t wpm;
  uint8_t book_id[16];
  uint32_t book_revision;
  uint32_t absolute_word_index;
  uint32_t total_word_count;
  uint32_t progress_updated_at;
  uint32_t total_words_read;
  uint32_t reading_seconds;
  uint32_t session_count;
} PersistedStateV4;

typedef struct PersistedStateV3 {
  uint32_t magic;
  uint8_t version;
  uint8_t reserved;
  uint16_t wpm;
  uint8_t book_id[16];
  uint32_t book_revision;
  uint32_t absolute_word_index;
  uint32_t total_word_count;
  uint32_t progress_updated_at;
} PersistedStateV3;

typedef struct PersistedStateV2 {
  uint32_t magic;
  uint8_t version;
  uint8_t reserved;
  uint16_t wpm;
  uint8_t book_id[16];
  uint32_t book_revision;
  uint32_t absolute_word_index;
  uint32_t total_word_count;
} PersistedStateV2;

typedef struct PersistedSettings {
  uint32_t magic;
  uint8_t version;
  uint8_t profile;
  uint8_t custom_display_flags;
  uint8_t behavior_flags;
  uint8_t language;
  uint8_t library_sort;
  uint8_t backlight_mode;
  uint8_t progress_display_mode;
  uint8_t sleep_mode;
  uint8_t sleep_duration_index;
  uint8_t attention_interval_index;
  uint8_t attention_grace_index;
  uint8_t attention_confirm_mode;
  uint8_t sleep_bookmark_timing;
  uint8_t attention_bookmark_timing;
  uint8_t orp_color;
} PersistedSettings;

typedef struct PersistedSettingsV10 {
  uint32_t magic;
  uint8_t version;
  uint8_t profile;
  uint8_t custom_display_flags;
  uint8_t behavior_flags;
  uint8_t language;
  uint8_t library_sort;
  uint8_t backlight_mode;
  uint8_t progress_display_mode;
  uint8_t sleep_mode;
  uint8_t sleep_duration_index;
  uint8_t attention_interval_index;
  uint8_t attention_grace_index;
  uint8_t attention_confirm_mode;
  uint8_t sleep_bookmark_timing;
  uint8_t attention_bookmark_timing;
} PersistedSettingsV10;

_Static_assert(sizeof(PersistedSettings) == 20,
               "Settings persistence layout changed unexpectedly");
_Static_assert(sizeof(PersistedSettingsV10) == 20,
               "Version 10 settings migration layout changed unexpectedly");

typedef struct PersistedSettingsV7 {
  uint32_t magic;
  uint8_t version;
  uint8_t profile;
  uint8_t custom_display_flags;
  uint8_t behavior_flags;
  uint8_t language;
  uint8_t library_sort;
  uint8_t backlight_mode;
  uint8_t progress_display_mode;
  uint8_t sleep_mode;
  uint8_t sleep_duration_index;
  uint8_t attention_grace_index;
  uint8_t attention_confirm_mode;
  uint8_t sleep_bookmark_timing;
} PersistedSettingsV7;

typedef struct PersistedSettingsV6 {
  uint32_t magic;
  uint8_t version;
  uint8_t profile;
  uint8_t custom_display_flags;
  uint8_t behavior_flags;
  uint8_t language;
  uint8_t library_sort;
  uint8_t backlight_mode;
  uint8_t progress_display_mode;
} PersistedSettingsV6;

typedef struct PersistedSettingsV3 {
  uint32_t magic;
  uint8_t version;
  uint8_t profile;
  uint8_t custom_display_flags;
  uint8_t behavior_flags;
  uint8_t language;
  uint8_t library_sort;
  uint8_t backlight_mode;
} PersistedSettingsV3;

typedef struct PersistedSettingsV2 {
  uint32_t magic;
  uint8_t version;
  uint8_t profile;
  uint8_t custom_display_flags;
  uint8_t behavior_flags;
  uint8_t language;
  uint8_t library_sort;
} PersistedSettingsV2;

typedef struct PersistedSettingsV1 {
  uint32_t magic;
  uint8_t version;
  uint8_t profile;
  uint8_t custom_display_flags;
  uint8_t behavior_flags;
} PersistedSettingsV1;

typedef struct BookmarkRecord {
  uint8_t book_id[16];
  uint32_t book_revision;
  uint32_t word_index;
  uint32_t total_word_count;
  uint32_t created_at;
  uint8_t kind;
} __attribute__((packed)) BookmarkRecord;

typedef struct BookmarkCollection {
  uint8_t count;
  BookmarkRecord records[BOOKMARK_MAX_COUNT];
} BookmarkCollection;

typedef struct PersistedBookmarkChunk {
  uint32_t magic;
  uint8_t version;
  uint8_t chunk_index;
  uint8_t count;
  uint8_t total_count;
  BookmarkRecord records[BOOKMARKS_PER_CHUNK];
} __attribute__((packed)) PersistedBookmarkChunk;

typedef struct BookmarkRecordV3 {
  uint8_t book_id[16];
  uint32_t book_revision;
  uint32_t word_index;
  uint32_t total_word_count;
  uint8_t kind;
} __attribute__((packed)) BookmarkRecordV3;

typedef struct PersistedBookmarksV3 {
  uint32_t magic;
  uint8_t version;
  uint8_t count;
  uint16_t reserved;
  BookmarkRecordV3 records[8];
} __attribute__((packed)) PersistedBookmarksV3;

typedef struct BookmarkRecordV2 {
  uint8_t book_id[16];
  uint32_t book_revision;
  uint32_t word_index;
  uint32_t total_word_count;
} BookmarkRecordV2;

typedef struct PersistedBookmarksV2 {
  uint32_t magic;
  uint8_t version;
  uint8_t count;
  uint16_t reserved;
  BookmarkRecordV2 records[8];
} PersistedBookmarksV2;

_Static_assert(sizeof(PersistedBookmarkChunk) <= PERSIST_DATA_MAX_LENGTH,
               "Bookmark chunk exceeds Pebble persistence value limit");

static Window *s_window;
static Layer *s_reader_layer;
static AppTimer *s_timer;
static AppTimer *s_feedback_timer;
static AppTimer *s_transport_followup_timer;
static AppTimer *s_prefetch_timeout_timer;
static AppTimer *s_backlight_release_timer;
static AppTimer *s_sleep_timer;
static AppTimer *s_sleep_close_timer;
static uint8_t s_control_packet[HEADER_LENGTH + 32];
static uint8_t *s_window_buffer;
static uint16_t s_window_buffer_length;
static uint16_t s_window_next_sequence;
static uint16_t s_window_sequence_count;
static uint32_t s_window_request_id;
static uint32_t s_window_chunk_id;
static uint32_t s_request_id_counter;
static uint32_t s_expected_request_id;
static uint32_t s_expected_chunk_id;
static uint32_t s_expected_start_index;
static uint32_t s_expected_revision;
static uint8_t s_expected_book_id[16];
static bool s_expected_start_exact;
static uint8_t s_prefetch_retry_count;
static uint32_t s_prefetch_retry_start;
static WindowTarget s_prefetch_retry_target;
static bool s_remote_loaded;
static char s_remote_words[READER_WINDOW_MAX_WORDS][READER_WINDOW_MAX_WORD_BYTES + 1];
static bool s_remote_paragraph_end[READER_WINDOW_MAX_WORDS];
static bool s_remote_chapter_start[READER_WINDOW_MAX_WORDS];
static bool s_remote_sentence_start[READER_WINDOW_MAX_WORDS];
static uint8_t s_remote_focus_index[READER_WINDOW_MAX_WORDS];
static uint16_t s_remote_base_duration[READER_WINDOW_MAX_WORDS];
typedef struct ReaderWindowCache {
  char words[READER_WINDOW_MAX_WORDS][READER_WINDOW_MAX_WORD_BYTES + 1];
  bool paragraph_end[READER_WINDOW_MAX_WORDS];
  bool chapter_start[READER_WINDOW_MAX_WORDS];
  bool sentence_start[READER_WINDOW_MAX_WORDS];
  uint8_t focus_index[READER_WINDOW_MAX_WORDS];
  uint16_t base_duration[READER_WINDOW_MAX_WORDS];
} ReaderWindowCache;

static ReaderWindowCache *s_previous_cache;
static ReaderWindowCache *s_next_cache;
static ReaderWindowCache *s_after_cache;
#define s_previous_words (s_previous_cache->words)
#define s_previous_paragraph_end (s_previous_cache->paragraph_end)
#define s_previous_chapter_start (s_previous_cache->chapter_start)
#define s_previous_sentence_start (s_previous_cache->sentence_start)
#define s_previous_focus_index (s_previous_cache->focus_index)
#define s_previous_base_duration (s_previous_cache->base_duration)
#define s_next_words (s_next_cache->words)
#define s_next_paragraph_end (s_next_cache->paragraph_end)
#define s_next_chapter_start (s_next_cache->chapter_start)
#define s_next_sentence_start (s_next_cache->sentence_start)
#define s_next_focus_index (s_next_cache->focus_index)
#define s_next_base_duration (s_next_cache->base_duration)
#define s_after_words (s_after_cache->words)
#define s_after_paragraph_end (s_after_cache->paragraph_end)
#define s_after_chapter_start (s_after_cache->chapter_start)
#define s_after_sentence_start (s_after_cache->sentence_start)
#define s_after_focus_index (s_after_cache->focus_index)
#define s_after_base_duration (s_after_cache->base_duration)
static uint16_t s_remote_word_count;
static uint32_t s_remote_start_index;
static uint32_t s_remote_total_count;
static char s_remote_title[READER_WINDOW_MAX_TITLE_BYTES + 1];
static uint8_t s_remote_book_id[16];
static uint32_t s_remote_revision;
static bool s_window_request_sent;
static bool s_navigation_request;
static bool s_resume_context_after_window;
static bool s_pending_loaded;
static bool s_after_loaded;
static bool s_previous_loaded;
static bool s_resume_after_window;
static WindowTarget s_window_target;
static uint16_t s_next_word_count;
static uint32_t s_next_start_index;
static uint32_t s_next_total_count;
static char s_next_title[READER_WINDOW_MAX_TITLE_BYTES + 1];
static uint16_t s_previous_word_count;
static uint32_t s_previous_start_index;
static uint32_t s_previous_total_count;
static char s_previous_title[READER_WINDOW_MAX_TITLE_BYTES + 1];
static uint16_t s_after_word_count;
static uint32_t s_after_start_index;
static uint32_t s_after_total_count;
static char s_after_title[READER_WINDOW_MAX_TITLE_BYTES + 1];
static bool s_gallery_mode;
static bool s_waiting_for_book;
static bool s_waiting_for_chapters;
static bool s_pending_chapter_request;
static bool s_pending_book_request;
static bool s_pending_book_sentence_resume;
static uint32_t s_pending_book_start;
static uint8_t s_pending_book_id[16];
static uint32_t s_pending_book_revision;
static uint8_t s_book_request_attempts;
static LoadingPresentation s_pending_book_loading_presentation;
static bool s_loading_mode = true;
static LoadingPresentation s_loading_presentation = LOADING_PRESENTATION_CORNER;
static bool s_chapter_mode;
static bool s_reader_menu_mode;
static bool s_settings_mode;
static bool s_display_mode;
static bool s_statistics_mode;
static bool s_bookmark_mode;
static bool s_sleep_mode_menu;
static bool s_resume_request_pending;
static bool s_progress_pending;
static bool s_confirmed_dirty;
static bool s_progress_outbox_active;
static bool s_settings_outbox_active;
static bool s_request_outbox_active;
static bool s_library_sync_outbox_active;
static bool s_library_sync_pending;
static bool s_account_settings_pending;
static bool s_account_settings_awaiting_confirmation;
static uint8_t s_loading_frame;
static AppTimer *s_loading_timer;
static time_t s_loading_started_sec;
static uint8_t s_library_book_ids[LIBRARY_MAX_BOOKS][16];
static uint32_t s_library_revisions[LIBRARY_MAX_BOOKS];
static char s_library_titles[LIBRARY_MAX_BOOKS][LIBRARY_TITLE_BYTES + 1];
static char s_library_authors[LIBRARY_MAX_BOOKS][LIBRARY_AUTHOR_BYTES + 1];
static uint32_t s_library_progress_index[LIBRARY_MAX_BOOKS];
static uint32_t s_library_resume_index[LIBRARY_MAX_BOOKS];
static uint32_t s_library_total_count[LIBRARY_MAX_BOOKS];
static uint8_t s_library_progress_percent[LIBRARY_MAX_BOOKS];
static uint32_t s_library_progress_updated_at[LIBRARY_MAX_BOOKS];
static uint32_t s_library_created_at[LIBRARY_MAX_BOOKS];
static bool s_library_has_cover[LIBRARY_MAX_BOOKS];
static uint8_t (*s_library_covers)[LIBRARY_COVER_BYTES];
static uint8_t (*s_library_cover_palettes)[LIBRARY_COVER_PALETTE_BYTES];
static uint8_t s_library_count;
static uint8_t s_library_selected;
static uint32_t s_chapter_start_indices[CHAPTER_MAX_COUNT];
static char s_chapter_titles[CHAPTER_MAX_COUNT][CHAPTER_TITLE_BYTES + 1];
static uint8_t s_chapter_count;
static uint8_t s_chapter_selected;
static uint8_t s_reader_menu_selected;
static uint8_t s_settings_selected;
static uint8_t s_display_selected;
static uint8_t s_statistics_page;
static uint8_t s_bookmark_selected;
static bool s_bookmark_dialog_mode;
static uint8_t s_bookmark_dialog_selected;
static uint8_t s_sleep_selected;
static BookmarkCollection s_bookmarks;
static AppLanguage s_language = APP_LANGUAGE_ENGLISH;
static LibrarySort s_library_sort = LIBRARY_SORT_LAST_READ;
static PersistedState s_state = {
  .magic = STATE_MAGIC,
  .version = STATE_VERSION,
  .wpm = DEFAULT_WPM,
};
static PersistedSettings s_settings = {
  .magic = SETTINGS_MAGIC,
  .version = SETTINGS_VERSION,
  .profile = READER_PROFILE_FOCUS,
  .custom_display_flags = CUSTOM_SHOW_TITLE | CUSTOM_SHOW_WPM | CUSTOM_SHOW_PROGRESS |
                          CUSTOM_SHOW_GUIDES | CUSTOM_HIGHLIGHT_ORP,
  .behavior_flags = READER_BEHAVIOR_SENTENCE_RESUME | READER_BEHAVIOR_ADAPTIVE_WORDS |
                    READER_BEHAVIOR_TOUCH | READER_BEHAVIOR_ATTENTION_GUARD |
                    READER_BEHAVIOR_CURRENT_SENTENCE_FIRST | READER_BEHAVIOR_PAUSED_CONTEXT,
  .language = APP_LANGUAGE_ENGLISH,
  .library_sort = LIBRARY_SORT_LAST_READ,
  .backlight_mode = BACKLIGHT_SMART,
  .progress_display_mode = PROGRESS_DISPLAY_PERCENT,
  .sleep_mode = SLEEP_MODE_OFF,
  .sleep_duration_index = 1,
  .attention_interval_index = 2,
  .attention_grace_index = 1,
  .attention_confirm_mode = ATTENTION_CONFIRM_ANY,
  .sleep_bookmark_timing = SLEEP_BOOKMARK_END,
  .attention_bookmark_timing = SLEEP_BOOKMARK_START,
  .orp_color = ORP_COLOR_YELLOW,
};
static uint16_t s_word_index;
static uint8_t s_word_segment_index;
static bool s_playing;
static bool s_context_mode;
static uint16_t s_context_page_start;
static uint16_t s_context_page_end;
static uint32_t s_context_history[8];
static uint8_t s_context_history_count;
static bool s_context_find_last_page;
static uint16_t s_context_last_page_end;
static bool s_pending_context_navigation;
static uint32_t s_pending_context_start;
static bool s_pending_reader_navigation;
static char s_pending_reader_command[8];
static uint32_t s_pending_reader_start;
static uint32_t s_wpm_feedback_until_ms;
static uint32_t s_progress_feedback_until_ms;
static time_t s_last_persist_sec;
static uint32_t s_session_words;
static uint32_t s_session_active_ms;
static uint32_t s_active_ms_remainder;
static bool s_session_started;
static bool s_session_counted;
static bool s_backlight_forced;
static uint32_t s_play_tick_ms;
static bool s_candidate_active;
static bool s_candidate_forward;
static uint32_t s_candidate_origin;
static uint32_t s_candidate_target;
static uint32_t s_candidate_active_ms;
static uint32_t s_candidate_words;
static bool s_origin_return_available;
static bool s_origin_dialog_mode;
static uint8_t s_origin_dialog_selected;
static bool s_origin_dialog_offered;
static bool s_select_button_down;
static bool s_up_button_down;
static bool s_down_button_down;
static bool s_button_chord_consumed;
static bool s_sleep_session_active;
static bool s_attention_guard_active;
static bool s_attention_prompt;
static SleepMode s_attention_prompt_mode;
static bool s_attention_stop_pending;
static bool s_sleep_end_pending;
static bool s_sleep_settling;
static bool s_sleep_resume_after_settle;
static uint32_t s_sleep_settle_started_ms;
static bool s_sleep_shortcut_mode;
static uint8_t s_sleep_shortcut_selected;
static bool s_sleep_shortcut_was_playing;
static time_t s_sleep_deadline;
static time_t s_attention_prompt_deadline;
static uint32_t s_sleep_phase_start_position;
static bool s_touch_dispatching;

#ifdef _PBL_API_EXISTS_touch_service_subscribe
static bool s_touch_subscribed;
static bool s_touch_candidate;
static GPoint s_touch_start;
#endif

static void set_loading(bool loading);
static void set_loading_with_presentation(bool loading, LoadingPresentation presentation);
static void feedback_timer_callback(void *context);
static void schedule_transport_followup(void);
static void cancel_prefetch_timeout(void);
static SleepMode active_monitor_mode(void);
static bool monitor_active(void);
#ifdef _PBL_API_EXISTS_touch_service_subscribe
static void touch_service_handler(const TouchEvent *event, void *context);
#endif
static void expect_response(uint32_t request_id, uint32_t chunk_id, uint32_t start_index,
                            const uint8_t *book_id, uint32_t revision, WindowTarget target,
                            bool exact_start);

static bool playback_requested(void) {
  return s_playing || s_resume_after_window;
}

static uint32_t next_request_id(void) {
  if (s_request_id_counter == 0) {
    s_request_id_counter = (uint32_t)time(NULL);
  }
  s_request_id_counter += 1;
  if (s_request_id_counter == 0) {
    s_request_id_counter = 1;
  }
  return s_request_id_counter;
}

static const char *word_at(uint16_t index) {
  return s_remote_loaded && index < s_remote_word_count ? s_remote_words[index] : "";
}

static uint16_t read_u16_be(const uint8_t *data) {
  return ((uint16_t)data[0] << 8) | (uint16_t)data[1];
}

static uint32_t read_u32_be(const uint8_t *data) {
  return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | (uint32_t)data[3];
}

static void write_u16_be(uint8_t *data, uint16_t value) {
  data[0] = (uint8_t)(value >> 8);
  data[1] = (uint8_t)value;
}

static void write_u32_be(uint8_t *data, uint32_t value) {
  data[0] = (uint8_t)(value >> 24);
  data[1] = (uint8_t)(value >> 16);
  data[2] = (uint8_t)(value >> 8);
  data[3] = (uint8_t)value;
}

static const char *localized(const char *english, const char *german) {
  return s_language == APP_LANGUAGE_GERMAN ? german : english;
}

static uint32_t crc32_bytes(const uint8_t *data, uint16_t length) {
  uint32_t crc = 0xffffffffUL;
  for (uint16_t index = 0; index < length; index += 1) {
    crc ^= data[index];
    for (uint8_t bit = 0; bit < 8; bit += 1) {
      crc = (crc & 1) ? (0xedb88320UL ^ (crc >> 1)) : (crc >> 1);
    }
  }
  return crc ^ 0xffffffffUL;
}

static bool validate_packet(const uint8_t *packet, uint16_t length) {
  if (length < HEADER_LENGTH) {
    return false;
  }
  if (packet[0] != 'P' || packet[1] != 'R' || packet[2] != 'S' || packet[3] != 'P') {
    return false;
  }
  if (packet[4] != PROTOCOL_VERSION || packet[7] != HEADER_LENGTH) {
    return false;
  }

  const uint16_t sequence_number = read_u16_be(packet + 20);
  const uint16_t sequence_count = read_u16_be(packet + 22);
  const uint16_t payload_length = read_u16_be(packet + 24);
  if (sequence_count == 0 || sequence_number >= sequence_count) {
    return false;
  }
  if ((uint16_t)(HEADER_LENGTH + payload_length) != length) {
    return false;
  }
  return read_u32_be(packet + 26) == crc32_bytes(packet + HEADER_LENGTH, payload_length);
}

static void send_control_packet(uint8_t message_type, const uint8_t *received, uint16_t received_length, const char *reason) {
  const uint16_t payload_length = reason == NULL ? 0 : (uint16_t)strlen(reason);
  const uint16_t bounded_payload_length =
      payload_length > sizeof(s_control_packet) - HEADER_LENGTH ? sizeof(s_control_packet) - HEADER_LENGTH : payload_length;
  memset(s_control_packet, 0, sizeof(s_control_packet));
  s_control_packet[0] = 'P';
  s_control_packet[1] = 'R';
  s_control_packet[2] = 'S';
  s_control_packet[3] = 'P';
  s_control_packet[4] = PROTOCOL_VERSION;
  s_control_packet[5] = message_type;
  s_control_packet[7] = HEADER_LENGTH;

  if (received != NULL && received_length >= HEADER_LENGTH) {
    memcpy(s_control_packet + 8, received + 8, 18);
    memcpy(s_control_packet + 30, received + 30, 16);
  } else {
    write_u16_be(s_control_packet + 22, 1);
  }

  write_u16_be(s_control_packet + 24, bounded_payload_length);
  if (bounded_payload_length > 0) {
    memcpy(s_control_packet + HEADER_LENGTH, reason, bounded_payload_length);
  }
  write_u32_be(s_control_packet + 26, crc32_bytes(s_control_packet + HEADER_LENGTH, bounded_payload_length));

  DictionaryIterator *out_iter;
  AppMessageResult result = app_message_outbox_begin(&out_iter);
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "outbox begin failed: %d", (int)result);
    return;
  }
  dict_write_data(out_iter, MESSAGE_KEY_PACKET, s_control_packet, HEADER_LENGTH + bounded_payload_length);
  result = app_message_outbox_send();
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "outbox send failed: %d", (int)result);
  }
}

static uint16_t word_count(void) {
  return s_remote_loaded ? s_remote_word_count : 0;
}

static bool is_paragraph_end(uint16_t word_index) {
  return s_remote_loaded && word_count() > 0 &&
         (word_index >= word_count() - 1 || s_remote_paragraph_end[word_index]);
}

static uint32_t utf8_next(const char **cursor) {
  const uint8_t *bytes = (const uint8_t *)*cursor;
  if (bytes[0] == 0) {
    return 0;
  }

  if (bytes[0] < 0x80) {
    *cursor += 1;
    return bytes[0];
  }
  if ((bytes[0] & 0xe0) == 0xc0 && bytes[1] != 0) {
    *cursor += 2;
    return ((uint32_t)(bytes[0] & 0x1f) << 6) | (uint32_t)(bytes[1] & 0x3f);
  }
  if ((bytes[0] & 0xf0) == 0xe0 && bytes[1] != 0 && bytes[2] != 0) {
    *cursor += 3;
    return ((uint32_t)(bytes[0] & 0x0f) << 12) | ((uint32_t)(bytes[1] & 0x3f) << 6) |
           (uint32_t)(bytes[2] & 0x3f);
  }
  if ((bytes[0] & 0xf8) == 0xf0 && bytes[1] != 0 && bytes[2] != 0 && bytes[3] != 0) {
    *cursor += 4;
    return ((uint32_t)(bytes[0] & 0x07) << 18) | ((uint32_t)(bytes[1] & 0x3f) << 12) |
           ((uint32_t)(bytes[2] & 0x3f) << 6) | (uint32_t)(bytes[3] & 0x3f);
  }

  *cursor += 1;
  return bytes[0];
}

static bool is_ascii_lower(uint32_t codepoint) {
  return codepoint >= 'a' && codepoint <= 'z';
}

static bool is_ascii_upper(uint32_t codepoint) {
  return codepoint >= 'A' && codepoint <= 'Z';
}

static bool is_digit(uint32_t codepoint) {
  return codepoint >= '0' && codepoint <= '9';
}

static bool is_latin_letter(uint32_t codepoint) {
  return is_ascii_lower(codepoint) || is_ascii_upper(codepoint) || (codepoint >= 0x00c0 && codepoint <= 0x024f);
}

static bool is_word_character(uint32_t codepoint) {
  return is_latin_letter(codepoint) || is_digit(codepoint);
}

static bool is_vowel(uint32_t codepoint) {
  switch (codepoint) {
    case 'a':
    case 'e':
    case 'i':
    case 'o':
    case 'u':
    case 'y':
    case 'A':
    case 'E':
    case 'I':
    case 'O':
    case 'U':
    case 'Y':
    case 0x00e4:
    case 0x00f6:
    case 0x00fc:
    case 0x00c4:
    case 0x00d6:
    case 0x00dc:
      return true;
    default:
      return false;
  }
}

static uint16_t word_character_count(const char *word) {
  uint16_t count = 0;
  const char *cursor = word;
  while (*cursor != 0) {
    const uint32_t codepoint = utf8_next(&cursor);
    if (is_word_character(codepoint)) {
      count += 1;
    }
  }
  return count;
}

static size_t dash_byte_length(const char *cursor) {
  const uint8_t *bytes = (const uint8_t *)cursor;
  if (bytes[0] == '-') {
    return 1;
  }
  if (bytes[0] == 0xe2 && bytes[1] == 0x80 && bytes[2] >= 0x90 && bytes[2] <= 0x95) {
    return 3;
  }
  return 0;
}

static uint8_t word_segment_count(const char *word) {
  if (word_character_count(word) < 14) {
    return 1;
  }
  uint8_t count = 1;
  const char *segment_start = word;
  for (const char *cursor = word; *cursor != 0;) {
    const size_t dash_length = dash_byte_length(cursor);
    if (dash_length > 0) {
      if (cursor > segment_start && cursor[dash_length] != 0 && count < UINT8_MAX) {
        count += 1;
        segment_start = cursor + dash_length;
      }
      cursor += dash_length;
    } else {
      utf8_next(&cursor);
    }
  }
  return count;
}

static void copy_word_segment(const char *word, uint8_t requested_segment, char *target,
                              size_t target_size) {
  if (target_size == 0) {
    return;
  }
  const uint8_t count = word_segment_count(word);
  if (count == 1 || requested_segment >= count) {
    snprintf(target, target_size, "%s", word);
    return;
  }

  const char *segment_start = word;
  uint8_t segment = 0;
  for (const char *cursor = word; *cursor != 0;) {
    const size_t dash_length = dash_byte_length(cursor);
    if (dash_length > 0 && cursor > segment_start && cursor[dash_length] != 0) {
      if (segment == requested_segment) {
        const size_t length = (size_t)(cursor - segment_start) + dash_length;
        const size_t bounded = length < target_size - 1 ? length : target_size - 1;
        memcpy(target, segment_start, bounded);
        target[bounded] = 0;
        return;
      }
      segment += 1;
      segment_start = cursor + dash_length;
      cursor += dash_length;
    } else if (dash_length > 0) {
      cursor += dash_length;
    } else {
      utf8_next(&cursor);
    }
  }
  snprintf(target, target_size, "%s", segment_start);
}

static uint16_t approximate_syllable_groups(const char *word) {
  uint16_t groups = 0;
  uint16_t letters = 0;
  bool previous_was_vowel = false;
  uint32_t previous_letter = 0;
  uint32_t last_letter = 0;

  const char *cursor = word;
  while (*cursor != 0) {
    const uint32_t codepoint = utf8_next(&cursor);
    if (!is_latin_letter(codepoint)) {
      previous_was_vowel = false;
      continue;
    }
    letters += 1;
    previous_letter = last_letter;
    last_letter = codepoint;
    const bool vowel = is_vowel(codepoint);
    if (vowel && !previous_was_vowel) {
      groups += 1;
    }
    previous_was_vowel = vowel;
  }

  if (groups > 1 && letters > 3 && last_letter == 'e' && previous_letter != 'l' && previous_letter != 'y') {
    groups -= 1;
  }
  return groups == 0 && letters > 0 ? 1 : groups;
}

static bool is_ignored_trailing(uint32_t codepoint) {
  switch (codepoint) {
    case '"':
    case '\'':
    case ')':
    case ']':
    case '}':
    case 0x2019:
    case 0x201d:
    case 0x201c:
    case 0x201e:
    case 0x201a:
    case 0x00bb:
    case 0x00ab:
    case 0x2039:
    case 0x203a:
      return true;
    default:
      return false;
  }
}

static uint32_t trailing_rhythm_character(const char *word) {
  uint32_t last = 0;
  const char *cursor = word;
  while (*cursor != 0) {
    const uint32_t codepoint = utf8_next(&cursor);
    if (!is_ignored_trailing(codepoint)) {
      last = codepoint;
    }
  }

  switch (last) {
    case 0x2013:
    case 0x2014:
      return '-';
    case 0x2026:
      return 0x2026;
    default:
      return last;
  }
}

static bool starts_with_lowercase_letter(const char *word) {
  const char *cursor = word;
  while (*cursor != 0) {
    const uint32_t codepoint = utf8_next(&cursor);
    if (is_ascii_lower(codepoint) || codepoint == 0x00e4 || codepoint == 0x00f6 || codepoint == 0x00fc ||
        codepoint == 0x00df) {
      return true;
    }
    if (is_latin_letter(codepoint)) {
      return false;
    }
  }
  return false;
}

static bool word_looks_like_abbreviation(const char *word, bool next_starts_lowercase) {
  if (strcmp(word, "Dr.") == 0 || strcmp(word, "Prof.") == 0 || strcmp(word, "z.B.") == 0 ||
      strcmp(word, "bzw.") == 0 || strcmp(word, "ca.") == 0) {
    return true;
  }

  const uint16_t readable = word_character_count(word);
  const size_t length = strlen(word);
  if (length == 0 || word[length - 1] != '.') {
    return false;
  }
  if (readable <= 2) {
    return true;
  }
  return next_starts_lowercase && readable <= 4;
}

static bool word_ends_sentence_at(uint16_t index) {
  if (index >= word_count()) {
    return false;
  }

  const char *word = word_at(index);
  const char *next = index + 1 < word_count() ? word_at(index + 1) : "";
  const uint32_t trailing = trailing_rhythm_character(word);
  if (trailing == '!' || trailing == '?' || trailing == 0x2026) {
    return true;
  }
  if (trailing == '.') {
    return !word_looks_like_abbreviation(word, starts_with_lowercase_letter(next));
  }
  return false;
}

static uint16_t sentence_start_at_or_before(uint16_t word_index) {
  if (word_count() == 0) {
    return 0;
  }
  if (word_index >= word_count()) {
    word_index = word_count() - 1;
  }
  while (word_index > 0) {
    if (s_remote_sentence_start[word_index] || word_ends_sentence_at(word_index - 1)) {
      return word_index;
    }
    word_index -= 1;
  }
  return 0;
}

static uint16_t progress_tenths(uint32_t index, uint32_t total) {
  if (total <= 1) {
    return 0;
  }
  if (index >= total) {
    index = total - 1;
  }
  return (uint16_t)(((uint64_t)index * 1000ULL) / (uint64_t)(total - 1));
}

static uint32_t current_millis(void) {
  time_t seconds;
  uint16_t milliseconds;
  time_ms(&seconds, &milliseconds);
  return (uint32_t)seconds * 1000UL + milliseconds;
}

static bool behavior_enabled(ReaderBehaviorFlag flag) {
  return (s_settings.behavior_flags & flag) != 0;
}

static void cancel_backlight_release(void) {
  if (s_backlight_release_timer != NULL) {
    app_timer_cancel(s_backlight_release_timer);
    s_backlight_release_timer = NULL;
  }
}

static void backlight_release_callback(void *context) {
  s_backlight_release_timer = NULL;
  if (s_settings.backlight_mode != BACKLIGHT_SMART || playback_requested() || s_context_mode) {
    return;
  }
  light_enable(false);
  s_backlight_forced = false;
}

static void update_backlight(void) {
  const bool should_force = s_settings.backlight_mode == BACKLIGHT_ALWAYS ||
      (s_settings.backlight_mode == BACKLIGHT_SMART && (playback_requested() || s_context_mode));
  if (should_force) {
    cancel_backlight_release();
    if (!s_backlight_forced) {
      light_enable(true);
      s_backlight_forced = true;
    }
    return;
  }
  if (s_settings.backlight_mode == BACKLIGHT_SMART && s_backlight_forced) {
    if (s_backlight_release_timer == NULL) {
      light_enable_interaction();
      s_backlight_release_timer = app_timer_register(3000, backlight_release_callback, NULL);
    }
    return;
  }
  cancel_backlight_release();
  if (s_backlight_forced) {
    light_enable(false);
    s_backlight_forced = false;
  }
}

static void apply_touch_setting(void) {
#ifdef _PBL_API_EXISTS_touch_service_subscribe
  const bool enabled = behavior_enabled(READER_BEHAVIOR_TOUCH);
  if (enabled && !s_touch_subscribed) {
    touch_service_subscribe(touch_service_handler, NULL);
    s_touch_subscribed = true;
  } else if (!enabled && s_touch_subscribed) {
    touch_service_unsubscribe();
    s_touch_subscribed = false;
    s_touch_candidate = false;
  }
#endif
}

static bool custom_display_enabled(CustomDisplayFlag flag) {
  return (s_settings.custom_display_flags & flag) != 0;
}

static uint8_t orp_color_index(void) {
  const uint8_t index = s_settings.orp_color;
  return index < ORP_COLOR_COUNT ? index : ORP_COLOR_RED;
}

static const char *orp_color_name(void) {
  switch (orp_color_index()) {
    case ORP_COLOR_WHITE:
      return localized("White", "Weiss");
    case ORP_COLOR_YELLOW:
      return localized("Yellow", "Gelb");
    case ORP_COLOR_CYAN:
      return "Cyan";
    case ORP_COLOR_GREEN:
      return localized("Green", "Gruen");
    default:
      return localized("Red", "Rot");
  }
}

static void cycle_orp_color(void) {
  s_settings.orp_color = (uint8_t)((orp_color_index() + 1) % ORP_COLOR_COUNT);
}

static GColor orp_focus_color(void) {
  switch (orp_color_index()) {
    case ORP_COLOR_WHITE:
      return GColorWhite;
    case ORP_COLOR_YELLOW:
      return PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite);
    case ORP_COLOR_CYAN:
      return PBL_IF_COLOR_ELSE(GColorCyan, GColorWhite);
    case ORP_COLOR_GREEN:
      return PBL_IF_COLOR_ELSE(GColorGreen, GColorWhite);
    default:
      return PBL_IF_COLOR_ELSE(GColorRed, GColorWhite);
  }
}

static GColor orp_neighbor_color(bool marker_visible) {
  return marker_visible && orp_color_index() == ORP_COLOR_WHITE ? GColorLightGray : GColorWhite;
}

static bool reader_display_enabled(CustomDisplayFlag flag) {
  if (s_context_mode) {
    return flag == CUSTOM_SHOW_TITLE || flag == CUSTOM_SHOW_WPM || flag == CUSTOM_SHOW_PROGRESS;
  }
  if (!playback_requested()) {
    return true;
  }
  if (s_settings.profile == READER_PROFILE_INFORMATIVE) {
    return true;
  }
  if (s_settings.profile == READER_PROFILE_CUSTOM) {
    return custom_display_enabled(flag);
  }
  if (flag == CUSTOM_SHOW_GUIDES || flag == CUSTOM_HIGHLIGHT_ORP) {
    return true;
  }
  return false;
}

static uint32_t local_day_number(time_t instant) {
  const struct tm *local = localtime(&instant);
  if (local == NULL) {
    return 0;
  }
  int32_t year = local->tm_year + 1900;
  const uint32_t month = (uint32_t)local->tm_mon + 1;
  const uint32_t day = (uint32_t)local->tm_mday;
  year -= month <= 2;
  const int32_t era = (year >= 0 ? year : year - 399) / 400;
  const uint32_t year_of_era = (uint32_t)(year - era * 400);
  const uint32_t shifted_month = month > 2 ? month - 3 : month + 9;
  const uint32_t day_of_year = (153 * shifted_month + 2) / 5 + day - 1;
  const uint32_t day_of_era = year_of_era * 365 + year_of_era / 4 - year_of_era / 100 + day_of_year;
  const int32_t serial = era * 146097 + (int32_t)day_of_era - 719468;
  return serial > 0 ? (uint32_t)serial : 0;
}

static uint32_t current_absolute_position(void) {
  if (!s_remote_loaded || s_remote_total_count == 0) {
    return s_state.absolute_word_index;
  }
  uint32_t position = s_remote_start_index + s_word_index;
  return position < s_remote_total_count ? position : s_remote_total_count - 1;
}

static uint8_t current_book_hash_bit(void) {
  uint32_t hash = 2166136261UL;
  for (uint8_t index = 0; index < sizeof(s_remote_book_id); index += 1) {
    hash = (hash ^ s_remote_book_id[index]) * 16777619UL;
  }
  return (uint8_t)(hash & 63);
}

static void mark_book_started(void) {
  const uint8_t bit = current_book_hash_bit();
  uint32_t *word = bit < 32 ? &s_state.started_book_bits_low : &s_state.started_book_bits_high;
  const uint32_t mask = 1UL << (bit & 31);
  if ((*word & mask) == 0) {
    *word |= mask;
    if (s_state.books_started < UINT16_MAX) {
      s_state.books_started += 1;
    }
  }
}

static void mark_book_completed_if_needed(void) {
  if (s_state.total_word_count <= 1 || progress_tenths(s_state.absolute_word_index,
                                                       s_state.total_word_count) < 999) {
    return;
  }
  const uint8_t bit = current_book_hash_bit();
  uint32_t *word = bit < 32 ? &s_state.completed_book_bits_low : &s_state.completed_book_bits_high;
  const uint32_t mask = 1UL << (bit & 31);
  if ((*word & mask) == 0) {
    *word |= mask;
    if (s_state.books_completed < UINT16_MAX) {
      s_state.books_completed += 1;
    }
  }
}

static void account_active_seconds(uint32_t seconds) {
  if (seconds == 0) {
    return;
  }
  if (UINT32_MAX - s_state.reading_seconds < seconds) {
    s_state.reading_seconds = UINT32_MAX;
  } else {
    s_state.reading_seconds += seconds;
  }
  const uint32_t today = local_day_number(time(NULL));
  if (today == 0) {
    return;
  }
  if (s_state.active_day != today) {
    s_state.active_day = today;
    s_state.active_day_seconds = 0;
  }
  const bool was_qualified = s_state.active_day_seconds >= 60;
  s_state.active_day_seconds = UINT32_MAX - s_state.active_day_seconds < seconds
      ? UINT32_MAX : s_state.active_day_seconds + seconds;
  if (!was_qualified && s_state.active_day_seconds >= 60) {
    s_state.current_streak = today == s_state.last_read_day + 1 && s_state.current_streak > 0
        ? (s_state.current_streak < UINT16_MAX ? s_state.current_streak + 1 : UINT16_MAX) : 1;
    s_state.last_read_day = today;
    if (s_state.current_streak > s_state.longest_streak) {
      s_state.longest_streak = s_state.current_streak;
    }
    if (s_state.active_day_count < UINT16_MAX) {
      s_state.active_day_count += 1;
    }
  }
}

static void commit_words(uint32_t count) {
  if (count == 0) {
    return;
  }
  s_state.total_words_read = UINT32_MAX - s_state.total_words_read < count
      ? UINT32_MAX : s_state.total_words_read + count;
  s_session_words = UINT32_MAX - s_session_words < count ? UINT32_MAX : s_session_words + count;
  mark_book_started();
}

static void add_committed_active_ms(uint32_t milliseconds) {
  if (milliseconds > 0 && !s_session_counted) {
    if (s_state.session_count < UINT32_MAX) s_state.session_count += 1;
    s_session_counted = true;
  }
  s_session_active_ms = UINT32_MAX - s_session_active_ms < milliseconds
      ? UINT32_MAX : s_session_active_ms + milliseconds;
  const uint32_t combined = s_active_ms_remainder + milliseconds;
  account_active_seconds(combined / 1000);
  s_active_ms_remainder = combined % 1000;
}

static void confirm_position(uint32_t position) {
  if (s_remote_total_count > 0 && position >= s_remote_total_count) {
    position = s_remote_total_count - 1;
  }
  if (s_state.absolute_word_index != position) {
    s_state.absolute_word_index = position;
    s_confirmed_dirty = true;
  }
  if (position > s_state.furthest_word_index) {
    s_state.furthest_word_index = position;
  }
  s_state.total_word_count = s_remote_total_count;
  mark_book_completed_if_needed();
}

static void confirm_candidate_if_ready(void) {
  if (!s_candidate_active || s_candidate_active_ms < 10000) {
    return;
  }
  add_committed_active_ms(s_candidate_active_ms);
  commit_words(s_candidate_words);
  confirm_position(current_absolute_position());
  s_candidate_active = false;
  s_candidate_active_ms = 0;
  s_candidate_words = 0;
  s_origin_return_available = false;
  s_progress_pending = true;
}

static void record_reading_word(void) {
  if (s_candidate_active) {
    if (s_candidate_words < UINT32_MAX) {
      s_candidate_words += 1;
    }
    return;
  }
  commit_words(1);
  confirm_position(current_absolute_position());
  s_progress_pending = true;
}

static uint16_t effective_current_streak(void) {
  const uint32_t today = local_day_number(time(NULL));
  return today > 0 && s_state.last_read_day > 0 &&
      (today == s_state.last_read_day || today == s_state.last_read_day + 1)
      ? s_state.current_streak : 0;
}

static void commit_reading_time(void) {
  if (!s_playing || s_play_tick_ms == 0) {
    return;
  }
  const uint32_t now = current_millis();
  const uint32_t elapsed = now - s_play_tick_ms;
  s_play_tick_ms = now;
  if (elapsed > MAX_ACTIVE_INTERVAL_MS) {
    return;
  }
  if (s_candidate_active) {
    s_candidate_active_ms = UINT32_MAX - s_candidate_active_ms < elapsed
        ? UINT32_MAX : s_candidate_active_ms + elapsed;
    confirm_candidate_if_ready();
  } else {
    add_committed_active_ms(elapsed);
  }
}

static void stop_playback_clock(void) {
  if (s_playing) commit_reading_time();
  s_playing = false;
  s_play_tick_ms = 0;
}

static void persist_state(void) {
  commit_reading_time();
  if (s_remote_loaded && s_remote_total_count > 0) {
    s_state.magic = STATE_MAGIC;
    s_state.version = STATE_VERSION;
    memcpy(s_state.book_id, s_remote_book_id, sizeof(s_state.book_id));
    s_state.book_revision = s_remote_revision;
    s_state.total_word_count = s_remote_total_count;
    if (s_confirmed_dirty) {
      const time_t now = time(NULL);
      s_state.progress_updated_at = now > 0 ? (uint32_t)now : s_state.progress_updated_at;
    }
    for (uint8_t index = 0; index < s_library_count; index += 1) {
      if (s_library_revisions[index] == s_remote_revision &&
          memcmp(s_library_book_ids[index], s_remote_book_id, 16) == 0) {
        s_library_resume_index[index] = s_state.absolute_word_index;
        s_library_progress_index[index] = s_state.furthest_word_index;
        s_library_total_count[index] = s_remote_total_count;
        s_library_progress_percent[index] = s_remote_total_count <= 1
            ? 0
            : (uint8_t)((s_library_resume_index[index] * 100UL) /
                        (s_remote_total_count - 1));
        s_library_progress_updated_at[index] = s_state.progress_updated_at;
        break;
      }
    }
    if (s_confirmed_dirty) {
      s_progress_pending = true;
      s_confirmed_dirty = false;
    }
  }
  persist_write_data(STATE_KEY, &s_state, sizeof(s_state));
  s_last_persist_sec = time(NULL);
}

static void persist_periodically(void) {
  const time_t now = time(NULL);
  if (now - s_last_persist_sec >= PROGRESS_INTERVAL_SECONDS) {
    persist_state();
  }
}

static void load_state(void) {
  PersistedState loaded;
  if (persist_exists(STATE_KEY) && persist_read_data(STATE_KEY, &loaded, sizeof(loaded)) == (int)sizeof(loaded) &&
      loaded.magic == STATE_MAGIC && loaded.version == STATE_VERSION) {
    s_state = loaded;
  } else if (persist_exists(STATE_KEY) && persist_get_size(STATE_KEY) == (int)sizeof(PersistedStateV6)) {
    PersistedStateV6 previous;
    if (persist_read_data(STATE_KEY, &previous, sizeof(previous)) == (int)sizeof(previous) &&
        previous.magic == STATE_MAGIC && previous.version == 6) {
      s_state = previous;
      s_state.version = STATE_VERSION;
      if (s_state.reading_seconds >= CLOCK_WRAP_SECONDS) {
        s_state.reading_seconds -= CLOCK_WRAP_SECONDS;
      }
      if (s_state.active_day_seconds >= CLOCK_WRAP_SECONDS) {
        s_state.active_day_seconds -= CLOCK_WRAP_SECONDS;
      }
    }
  } else if (persist_exists(STATE_KEY) && persist_get_size(STATE_KEY) == (int)sizeof(PersistedStateV5)) {
    PersistedStateV5 previous;
    if (persist_read_data(STATE_KEY, &previous, sizeof(previous)) == (int)sizeof(previous) &&
        previous.magic == STATE_MAGIC && previous.version == 5) {
      s_state.magic = STATE_MAGIC;
      s_state.version = STATE_VERSION;
      s_state.wpm = previous.wpm;
      memcpy(s_state.book_id, previous.book_id, sizeof(s_state.book_id));
      s_state.book_revision = previous.book_revision;
      s_state.absolute_word_index = previous.absolute_word_index;
      s_state.furthest_word_index = previous.absolute_word_index;
      s_state.total_word_count = previous.total_word_count;
      s_state.progress_updated_at = previous.progress_updated_at;
    }
  } else if (persist_exists(STATE_KEY) && persist_get_size(STATE_KEY) == (int)sizeof(PersistedStateV4)) {
    PersistedStateV4 previous;
    if (persist_read_data(STATE_KEY, &previous, sizeof(previous)) == (int)sizeof(previous) &&
        previous.magic == STATE_MAGIC && previous.version == 4) {
      s_state.magic = STATE_MAGIC;
      s_state.version = STATE_VERSION;
      s_state.wpm = previous.wpm;
      memcpy(s_state.book_id, previous.book_id, sizeof(s_state.book_id));
      s_state.book_revision = previous.book_revision;
      s_state.absolute_word_index = previous.absolute_word_index;
      s_state.furthest_word_index = previous.absolute_word_index;
      s_state.total_word_count = previous.total_word_count;
      s_state.progress_updated_at = previous.progress_updated_at;
      s_state.total_words_read = previous.total_words_read;
      s_state.reading_seconds = previous.reading_seconds;
      s_state.session_count = previous.session_count;
    }
  } else if (persist_exists(STATE_KEY) && persist_get_size(STATE_KEY) == (int)sizeof(PersistedStateV3)) {
    PersistedStateV3 previous;
    if (persist_read_data(STATE_KEY, &previous, sizeof(previous)) == (int)sizeof(previous) &&
        previous.magic == STATE_MAGIC && previous.version == 3) {
      s_state.magic = STATE_MAGIC;
      s_state.version = STATE_VERSION;
      s_state.wpm = previous.wpm;
      memcpy(s_state.book_id, previous.book_id, sizeof(s_state.book_id));
      s_state.book_revision = previous.book_revision;
      s_state.absolute_word_index = previous.absolute_word_index;
      s_state.furthest_word_index = previous.absolute_word_index;
      s_state.total_word_count = previous.total_word_count;
      s_state.progress_updated_at = previous.progress_updated_at;
    }
  } else if (persist_exists(STATE_KEY) && persist_get_size(STATE_KEY) == (int)sizeof(PersistedStateV2)) {
    PersistedStateV2 previous;
    if (persist_read_data(STATE_KEY, &previous, sizeof(previous)) == (int)sizeof(previous) &&
        previous.magic == STATE_MAGIC && previous.version == 2) {
      s_state.magic = STATE_MAGIC;
      s_state.version = STATE_VERSION;
      s_state.wpm = previous.wpm;
      memcpy(s_state.book_id, previous.book_id, sizeof(s_state.book_id));
      s_state.book_revision = previous.book_revision;
      s_state.absolute_word_index = previous.absolute_word_index;
      s_state.furthest_word_index = previous.absolute_word_index;
      s_state.total_word_count = previous.total_word_count;
      s_state.progress_updated_at = 0;
    }
  }
  if (s_state.wpm < MIN_WPM || s_state.wpm > MAX_WPM) {
    s_state.wpm = DEFAULT_WPM;
  }
  if (s_state.furthest_word_index < s_state.absolute_word_index) {
    s_state.furthest_word_index = s_state.absolute_word_index;
  }
  s_last_persist_sec = time(NULL);
}

static void persist_settings(void) {
  s_settings.magic = SETTINGS_MAGIC;
  s_settings.version = SETTINGS_VERSION;
  s_settings.language = s_language;
  s_settings.library_sort = s_library_sort;
  persist_write_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
}

static uint8_t migrate_legacy_orp_color(uint8_t behavior_flags) {
  const uint8_t packed =
      (behavior_flags & LEGACY_ORP_COLOR_MASK) >> LEGACY_ORP_COLOR_SHIFT;
  if (packed == ORP_COLOR_RED) return ORP_COLOR_RED;
  if (packed == ORP_COLOR_GREEN || packed == (ORP_COLOR_GREEN | 1)) {
    return ORP_COLOR_GREEN;
  }
  // Bit 3 was shared with attention mode, so white/cyan cannot be distinguished
  // from the former default colors. Preserve the intended yellow default.
  return ORP_COLOR_YELLOW;
}

static uint8_t migrate_legacy_behavior_flags(uint8_t behavior_flags) {
  return behavior_flags & (uint8_t)~((1 << 4) | (1 << 5));
}

static void load_settings(void) {
  PersistedSettings loaded;
  if (persist_exists(SETTINGS_KEY) &&
      persist_read_data(SETTINGS_KEY, &loaded, sizeof(loaded)) == (int)sizeof(loaded) &&
      loaded.magic == SETTINGS_MAGIC && loaded.version == SETTINGS_VERSION &&
      loaded.profile <= READER_PROFILE_CUSTOM && loaded.language <= APP_LANGUAGE_GERMAN &&
      loaded.library_sort < LIBRARY_SORT_COUNT && loaded.backlight_mode < BACKLIGHT_MODE_COUNT &&
      loaded.progress_display_mode < PROGRESS_DISPLAY_MODE_COUNT && loaded.sleep_mode < SLEEP_MODE_COUNT &&
      loaded.sleep_duration_index < 5 && loaded.attention_interval_index < 5 &&
      loaded.attention_grace_index < 4 &&
      loaded.attention_confirm_mode < ATTENTION_CONFIRM_COUNT &&
      loaded.sleep_bookmark_timing <= SLEEP_BOOKMARK_END &&
      loaded.attention_bookmark_timing <= SLEEP_BOOKMARK_END &&
      loaded.orp_color < ORP_COLOR_COUNT) {
    s_settings = loaded;
  } else if (persist_exists(SETTINGS_KEY) &&
             persist_get_size(SETTINGS_KEY) == (int)sizeof(PersistedSettingsV10)) {
    PersistedSettingsV10 previous;
    if (persist_read_data(SETTINGS_KEY, &previous, sizeof(previous)) == (int)sizeof(previous) &&
        previous.magic == SETTINGS_MAGIC && previous.version >= 8 && previous.version <= 10 &&
        previous.profile <= READER_PROFILE_CUSTOM && previous.language <= APP_LANGUAGE_GERMAN &&
        previous.library_sort < LIBRARY_SORT_COUNT && previous.backlight_mode < BACKLIGHT_MODE_COUNT &&
        previous.progress_display_mode < PROGRESS_DISPLAY_MODE_COUNT &&
        previous.sleep_mode < SLEEP_MODE_COUNT && previous.sleep_duration_index < 5 &&
        previous.attention_interval_index < 5 && previous.attention_grace_index < 4 &&
        previous.attention_confirm_mode < ATTENTION_CONFIRM_COUNT &&
        previous.sleep_bookmark_timing <= SLEEP_BOOKMARK_END &&
        (previous.version < 10 || previous.attention_bookmark_timing <= SLEEP_BOOKMARK_END)) {
      s_settings.profile = previous.profile;
      s_settings.custom_display_flags = previous.custom_display_flags;
      s_settings.behavior_flags = migrate_legacy_behavior_flags(previous.behavior_flags);
      if (previous.version == 8) {
        s_settings.behavior_flags |= READER_BEHAVIOR_ATTENTION_GUARD;
      }
      s_settings.language = previous.language;
      s_settings.library_sort = previous.library_sort;
      s_settings.backlight_mode = previous.backlight_mode;
      s_settings.progress_display_mode = previous.progress_display_mode;
      s_settings.sleep_mode = previous.sleep_mode;
      s_settings.sleep_duration_index = previous.sleep_duration_index;
      s_settings.attention_interval_index = previous.attention_interval_index;
      s_settings.attention_grace_index = previous.attention_grace_index;
      s_settings.attention_confirm_mode = previous.attention_confirm_mode;
      s_settings.sleep_bookmark_timing = previous.sleep_bookmark_timing;
      s_settings.attention_bookmark_timing = previous.version < 10
          ? SLEEP_BOOKMARK_START : previous.attention_bookmark_timing;
      s_settings.orp_color = migrate_legacy_orp_color(previous.behavior_flags);
    }
  } else if (persist_exists(SETTINGS_KEY) &&
             persist_get_size(SETTINGS_KEY) == (int)sizeof(PersistedSettingsV7)) {
    PersistedSettingsV7 previous;
    if (persist_read_data(SETTINGS_KEY, &previous, sizeof(previous)) == (int)sizeof(previous) &&
        previous.magic == SETTINGS_MAGIC && previous.version == 7 &&
        previous.profile <= READER_PROFILE_CUSTOM && previous.language <= APP_LANGUAGE_GERMAN &&
        previous.library_sort < LIBRARY_SORT_COUNT && previous.backlight_mode < BACKLIGHT_MODE_COUNT &&
        previous.progress_display_mode < PROGRESS_DISPLAY_MODE_COUNT &&
        previous.sleep_mode < SLEEP_MODE_COUNT && previous.sleep_duration_index < 5 &&
        previous.attention_grace_index < 4 &&
        previous.attention_confirm_mode < ATTENTION_CONFIRM_COUNT &&
        previous.sleep_bookmark_timing <= SLEEP_BOOKMARK_END) {
      s_settings.profile = previous.profile;
      s_settings.custom_display_flags = previous.custom_display_flags;
      s_settings.behavior_flags = previous.behavior_flags | READER_BEHAVIOR_ATTENTION_GUARD;
      s_settings.language = previous.language;
      s_settings.library_sort = previous.library_sort;
      s_settings.backlight_mode = previous.backlight_mode;
      s_settings.progress_display_mode = previous.progress_display_mode;
      s_settings.sleep_mode = previous.sleep_mode;
      s_settings.sleep_duration_index = previous.sleep_duration_index;
      s_settings.attention_interval_index = previous.sleep_duration_index == 0 ? 3 : 4;
      s_settings.attention_grace_index = previous.attention_grace_index;
      s_settings.attention_confirm_mode = previous.attention_confirm_mode;
      s_settings.sleep_bookmark_timing = previous.sleep_bookmark_timing;
      s_settings.attention_bookmark_timing = SLEEP_BOOKMARK_START;
      s_settings.orp_color = ORP_COLOR_YELLOW;
    }
  } else if (persist_exists(SETTINGS_KEY) &&
             persist_get_size(SETTINGS_KEY) == (int)sizeof(PersistedSettingsV6)) {
    PersistedSettingsV6 previous;
    if (persist_read_data(SETTINGS_KEY, &previous, sizeof(previous)) == (int)sizeof(previous) &&
        previous.magic == SETTINGS_MAGIC && previous.version >= 4 && previous.version <= 6 &&
        previous.profile <= READER_PROFILE_CUSTOM && previous.language <= APP_LANGUAGE_GERMAN &&
        previous.library_sort < LIBRARY_SORT_COUNT && previous.backlight_mode < BACKLIGHT_MODE_COUNT &&
        previous.progress_display_mode < PROGRESS_DISPLAY_MODE_COUNT) {
      s_settings.profile = previous.profile;
      s_settings.custom_display_flags = previous.custom_display_flags;
      s_settings.behavior_flags = previous.behavior_flags | READER_BEHAVIOR_ATTENTION_GUARD |
          READER_BEHAVIOR_CURRENT_SENTENCE_FIRST | READER_BEHAVIOR_PAUSED_CONTEXT;
      s_settings.language = previous.language;
      s_settings.library_sort = previous.library_sort;
      s_settings.backlight_mode = previous.backlight_mode;
      s_settings.progress_display_mode = previous.progress_display_mode;
      s_settings.orp_color = ORP_COLOR_YELLOW;
    }
  } else if (persist_exists(SETTINGS_KEY) &&
             persist_get_size(SETTINGS_KEY) == (int)sizeof(PersistedSettingsV3)) {
    PersistedSettingsV3 previous;
    if (persist_read_data(SETTINGS_KEY, &previous, sizeof(previous)) == (int)sizeof(previous) &&
        previous.magic == SETTINGS_MAGIC && previous.version == 3 &&
        previous.profile <= READER_PROFILE_CUSTOM && previous.language <= APP_LANGUAGE_GERMAN &&
        previous.library_sort < LIBRARY_SORT_COUNT && previous.backlight_mode < BACKLIGHT_MODE_COUNT) {
      s_settings.profile = previous.profile;
      s_settings.custom_display_flags = previous.custom_display_flags;
      s_settings.behavior_flags = previous.behavior_flags | READER_BEHAVIOR_ATTENTION_GUARD;
      s_settings.language = previous.language;
      s_settings.library_sort = previous.library_sort;
      s_settings.backlight_mode = previous.backlight_mode;
      s_settings.progress_display_mode = PROGRESS_DISPLAY_PERCENT;
      s_settings.orp_color = ORP_COLOR_YELLOW;
    }
  } else if (persist_exists(SETTINGS_KEY) &&
             persist_get_size(SETTINGS_KEY) == (int)sizeof(PersistedSettingsV2)) {
    PersistedSettingsV2 previous;
    if (persist_read_data(SETTINGS_KEY, &previous, sizeof(previous)) == (int)sizeof(previous) &&
        previous.magic == SETTINGS_MAGIC && previous.version == 2 &&
        previous.profile <= READER_PROFILE_CUSTOM && previous.language <= APP_LANGUAGE_GERMAN &&
        previous.library_sort < LIBRARY_SORT_COUNT) {
      s_settings.profile = previous.profile;
      s_settings.custom_display_flags = previous.custom_display_flags;
      s_settings.behavior_flags = previous.behavior_flags | READER_BEHAVIOR_ATTENTION_GUARD;
      s_settings.language = previous.language;
      s_settings.library_sort = previous.library_sort;
      s_settings.backlight_mode = BACKLIGHT_SMART;
      s_settings.orp_color = ORP_COLOR_YELLOW;
    }
  } else if (persist_exists(SETTINGS_KEY) &&
             persist_get_size(SETTINGS_KEY) == (int)sizeof(PersistedSettingsV1)) {
    PersistedSettingsV1 previous;
    if (persist_read_data(SETTINGS_KEY, &previous, sizeof(previous)) == (int)sizeof(previous) &&
        previous.magic == SETTINGS_MAGIC && previous.version == 1 &&
        previous.profile <= READER_PROFILE_CUSTOM) {
      s_settings.profile = previous.profile;
      s_settings.custom_display_flags = previous.custom_display_flags;
      s_settings.behavior_flags = previous.behavior_flags | READER_BEHAVIOR_ATTENTION_GUARD;
      s_settings.orp_color = ORP_COLOR_YELLOW;
    }
  }
  s_language = (AppLanguage)s_settings.language;
  s_library_sort = (LibrarySort)s_settings.library_sort;
}

static void persist_bookmarks(void) {
  if (s_bookmarks.count == 0) {
    for (uint8_t chunk_index = 0; chunk_index < BOOKMARK_CHUNK_COUNT; chunk_index += 1) {
      persist_delete(BOOKMARKS_KEY + chunk_index);
    }
    return;
  }

  // Write data chunks first and the manifest chunk last, then remove obsolete chunks.
  for (uint8_t chunk_index = 1; chunk_index < BOOKMARK_CHUNK_COUNT; chunk_index += 1) {
    const uint8_t start = (uint8_t)chunk_index * BOOKMARKS_PER_CHUNK;
    if (start >= s_bookmarks.count) continue;
    PersistedBookmarkChunk chunk = {
      .magic = BOOKMARK_MAGIC,
      .version = BOOKMARK_VERSION,
      .chunk_index = chunk_index,
      .total_count = s_bookmarks.count,
    };
    const uint8_t remaining = s_bookmarks.count - start;
    chunk.count = remaining < BOOKMARKS_PER_CHUNK ? remaining : BOOKMARKS_PER_CHUNK;
    memcpy(chunk.records, &s_bookmarks.records[start], sizeof(BookmarkRecord) * chunk.count);
    persist_write_data(BOOKMARKS_KEY + chunk_index, &chunk, sizeof(chunk));
  }
  PersistedBookmarkChunk first = {
    .magic = BOOKMARK_MAGIC,
    .version = BOOKMARK_VERSION,
    .chunk_index = 0,
    .count = s_bookmarks.count < BOOKMARKS_PER_CHUNK
        ? s_bookmarks.count : BOOKMARKS_PER_CHUNK,
    .total_count = s_bookmarks.count,
  };
  memcpy(first.records, s_bookmarks.records, sizeof(BookmarkRecord) * first.count);
  persist_write_data(BOOKMARKS_KEY, &first, sizeof(first));

  const uint8_t required_chunks =
      (s_bookmarks.count + BOOKMARKS_PER_CHUNK - 1) / BOOKMARKS_PER_CHUNK;
  for (uint8_t chunk_index = required_chunks;
       chunk_index < BOOKMARK_CHUNK_COUNT; chunk_index += 1) {
    persist_delete(BOOKMARKS_KEY + chunk_index);
  }
}

static void sort_bookmarks_newest_first(void) {
  for (uint8_t left = 0; left < s_bookmarks.count; left += 1) {
    for (uint8_t right = left + 1; right < s_bookmarks.count; right += 1) {
      if (s_bookmarks.records[right].created_at > s_bookmarks.records[left].created_at) {
        BookmarkRecord swap = s_bookmarks.records[left];
        s_bookmarks.records[left] = s_bookmarks.records[right];
        s_bookmarks.records[right] = swap;
      }
    }
  }
}

static uint32_t migrated_bookmark_time(uint8_t index, uint8_t count) {
  const time_t now = time(NULL);
  const uint8_t age = count > index ? count - index - 1 : 0;
  return now > age ? (uint32_t)(now - age) : (uint32_t)index + 1;
}

static bool load_bookmark_chunks(void) {
  if (!persist_exists(BOOKMARKS_KEY) ||
      persist_get_size(BOOKMARKS_KEY) != (int)sizeof(PersistedBookmarkChunk)) {
    return false;
  }
  PersistedBookmarkChunk first;
  if (persist_read_data(BOOKMARKS_KEY, &first, sizeof(first)) != (int)sizeof(first) ||
      first.magic != BOOKMARK_MAGIC || first.version != BOOKMARK_VERSION ||
      first.chunk_index != 0 || first.total_count == 0 ||
      first.total_count > BOOKMARK_MAX_COUNT ||
      first.count != (first.total_count < BOOKMARKS_PER_CHUNK
                          ? first.total_count : BOOKMARKS_PER_CHUNK)) {
    return false;
  }

  s_bookmarks.count = first.total_count;
  memcpy(s_bookmarks.records, first.records, sizeof(BookmarkRecord) * first.count);
  for (uint8_t chunk_index = 1;
       chunk_index * BOOKMARKS_PER_CHUNK < first.total_count; chunk_index += 1) {
    PersistedBookmarkChunk chunk;
    const uint8_t start = chunk_index * BOOKMARKS_PER_CHUNK;
    const uint8_t remaining = first.total_count - start;
    const uint8_t expected = remaining < BOOKMARKS_PER_CHUNK ? remaining : BOOKMARKS_PER_CHUNK;
    if (persist_read_data(BOOKMARKS_KEY + chunk_index, &chunk, sizeof(chunk)) !=
            (int)sizeof(chunk) ||
        chunk.magic != BOOKMARK_MAGIC || chunk.version != BOOKMARK_VERSION ||
        chunk.chunk_index != chunk_index || chunk.total_count != first.total_count ||
        chunk.count != expected) {
      memset(&s_bookmarks, 0, sizeof(s_bookmarks));
      return false;
    }
    memcpy(&s_bookmarks.records[start], chunk.records, sizeof(BookmarkRecord) * chunk.count);
  }
  sort_bookmarks_newest_first();
  return true;
}

static void load_bookmarks(void) {
  memset(&s_bookmarks, 0, sizeof(s_bookmarks));
  if (load_bookmark_chunks()) {
    return;
  }

  if (persist_exists(BOOKMARKS_KEY) &&
      persist_get_size(BOOKMARKS_KEY) == (int)sizeof(PersistedBookmarksV3)) {
    PersistedBookmarksV3 previous;
    if (persist_read_data(BOOKMARKS_KEY, &previous, sizeof(previous)) == (int)sizeof(previous) &&
        previous.magic == BOOKMARK_MAGIC && previous.version == 3 && previous.count <= 8) {
      s_bookmarks.count = previous.count;
      for (uint8_t index = 0; index < previous.count; index += 1) {
        memcpy(s_bookmarks.records[index].book_id, previous.records[index].book_id, 16);
        s_bookmarks.records[index].book_revision = previous.records[index].book_revision;
        s_bookmarks.records[index].word_index = previous.records[index].word_index;
        s_bookmarks.records[index].total_word_count = previous.records[index].total_word_count;
        s_bookmarks.records[index].created_at = migrated_bookmark_time(index, previous.count);
        s_bookmarks.records[index].kind = previous.records[index].kind;
      }
      sort_bookmarks_newest_first();
      persist_bookmarks();
    }
  } else if (persist_exists(BOOKMARKS_KEY) &&
             persist_get_size(BOOKMARKS_KEY) == (int)sizeof(PersistedBookmarksV2)) {
    PersistedBookmarksV2 previous;
    if (persist_read_data(BOOKMARKS_KEY, &previous, sizeof(previous)) == (int)sizeof(previous) &&
        previous.magic == BOOKMARK_MAGIC && previous.version == 2 && previous.count <= 8) {
      s_bookmarks.count = previous.count;
      for (uint8_t index = 0; index < previous.count; index += 1) {
        memcpy(s_bookmarks.records[index].book_id, previous.records[index].book_id, 16);
        s_bookmarks.records[index].book_revision = previous.records[index].book_revision;
        s_bookmarks.records[index].word_index = previous.records[index].word_index;
        s_bookmarks.records[index].total_word_count = previous.records[index].total_word_count;
        s_bookmarks.records[index].created_at = migrated_bookmark_time(index, previous.count);
        s_bookmarks.records[index].kind = 0;
      }
      sort_bookmarks_newest_first();
      persist_bookmarks();
    }
  }
}

static uint16_t length_bonus_percent(const char *word) {
  const uint16_t length = word_character_count(word);
  if (length == 0) {
    return 0;
  }

  uint16_t bonus = 0;
  if (length > 6) {
    bonus += (length - 6) * 6;
  }
  if (length > 10) {
    bonus += (length - 10) * 9;
  }
  if (length > 14) {
    bonus += (length - 14) * 12;
  }
  return bonus > 170 ? 170 : bonus;
}

static uint16_t complexity_bonus_percent(const char *word) {
  uint16_t bonus = 0;
  const uint16_t syllables = approximate_syllable_groups(word);
  if (syllables > 2) {
    const uint16_t syllable_bonus = (syllables - 2) * 10;
    bonus += syllable_bonus > 50 ? 50 : syllable_bonus;
  }

  uint16_t letters = 0;
  uint16_t digits = 0;
  uint16_t uppercase = 0;
  const char *cursor = word;
  while (*cursor != 0) {
    const uint32_t codepoint = utf8_next(&cursor);
    if (is_latin_letter(codepoint)) {
      letters += 1;
      if (is_ascii_upper(codepoint)) {
        uppercase += 1;
      }
    } else if (is_digit(codepoint)) {
      digits += 1;
    }
  }

  if (letters > 0 && digits > 0) {
    bonus += 22;
  } else if (digits >= 3) {
    bonus += 10;
  }
  if (uppercase >= 2 && uppercase == letters) {
    bonus += 14;
  }
  return bonus > 85 ? 85 : bonus;
}

static uint16_t punctuation_bonus_ms(uint16_t index) {
  const char *word = word_at(index);
  const char *next = index + 1 < word_count() ? word_at(index + 1) : "";
  const uint32_t trailing = trailing_rhythm_character(word);
  switch (trailing) {
    case ',':
      return (45 * 120) / 100;
    case '-':
      return (60 * 120) / 100;
    case ';':
    case ':':
      return (80 * 120) / 100;
    case '.':
      return word_looks_like_abbreviation(word, starts_with_lowercase_letter(next)) ? 0 : (135 * 180) / 100;
    case '!':
    case '?':
      return (150 * 180) / 100;
    case 0x2026:
      return (135 * 180) / 100;
    default:
      return 0;
  }
}

static uint32_t duration_for_word(uint16_t index) {
  if (s_remote_loaded) {
    const uint32_t scaled = ((uint32_t)s_remote_base_duration[index] * 300UL) / s_state.wpm;
    const uint32_t chapter_pause = s_remote_chapter_start[index] ? 450 : 0;
    uint32_t adaptation_pause = 0;
    if (behavior_enabled(READER_BEHAVIOR_ADAPTIVE_WORDS)) {
      const uint16_t length = word_character_count(word_at(index));
      adaptation_pause = length >= 15 ? 180 : length >= 11 ? 100 : 0;
    }
    const uint32_t duration = scaled + chapter_pause + adaptation_pause;
    return duration < 80 ? 80 : duration;
  }
  const char *word = word_at(index);
  uint32_t duration = 60000UL / s_state.wpm;
  duration += (length_bonus_percent(word) * 80) / 100;
  duration += (complexity_bonus_percent(word) * 70) / 100;
  duration += punctuation_bonus_ms(index);

  if (is_paragraph_end(index)) {
    duration += 220;
  }
  return duration;
}

static uint32_t duration_for_current_segment(void) {
  const uint8_t segments = word_segment_count(word_at(s_word_index));
  if (segments <= 1) {
    return duration_for_word(s_word_index);
  }
  const uint32_t divided = duration_for_word(s_word_index) / segments;
  return divided < 160 ? 160 : divided;
}

static uint8_t focus_ordinal(uint16_t length) {
  if (length <= 1) {
    return 0;
  }
  if (length <= 5) {
    return 1;
  }
  if (length <= 9) {
    return 2;
  }
  if (length <= 13) {
    return 3;
  }
  return 4;
}

static size_t focus_byte_offset(const char *word) {
  size_t offsets[5] = {0};
  uint16_t characters = 0;
  const char *cursor = word;
  while (*cursor != 0) {
    const char *start = cursor;
    const uint32_t codepoint = utf8_next(&cursor);
    if (is_word_character(codepoint)) {
      if (characters < ARRAY_LENGTH(offsets)) {
        offsets[characters] = (size_t)(start - word);
      }
      characters += 1;
    }
  }
  if (characters == 0) {
    return 0;
  }
  return offsets[focus_ordinal(characters) < characters ? focus_ordinal(characters) : characters - 1];
}

static size_t focus_byte_offset_for_index(const char *word, uint8_t display_index) {
  if (display_index == UINT8_MAX) {
    return focus_byte_offset(word);
  }
  const char *cursor = word;
  uint8_t index = 0;
  while (*cursor != 0) {
    const char *start = cursor;
    utf8_next(&cursor);
    if (index == display_index) {
      return (size_t)(start - word);
    }
    index += 1;
  }
  return focus_byte_offset(word);
}

static size_t utf8_char_byte_length(const char *cursor) {
  const uint8_t first = (uint8_t)cursor[0];
  if (first < 0x80) {
    return 1;
  }
  if ((first & 0xe0) == 0xc0) {
    return 2;
  }
  if ((first & 0xf0) == 0xe0) {
    return 3;
  }
  if ((first & 0xf8) == 0xf0) {
    return 4;
  }
  return 1;
}

static void copy_bytes(char *target, size_t target_size, const char *source, size_t start, size_t length) {
  if (target_size == 0) {
    return;
  }
  if (length >= target_size) {
    length = target_size - 1;
  }
  memcpy(target, source + start, length);
  target[length] = 0;
}

static int16_t text_width(GContext *ctx, const char *text, GFont font) {
  const GSize size = graphics_text_layout_get_content_size(
      text, font, GRect(0, 0, 300, 52), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
  return size.w;
}

static void draw_centered_segment(GContext *ctx, const char *text, GFont font, GRect rect) {
  graphics_draw_text(ctx, text, font, rect, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

static GFont word_font_for_level(uint8_t level) {
  switch (level) {
    case 1:
      return fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    case 2:
      return fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    case 3:
      return fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
    default:
      return fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  }
}

static bool word_fits_orp_anchor(GContext *ctx, const char *word, uint8_t focus_index, GFont font, GRect bounds) {
  const size_t focus_start = focus_byte_offset_for_index(word, focus_index);
  const size_t focus_length = utf8_char_byte_length(word + focus_start);
  char before[80];
  char focus[8];
  char after[80];
  copy_bytes(before, sizeof(before), word, 0, focus_start);
  copy_bytes(focus, sizeof(focus), word, focus_start, focus_length);
  copy_bytes(after, sizeof(after), word, focus_start + focus_length,
             strlen(word) - focus_start - focus_length);
  const int16_t focus_width = text_width(ctx, focus, font);
  const int16_t left_width = text_width(ctx, before, font) + focus_width / 2;
  const int16_t right_width = text_width(ctx, after, font) + focus_width / 2;
  const int16_t anchor = (bounds.size.w * 30) / 100;
  return left_width <= anchor - 4 && right_width <= bounds.size.w - anchor - 4;
}

static uint8_t required_word_font_level(GContext *ctx, const char *word, uint8_t focus_index,
                                        GRect bounds) {
  for (uint8_t level = 0; level < 3; level += 1) {
    if (word_fits_orp_anchor(ctx, word, focus_index, word_font_for_level(level), bounds)) {
      return level;
    }
  }
  return 3;
}

static void draw_rsvp_word(GContext *ctx, GRect bounds) {
  if (!s_remote_loaded || word_count() == 0) {
    return;
  }
  char displayed_word[READER_WINDOW_MAX_WORD_BYTES + 1];
  copy_word_segment(word_at(s_word_index), s_word_segment_index, displayed_word, sizeof(displayed_word));
  const char *word = displayed_word;
  const uint8_t focus_index = word_segment_count(word_at(s_word_index)) > 1
      ? UINT8_MAX : s_remote_focus_index[s_word_index];
  GFont word_font = word_font_for_level(required_word_font_level(ctx, word, focus_index, bounds));
  const size_t focus_start = focus_byte_offset_for_index(word, focus_index);
  const size_t focus_length = utf8_char_byte_length(word + focus_start);

  char before[80];
  char focus[8];
  char after[80];
  copy_bytes(before, sizeof(before), word, 0, focus_start);
  copy_bytes(focus, sizeof(focus), word, focus_start, focus_length);
  copy_bytes(after, sizeof(after), word, focus_start + focus_length, strlen(word) - focus_start - focus_length);

  const int16_t before_width = text_width(ctx, before, word_font);
  const int16_t focus_width = text_width(ctx, focus, word_font);
  const int16_t anchor = (bounds.size.w * 30) / 100;
  const int16_t y = 84;
  const int16_t x = anchor - before_width - focus_width / 2;
  const int16_t word_width = before_width + focus_width + text_width(ctx, after, word_font);

  if (!playback_requested() && behavior_enabled(READER_BEHAVIOR_PAUSED_CONTEXT)) {
    const uint32_t absolute = s_remote_start_index + s_word_index;
    const int16_t space_width = text_width(ctx, " ", word_font);
    int16_t left_cursor = x - space_width;
    graphics_context_set_text_color(ctx, GColorDarkGray);
    for (uint8_t distance = 1; distance <= 12; distance += 1) {
      if (absolute < distance) break;
      const uint32_t wanted = absolute - distance;
      const char *candidate = NULL;
      if (wanted >= s_remote_start_index) {
        candidate = s_remote_words[wanted - s_remote_start_index];
      } else if (s_previous_loaded && wanted >= s_previous_start_index &&
                 wanted < s_previous_start_index + s_previous_word_count) {
        candidate = s_previous_words[wanted - s_previous_start_index];
      }
      if (candidate == NULL) break;
      const int16_t candidate_width = text_width(ctx, candidate, word_font);
      const int16_t candidate_x = left_cursor - candidate_width;
      graphics_draw_text(ctx, candidate, word_font,
                         GRect(candidate_x, y, candidate_width + 3, 48),
                         GTextOverflowModeFill, GTextAlignmentLeft, NULL);
      left_cursor = candidate_x - space_width;
      if (candidate_x <= 0) break;
    }

    int16_t right_cursor = x + word_width + space_width;
    for (uint8_t distance = 1; distance <= 12; distance += 1) {
      const uint32_t wanted = absolute + distance;
      const char *candidate = NULL;
      if (wanted < s_remote_start_index + s_remote_word_count) {
        candidate = s_remote_words[wanted - s_remote_start_index];
      } else if (s_pending_loaded && wanted >= s_next_start_index &&
                 wanted < s_next_start_index + s_next_word_count) {
        candidate = s_next_words[wanted - s_next_start_index];
      }
      if (candidate == NULL) break;
      const int16_t candidate_width = text_width(ctx, candidate, word_font);
      graphics_draw_text(ctx, candidate, word_font,
                         GRect(right_cursor, y, candidate_width + 3, 48),
                         GTextOverflowModeFill, GTextAlignmentLeft, NULL);
      right_cursor += candidate_width + space_width;
      if (right_cursor >= bounds.size.w) break;
    }
  }

  if (reader_display_enabled(CUSTOM_SHOW_GUIDES)) {
    graphics_context_set_stroke_color(ctx, GColorDarkGray);
    graphics_draw_line(ctx, GPoint(anchor - 20, y - 10), GPoint(anchor - 4, y - 10));
    graphics_draw_line(ctx, GPoint(anchor + 4, y - 10), GPoint(anchor + 20, y - 10));
    graphics_draw_line(ctx, GPoint(anchor - 20, y + 40), GPoint(anchor - 4, y + 40));
    graphics_draw_line(ctx, GPoint(anchor + 4, y + 40), GPoint(anchor + 20, y + 40));
    graphics_draw_line(ctx, GPoint(anchor, y - 10), GPoint(anchor, y - 4));
    graphics_draw_line(ctx, GPoint(anchor, y + 34), GPoint(anchor, y + 40));
  }

  const bool marker_visible = reader_display_enabled(CUSTOM_HIGHLIGHT_ORP);
  graphics_context_set_text_color(ctx, orp_neighbor_color(marker_visible));
  draw_centered_segment(ctx, before, word_font, GRect(x, y, before_width + 4, 48));
  graphics_context_set_text_color(ctx, marker_visible ? orp_focus_color() : GColorWhite);
  draw_centered_segment(ctx, focus, word_font, GRect(x + before_width, y, focus_width + 4, 48));
  graphics_context_set_text_color(ctx, orp_neighbor_color(marker_visible));
  draw_centered_segment(ctx, after, word_font, GRect(x + before_width + focus_width, y, 220, 48));
}

static bool context_has_contiguous_next_window(void) {
  return s_pending_loaded && s_next_start_index == s_remote_start_index + s_remote_word_count;
}

static uint16_t context_word_count(void) {
  return s_remote_word_count + (context_has_contiguous_next_window() ? s_next_word_count : 0);
}

static const char *context_word_at(uint16_t index) {
  if (index < s_remote_word_count) {
    return s_remote_words[index];
  }
  const uint16_t next_index = index - s_remote_word_count;
  return context_has_contiguous_next_window() && next_index < s_next_word_count
      ? s_next_words[next_index] : "";
}

static bool context_chapter_start_at(uint16_t index) {
  if (index < s_remote_word_count) {
    return s_remote_chapter_start[index];
  }
  const uint16_t next_index = index - s_remote_word_count;
  return context_has_contiguous_next_window() && next_index < s_next_word_count &&
      s_next_chapter_start[next_index];
}

static bool context_paragraph_end_at(uint16_t index) {
  if (index < s_remote_word_count) {
    return s_remote_paragraph_end[index];
  }
  const uint16_t next_index = index - s_remote_word_count;
  return context_has_contiguous_next_window() && next_index < s_next_word_count &&
      s_next_paragraph_end[next_index];
}

static bool layout_context_range(GContext *ctx, GRect text_bounds, uint16_t start, uint16_t end,
                                 bool draw) {
  const GFont regular = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  const GFont bold = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  const int16_t line_height = 17;
  const int16_t space_width = text_width(ctx, " ", regular);
  int16_t x = 0;
  int16_t y = 0;

  for (uint16_t index = start; index < end && index < context_word_count(); index += 1) {
    if (index > start && context_chapter_start_at(index)) {
      x = 0;
      y += line_height * 2;
    } else if (index > start && context_paragraph_end_at(index - 1)) {
      x = 0;
      y += line_height;
    }
    const GFont font = index == start ? bold : regular;
    const char *word = context_word_at(index);
    const int16_t width = text_width(ctx, word, font);
    const int16_t gap = x > 0 ? space_width : 0;
    if (x > 0 && x + gap + width > text_bounds.size.w) {
      x = 0;
      y += line_height;
    }
    if (y + line_height > text_bounds.size.h) {
      return false;
    }
    if (draw) {
      graphics_draw_text(ctx, word, font,
                         GRect(text_bounds.origin.x + x + (x > 0 ? gap : 0),
                               text_bounds.origin.y + y,
                               width > text_bounds.size.w ? text_bounds.size.w : width + 2,
                               line_height + 3),
                         GTextOverflowModeFill, GTextAlignmentLeft, NULL);
    }
    x += gap + width;
  }
  return true;
}

static void draw_context(GContext *ctx, GRect bounds) {
  const GRect text_bounds = GRect(8, 22, bounds.size.w - 16, bounds.size.h - 52);
  const uint16_t available_words = context_word_count();
  if (s_context_page_start >= available_words) {
    s_context_page_start = available_words > 0 ? available_words - 1 : 0;
  }
  if (s_context_find_last_page && available_words > 0) {
    const uint16_t target_end = s_context_last_page_end > 0 && s_context_last_page_end <= available_words
        ? s_context_last_page_end : available_words;
    uint16_t earliest = target_end - 1;
    for (int32_t candidate = (int32_t)target_end - 1; candidate >= 0; candidate -= 1) {
      if (!layout_context_range(ctx, text_bounds, (uint16_t)candidate, target_end, false)) {
        break;
      }
      earliest = (uint16_t)candidate;
    }
    s_context_page_start = earliest;
    s_word_index = earliest;
    s_word_segment_index = 0;
    s_context_find_last_page = false;
  }

  s_context_page_end = s_context_page_start;
  for (uint16_t end = s_context_page_start + 1; end <= available_words; end += 1) {
    if (!layout_context_range(ctx, text_bounds, s_context_page_start, end, false)) break;
    s_context_page_end = end;
  }
  if (s_context_page_end == s_context_page_start && s_context_page_start < available_words) {
    s_context_page_end += 1;
  }
  graphics_context_set_text_color(ctx, GColorWhite);
  layout_context_range(ctx, text_bounds, s_context_page_start, s_context_page_end, true);
}

static void format_reader_progress(uint32_t absolute_index, uint32_t total, char *text,
                                   size_t text_size) {
  if (s_settings.progress_display_mode == PROGRESS_DISPLAY_WORDS) {
    if (total >= 10000) {
      snprintf(text, text_size, "%lu.%luk/%lu.%luk",
               (unsigned long)(absolute_index / 1000),
               (unsigned long)((absolute_index / 100) % 10),
               (unsigned long)(total / 1000), (unsigned long)((total / 100) % 10));
    } else {
      snprintf(text, text_size, "%lu/%lu", (unsigned long)absolute_index,
               (unsigned long)total);
    }
    return;
  }
  if (s_settings.progress_display_mode == PROGRESS_DISPLAY_REMAINING) {
    const uint32_t remaining = total > absolute_index ? total - absolute_index - 1 : 0;
    const uint32_t minutes = s_state.wpm > 0
        ? (remaining + (uint32_t)s_state.wpm - 1) / (uint32_t)s_state.wpm : 0;
    snprintf(text, text_size, "%lu min", (unsigned long)minutes);
    return;
  }
  const uint16_t tenths = progress_tenths(absolute_index, total);
  snprintf(text, text_size, "%u.%u%%", tenths / 10, tenths % 10);
}

static void draw_footer(GContext *ctx, GRect bounds) {
  static char wpm[16];
  static char progress[24];
  const uint32_t absolute_index = s_remote_loaded ? s_remote_start_index + s_word_index : s_word_index;
  const uint32_t total = s_remote_loaded ? s_remote_total_count : word_count();
  snprintf(wpm, sizeof(wpm), "%u WPM", s_state.wpm);
  format_reader_progress(absolute_index, total, progress, sizeof(progress));

  graphics_context_set_text_color(ctx, GColorLightGray);
  if (reader_display_enabled(CUSTOM_SHOW_WPM)) {
    graphics_draw_text(ctx, wpm, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(8, bounds.size.h - 24, bounds.size.w / 2 - 8, 20),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  }
  if (reader_display_enabled(CUSTOM_SHOW_PROGRESS)) {
    graphics_draw_text(ctx, progress, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(bounds.size.w / 2, bounds.size.h - 24, bounds.size.w / 2 - 8, 20),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
  }
}

static void draw_progress_feedback(GContext *ctx, GRect bounds) {
  if (current_millis() >= s_progress_feedback_until_ms || !s_remote_loaded) {
    return;
  }
  static char text[24];
  format_reader_progress(s_remote_start_index + s_word_index, s_remote_total_count,
                         text, sizeof(text));
  const GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  const GSize content = graphics_text_layout_get_content_size(
      text, font, GRect(0, 0, bounds.size.w - 24, 34), GTextOverflowModeTrailingEllipsis,
      GTextAlignmentCenter);
  const int16_t panel_width = content.w + 24 < 84 ? 84 : content.w + 24;
  const GRect panel = GRect((bounds.size.w - panel_width) / 2, 134, panel_width, content.h + 7);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, panel, 4, GCornersAll);
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_draw_round_rect(ctx, panel, 4);
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, text, font,
                     GRect(panel.origin.x + 8, panel.origin.y - 2, panel.size.w - 16, panel.size.h + 3),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void draw_wpm_feedback(GContext *ctx, GRect bounds) {
  if (!s_playing || current_millis() >= s_wpm_feedback_until_ms) {
    return;
  }
  static char text[16];
  snprintf(text, sizeof(text), "%u WPM", s_state.wpm);
  const GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  const GSize content = graphics_text_layout_get_content_size(
      text, font, GRect(0, 0, bounds.size.w - 24, 34), GTextOverflowModeTrailingEllipsis,
      GTextAlignmentCenter);
  const int16_t panel_width = content.w + 24 < 84 ? 84 : content.w + 24;
  const GRect panel = GRect((bounds.size.w - panel_width) / 2, 134, panel_width, content.h + 7);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, panel, 4, GCornersAll);
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_draw_round_rect(ctx, panel, 4);
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, text, font,
                     GRect(panel.origin.x + 8, panel.origin.y - 2, panel.size.w - 16, panel.size.h + 3),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static bool current_position_is_bookmarked(void) {
  if (!s_remote_loaded) {
    return false;
  }
  const uint32_t position = current_absolute_position();
  for (uint8_t index = 0; index < s_bookmarks.count; index += 1) {
    const BookmarkRecord *bookmark = &s_bookmarks.records[index];
    if (bookmark->word_index == position &&
        memcmp(bookmark->book_id, s_remote_book_id, 16) == 0) {
      return true;
    }
  }
  return false;
}

static void draw_bookmark_status(GContext *ctx, GRect bounds) {
  const int16_t left = bounds.size.w - 25;
  const int16_t top = 6;
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(left - 4, top - 3, 20, 25), 2, GCornersAll);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(left, top, 12, 18), 2, GCornersTop);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  for (uint8_t row = 0; row < 6; row += 1) {
    graphics_draw_line(ctx, GPoint(left + 5 - row, top + 12 + row),
                       GPoint(left + 6 + row, top + 12 + row));
  }
}

static bool library_entry_should_swap(uint8_t left, uint8_t right) {
  int text_order = 0;
  switch (s_library_sort) {
    case LIBRARY_SORT_TITLE:
      text_order = strcmp(s_library_titles[left], s_library_titles[right]);
      break;
    case LIBRARY_SORT_AUTHOR:
      text_order = strcmp(s_library_authors[left], s_library_authors[right]);
      if (text_order == 0) {
        text_order = strcmp(s_library_titles[left], s_library_titles[right]);
      }
      break;
    case LIBRARY_SORT_RECENT:
      if (s_library_created_at[left] != s_library_created_at[right]) {
        return s_library_created_at[left] < s_library_created_at[right];
      }
      text_order = strcmp(s_library_titles[left], s_library_titles[right]);
      break;
    case LIBRARY_SORT_PROGRESS: {
      const uint16_t left_progress = progress_tenths(s_library_resume_index[left], s_library_total_count[left]);
      const uint16_t right_progress = progress_tenths(s_library_resume_index[right], s_library_total_count[right]);
      if (left_progress != right_progress) {
        return left_progress < right_progress;
      }
      text_order = strcmp(s_library_titles[left], s_library_titles[right]);
      break;
    }
    default: {
      const bool left_has_progress = s_library_resume_index[left] > 0 || s_library_progress_index[left] > 0;
      const bool right_has_progress = s_library_resume_index[right] > 0 || s_library_progress_index[right] > 0;
      if (left_has_progress != right_has_progress) {
        return !left_has_progress;
      }
      if (s_library_progress_updated_at[left] != s_library_progress_updated_at[right]) {
        return s_library_progress_updated_at[left] < s_library_progress_updated_at[right];
      }
      if (s_library_created_at[left] != s_library_created_at[right]) {
        return s_library_created_at[left] < s_library_created_at[right];
      }
      text_order = strcmp(s_library_titles[left], s_library_titles[right]);
      break;
    }
  }
  return text_order > 0;
}

static void swap_library_entries(uint8_t left, uint8_t right) {
  uint8_t id[16];
  char title[LIBRARY_TITLE_BYTES + 1];
  char author[LIBRARY_AUTHOR_BYTES + 1];
  uint8_t cover[LIBRARY_COVER_BYTES];
  uint8_t palette[LIBRARY_COVER_PALETTE_BYTES];
  memcpy(id, s_library_book_ids[left], sizeof(id));
  memcpy(s_library_book_ids[left], s_library_book_ids[right], sizeof(id));
  memcpy(s_library_book_ids[right], id, sizeof(id));
  memcpy(title, s_library_titles[left], sizeof(title));
  memcpy(s_library_titles[left], s_library_titles[right], sizeof(title));
  memcpy(s_library_titles[right], title, sizeof(title));
  memcpy(author, s_library_authors[left], sizeof(author));
  memcpy(s_library_authors[left], s_library_authors[right], sizeof(author));
  memcpy(s_library_authors[right], author, sizeof(author));
  memcpy(cover, s_library_covers[left], sizeof(cover));
  memcpy(s_library_covers[left], s_library_covers[right], sizeof(cover));
  memcpy(s_library_covers[right], cover, sizeof(cover));
  memcpy(palette, s_library_cover_palettes[left], sizeof(palette));
  memcpy(s_library_cover_palettes[left], s_library_cover_palettes[right], sizeof(palette));
  memcpy(s_library_cover_palettes[right], palette, sizeof(palette));
#define SWAP_LIBRARY_SCALAR(array, type) do { \
  type temporary = array[left]; \
  array[left] = array[right]; \
  array[right] = temporary; \
} while (0)
  SWAP_LIBRARY_SCALAR(s_library_revisions, uint32_t);
  SWAP_LIBRARY_SCALAR(s_library_resume_index, uint32_t);
  SWAP_LIBRARY_SCALAR(s_library_progress_index, uint32_t);
  SWAP_LIBRARY_SCALAR(s_library_total_count, uint32_t);
  SWAP_LIBRARY_SCALAR(s_library_progress_percent, uint8_t);
  SWAP_LIBRARY_SCALAR(s_library_progress_updated_at, uint32_t);
  SWAP_LIBRARY_SCALAR(s_library_created_at, uint32_t);
  SWAP_LIBRARY_SCALAR(s_library_has_cover, bool);
#undef SWAP_LIBRARY_SCALAR
}

static void sort_library_entries(void) {
  if (s_library_count < 2) {
    return;
  }
  uint8_t selected_id[16];
  const uint32_t selected_revision = s_library_revisions[s_library_selected];
  memcpy(selected_id, s_library_book_ids[s_library_selected], sizeof(selected_id));
  for (uint8_t pass = 0; pass + 1 < s_library_count; pass += 1) {
    for (uint8_t index = 0; index + 1 < s_library_count - pass; index += 1) {
      if (library_entry_should_swap(index, index + 1)) {
        swap_library_entries(index, index + 1);
      }
    }
  }
  for (uint8_t index = 0; index < s_library_count; index += 1) {
    if (s_library_revisions[index] == selected_revision &&
        memcmp(s_library_book_ids[index], selected_id, sizeof(selected_id)) == 0) {
      s_library_selected = index;
      break;
    }
  }
}

static bool parse_library(const uint8_t *data, uint16_t length) {
  if (length < LIBRARY_HEADER_BYTES || memcmp(data, "LIBR", 4) != 0 ||
      data[4] != LIBRARY_VERSION || data[6] > APP_LANGUAGE_GERMAN ||
      data[7] >= LIBRARY_SORT_COUNT || s_library_covers == NULL ||
      s_library_cover_palettes == NULL) {
    return false;
  }
  const AppLanguage remote_language = (AppLanguage)data[6];
  const LibrarySort remote_sort = (LibrarySort)data[7];
  if (s_account_settings_pending || s_account_settings_awaiting_confirmation) {
    if (remote_language == s_language && remote_sort == s_library_sort) {
      s_account_settings_awaiting_confirmation = false;
    }
  } else {
    s_language = remote_language;
    s_library_sort = remote_sort;
  }
  const uint8_t count = data[5];
  if (count > LIBRARY_MAX_BOOKS ||
      (uint32_t)LIBRARY_HEADER_BYTES + (uint32_t)count * LIBRARY_ENTRY_BYTES != length) {
    return false;
  }

  for (uint8_t index = 0; index < count; index += 1) {
    const uint8_t *entry = data + LIBRARY_HEADER_BYTES + index * LIBRARY_ENTRY_BYTES;
    const uint8_t title_length = entry[20];
    const uint8_t author_length = entry[69];
    if (title_length > LIBRARY_TITLE_BYTES || author_length > LIBRARY_AUTHOR_BYTES ||
        entry[106] > 100 || (entry[115] != 0 && entry[115] != 1)) {
      return false;
    }
    memcpy(s_library_book_ids[index], entry, sizeof(s_library_book_ids[index]));
    s_library_revisions[index] = read_u32_be(entry + 16);
    memset(s_library_titles[index], 0, sizeof(s_library_titles[index]));
    memcpy(s_library_titles[index], entry + 21, title_length);
    memset(s_library_authors[index], 0, sizeof(s_library_authors[index]));
    memcpy(s_library_authors[index], entry + 70, author_length);
    s_library_resume_index[index] = read_u32_be(entry + 94);
    s_library_progress_index[index] = read_u32_be(entry + 98);
    s_library_total_count[index] = read_u32_be(entry + 102);
    if (s_library_total_count[index] == 0) {
      s_library_resume_index[index] = 0;
      s_library_progress_index[index] = 0;
    } else {
      if (s_library_resume_index[index] >= s_library_total_count[index]) {
        s_library_resume_index[index] = s_library_total_count[index] - 1;
      }
      if (s_library_progress_index[index] >= s_library_total_count[index]) {
        s_library_progress_index[index] = s_library_total_count[index] - 1;
      }
      if (s_library_resume_index[index] > s_library_progress_index[index]) {
        s_library_progress_index[index] = s_library_resume_index[index];
      }
    }
    s_library_progress_percent[index] = s_library_total_count[index] <= 1 ? 0
        : (uint8_t)((s_library_resume_index[index] * 100UL) / (s_library_total_count[index] - 1));
    s_library_progress_updated_at[index] = read_u32_be(entry + 107);
    s_library_created_at[index] = read_u32_be(entry + 111);
    s_library_has_cover[index] = entry[115] != 0;
    memcpy(s_library_cover_palettes[index], entry + 116, LIBRARY_COVER_PALETTE_BYTES);
    memcpy(s_library_covers[index], entry + 132, LIBRARY_COVER_BYTES);
  }
  s_library_count = count;
  if (s_library_selected >= count) {
    s_library_selected = 0;
  }
  sort_library_entries();
  persist_settings();
  bool resume_found = false;
  if (s_state.book_revision > 0) {
    for (uint8_t index = 0; index < count; index += 1) {
      if (s_library_revisions[index] == s_state.book_revision &&
          memcmp(s_library_book_ids[index], s_state.book_id, 16) == 0) {
        s_library_selected = index;
        const bool local_is_newer =
            s_waiting_for_book || s_window_request_sent ||
            (s_state.progress_updated_at > 0 &&
             s_state.progress_updated_at >= s_library_progress_updated_at[index]) ||
            (s_state.progress_updated_at == 0 &&
             s_state.absolute_word_index > s_library_resume_index[index]);
        if (local_is_newer && s_state.absolute_word_index < s_library_total_count[index]) {
          s_library_resume_index[index] = s_state.absolute_word_index;
          if (s_state.furthest_word_index > s_library_progress_index[index]) {
            s_library_progress_index[index] = s_state.furthest_word_index;
          }
          s_library_progress_percent[index] = s_library_total_count[index] <= 1
              ? 0
              : (uint8_t)((s_library_resume_index[index] * 100UL) /
                          (s_library_total_count[index] - 1));
          s_library_progress_updated_at[index] = s_state.progress_updated_at;
          s_progress_pending = true;
        } else if (s_library_resume_index[index] < s_library_total_count[index]) {
          s_state.absolute_word_index = s_library_resume_index[index];
          s_state.furthest_word_index = s_library_progress_index[index];
          s_state.total_word_count = s_library_total_count[index];
          s_state.progress_updated_at = s_library_progress_updated_at[index];
          persist_write_data(STATE_KEY, &s_state, sizeof(s_state));
        }
        resume_found = true;
        break;
      }
    }
  }
  const bool reader_request_active = s_waiting_for_book || s_window_request_sent || s_remote_loaded;
  if (resume_found && reader_request_active) {
    s_resume_request_pending = false;
  } else {
    s_resume_request_pending = resume_found;
    s_gallery_mode = !resume_found;
    set_loading_with_presentation(resume_found, LOADING_PRESENTATION_BOOK);
  }
  sort_library_entries();
  if (s_reader_layer != NULL) {
    layer_mark_dirty(s_reader_layer);
  }
  return true;
}

static bool parse_chapters(const uint8_t *data, uint16_t length) {
  if (length < CHAPTER_HEADER_BYTES || memcmp(data, "CHAP", 4) != 0 || data[4] != 1) {
    return false;
  }
  const uint8_t count = data[5];
  if (count == 0 || count > CHAPTER_MAX_COUNT ||
      (uint32_t)CHAPTER_HEADER_BYTES + (uint32_t)count * CHAPTER_ENTRY_BYTES != length) {
    return false;
  }
  const uint32_t position = s_library_resume_index[s_library_selected];
  s_chapter_selected = 0;
  for (uint8_t index = 0; index < count; index += 1) {
    const uint8_t *entry = data + CHAPTER_HEADER_BYTES + index * CHAPTER_ENTRY_BYTES;
    const uint8_t title_length = entry[4];
    if (title_length > CHAPTER_TITLE_BYTES) {
      return false;
    }
    s_chapter_start_indices[index] = read_u32_be(entry);
    memset(s_chapter_titles[index], 0, sizeof(s_chapter_titles[index]));
    memcpy(s_chapter_titles[index], entry + 5, title_length);
    if (s_chapter_start_indices[index] <= position) {
      s_chapter_selected = index;
    }
  }
  s_chapter_count = count;
  cancel_prefetch_timeout();
  s_window_request_sent = false;
  s_waiting_for_chapters = false;
  s_chapter_mode = true;
  s_gallery_mode = false;
  set_loading(false);
  if (s_reader_layer != NULL) {
    layer_mark_dirty(s_reader_layer);
  }
  return true;
}

static void draw_cover_thumbnail(GContext *ctx, uint8_t index, int16_t left, int16_t top) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(left, top, LIBRARY_COVER_WIDTH, LIBRARY_COVER_HEIGHT), 0, GCornerNone);
  if (!s_library_has_cover[index] || s_library_covers == NULL) {
    graphics_context_set_stroke_color(ctx, GColorLightGray);
    graphics_draw_rect(ctx, GRect(left, top, LIBRARY_COVER_WIDTH, LIBRARY_COVER_HEIGHT));
    return;
  }
  graphics_context_set_fill_color(ctx, GColorBlack);
  for (uint8_t y = 0; y < LIBRARY_COVER_HEIGHT; y += 1) {
    for (uint8_t x = 0; x < LIBRARY_COVER_WIDTH; x += 1) {
      const uint8_t packed = s_library_covers[index][y * LIBRARY_COVER_ROW_BYTES + x / 2];
      const uint8_t palette_index = (packed >> (x % 2 == 0 ? 4 : 0)) & 0x0f;
#ifdef PBL_COLOR
      GColor color = GColorBlack;
      color.argb = s_library_cover_palettes[index][palette_index];
      graphics_context_set_fill_color(ctx, color);
#else
      const uint8_t color = s_library_cover_palettes[index][palette_index];
      const uint8_t luminance = ((color >> 4) & 3) * 3 + ((color >> 2) & 3) * 6 + (color & 3);
      graphics_context_set_fill_color(ctx, luminance < 15 ? GColorBlack : GColorWhite);
#endif
      graphics_fill_rect(ctx, GRect(left + x, top + y, 1, 1), 0, GCornerNone);
    }
  }
}

static void draw_gallery(GContext *ctx, GRect bounds) {
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, localized("Books", "Bücher"), fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(8, 4, bounds.size.w - 16, 24), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
  if (s_library_count == 0) {
    graphics_draw_text(ctx, localized("No books", "Keine Bücher"), fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(8, 76, bounds.size.w - 16, 40), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);
    return;
  }
  uint8_t first = s_library_selected > 0 ? s_library_selected - 1 : 0;
  if (first + 3 > s_library_count) {
    first = s_library_count > 3 ? s_library_count - 3 : 0;
  }
  for (uint8_t row = 0; row < 3 && first + row < s_library_count; row += 1) {
    const uint8_t index = first + row;
    const int16_t top = 29 + row * 63;
    if (index == s_library_selected) {
      graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkGreen, GColorDarkGray));
      graphics_fill_rect(ctx, GRect(4, top - 2, bounds.size.w - 8, 60), 4, GCornersAll);
    }
    draw_cover_thumbnail(ctx, index, 8, top + 1);
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, s_library_titles[index], fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(51, top, bounds.size.w - 59, 20), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);
    graphics_context_set_text_color(ctx, GColorLightGray);
    graphics_draw_text(ctx, s_library_authors[index], fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(51, top + 18, bounds.size.w - 59, 19), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);
    const uint16_t tenths = progress_tenths(s_library_resume_index[index], s_library_total_count[index]);
    const int16_t bar_width = bounds.size.w - 94;
    graphics_context_set_fill_color(ctx, GColorDarkGray);
    graphics_fill_rect(ctx, GRect(51, top + 42, bar_width, 4), 2, GCornersAll);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(51, top + 42, (bar_width * tenths) / 1000, 4),
                       2, GCornersAll);
    static char percent[10];
    snprintf(percent, sizeof(percent), "%u.%u%%", tenths / 10, tenths % 10);
    graphics_context_set_text_color(ctx, GColorLightGray);
    graphics_draw_text(ctx, percent, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(bounds.size.w - 39, top + 33, 34, 18), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentRight, NULL);
  }
}

static void draw_reader_menu(GContext *ctx, GRect bounds) {
  const char *labels[MENU_DESTINATION_COUNT] = {
      localized("Chapters", "Kapitel"), localized("Bookmarks", "Lesezeichen"),
      localized("Library", "Bibliothek"), localized("Settings", "Einstellungen"),
      localized("Display", "Anzeige"), localized("Statistics", "Statistik"),
  };
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, localized("Reading menu", "Lesemenü"), fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(8, 5, bounds.size.w - 16, 24), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
  uint8_t first = s_reader_menu_selected > 2 ? s_reader_menu_selected - 2 : 0;
  if (first + 6 > MENU_DESTINATION_COUNT) first = MENU_DESTINATION_COUNT - 6;
  for (uint8_t visible = 0; visible < 6; visible += 1) {
    const uint8_t index = first + visible;
    const int16_t top = 32 + visible * 32;
    if (index == s_reader_menu_selected) {
      graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkGreen, GColorDarkGray));
      graphics_fill_rect(ctx, GRect(4, top - 1, bounds.size.w - 8, 30), 4, GCornersAll);
    }
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, labels[index], fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(12, top + 4, bounds.size.w - 24, 24), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);
  }
}

static uint8_t sleep_duration_minutes(void) {
  static const uint8_t timer_minutes[5] = {10, 15, 30, 45, 60};
  const uint8_t index = s_settings.sleep_duration_index < 5 ? s_settings.sleep_duration_index : 1;
  return timer_minutes[index];
}

static uint8_t attention_interval_minutes(void) {
  static const uint8_t minutes[5] = {1, 2, 3, 5, 10};
  const uint8_t index = s_settings.attention_interval_index < 5
      ? s_settings.attention_interval_index : 2;
  return minutes[index];
}

static uint8_t attention_grace_seconds(void) {
  static const uint8_t seconds[4] = {15, 30, 45, 60};
  return seconds[s_settings.attention_grace_index < 4 ? s_settings.attention_grace_index : 1];
}

static const char *sleep_mode_name(void) {
  if (s_settings.sleep_mode == SLEEP_MODE_TIMER) return localized("Timer", "Timer");
  if (s_settings.sleep_mode == SLEEP_MODE_ATTENTION) return localized("Attention", "Aufmerksamkeit");
  return localized("Off", "Aus");
}

static const char *attention_confirm_name(void) {
  switch (s_settings.attention_confirm_mode) {
    case ATTENTION_CONFIRM_TOUCH: return localized("Touch", "Berührung");
    case ATTENTION_CONFIRM_BUTTON: return localized("Button", "Taste");
    case ATTENTION_CONFIRM_SHAKE: return localized("Shake", "Schütteln");
    default: return localized("Any", "Beliebig");
  }
}

static uint8_t *selected_bookmark_timing(void) {
  return s_settings.sleep_mode == SLEEP_MODE_ATTENTION
      ? &s_settings.attention_bookmark_timing
      : &s_settings.sleep_bookmark_timing;
}

static void draw_sleep_settings(GContext *ctx, GRect bounds) {
  const char *labels[SLEEP_ROW_COUNT] = {
      localized("Mode", "Modus"),
      s_settings.sleep_mode == SLEEP_MODE_ATTENTION ? localized("Interval", "Intervall")
                                                   : localized("Duration", "Dauer"),
      localized("Confirm window", "Zeitfenster"), localized("Confirm with", "Bestätigen"),
      localized("Bookmark", "Lesezeichen"),
      s_sleep_session_active ? localized("Stop", "Beenden") : localized("Start", "Starten"),
  };
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, localized("Safety & sleep", "Sicherheit & Schlaf"),
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(8, 5, bounds.size.w - 16, 24), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
  for (uint8_t row = 0; row < SLEEP_ROW_COUNT; row += 1) {
    const int16_t top = 32 + row * 32;
    if (row == s_sleep_selected) {
      graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkGreen, GColorDarkGray));
      graphics_fill_rect(ctx, GRect(4, top - 1, bounds.size.w - 8, 30), 4, GCornersAll);
    }
    char value[22] = "";
    if (row == SLEEP_ROW_MODE) snprintf(value, sizeof(value), "%s", sleep_mode_name());
    if (row == SLEEP_ROW_DURATION) snprintf(value, sizeof(value), "%u min",
        s_settings.sleep_mode == SLEEP_MODE_ATTENTION
            ? attention_interval_minutes() : sleep_duration_minutes());
    if (row == SLEEP_ROW_GRACE) snprintf(value, sizeof(value), "%u s", attention_grace_seconds());
    if (row == SLEEP_ROW_CONFIRM) snprintf(value, sizeof(value), "%s", attention_confirm_name());
    if (row == SLEEP_ROW_BOOKMARK) snprintf(value, sizeof(value), "%s",
        *selected_bookmark_timing() == SLEEP_BOOKMARK_START
            ? localized("Start", "Anfang") : localized("End", "Ende"));
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, labels[row], fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(10, top + 2, bounds.size.w - 112, 22), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);
    graphics_context_set_text_color(ctx, GColorLightGray);
    graphics_draw_text(ctx, value, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(bounds.size.w - 99, top + 2, 89, 22), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentRight, NULL);
  }
}

static const char *profile_name(void) {
  switch (s_settings.profile) {
    case READER_PROFILE_INFORMATIVE:
      return localized("Informative", "Informativ");
    case READER_PROFILE_CUSTOM:
      return "Custom";
    default:
      return localized("Focus", "Fokus");
  }
}

static const char *setting_label(uint8_t row) {
  const char *labels[SETTING_ROW_COUNT] = {
      localized("Language", "Sprache"), localized("Library sort", "Sortierung"),
      localized("Speed", "Tempo"), localized("Resume", "Wiedereinstieg"),
      localized("Long words", "Lange Wörter"), "Touch",
      localized("Right swipe", "Rechts wischen"),
      localized("Attention check", "Aufmerksamkeitscheck"),
      localized("Safety & sleep", "Sicherheit & Schlaf"),
      localized("Previous position", "Vorherige Position"),
  };
  return row < SETTING_ROW_COUNT ? labels[row] : "";
}

static const char *library_sort_name(void) {
  switch (s_library_sort) {
    case LIBRARY_SORT_TITLE:
      return localized("Title", "Titel");
    case LIBRARY_SORT_AUTHOR:
      return localized("Author", "Autor");
    case LIBRARY_SORT_RECENT:
      return localized("Added", "Hinzugefügt");
    case LIBRARY_SORT_PROGRESS:
      return localized("Progress", "Fortschritt");
    default:
      return localized("Last read", "Zuletzt gelesen");
  }
}

static const char *on_off(bool value) {
  return value ? localized("On", "Ein") : localized("Off", "Aus");
}

static void setting_value(uint8_t row, char *value, size_t value_size) {
  switch (row) {
    case SETTING_LANGUAGE:
      snprintf(value, value_size, "%s", s_language == APP_LANGUAGE_GERMAN ? "Deutsch" : "English");
      break;
    case SETTING_LIBRARY_SORT:
      snprintf(value, value_size, "%s", library_sort_name());
      break;
    case SETTING_WPM:
      snprintf(value, value_size, "%u", s_state.wpm);
      break;
    case SETTING_RESUME:
      snprintf(value, value_size, "%s", behavior_enabled(READER_BEHAVIOR_SENTENCE_RESUME)
          ? localized("Sentence", "Satz") : localized("Exact", "Exakt"));
      break;
    case SETTING_LONG_WORDS:
      snprintf(value, value_size, "%s", behavior_enabled(READER_BEHAVIOR_ADAPTIVE_WORDS)
          ? localized("Extra time", "Mehr Zeit") : localized("Normal", "Normal"));
      break;
    case SETTING_TOUCH:
      snprintf(value, value_size, "%s", on_off(behavior_enabled(READER_BEHAVIOR_TOUCH)));
      break;
    case SETTING_REWIND:
      snprintf(value, value_size, "%s", behavior_enabled(READER_BEHAVIOR_CURRENT_SENTENCE_FIRST)
          ? localized("Current first", "Aktueller Satz")
          : localized("Previous", "Voriger Satz"));
      break;
    case SETTING_ATTENTION_GUARD:
      snprintf(value, value_size, "%s",
               on_off(behavior_enabled(READER_BEHAVIOR_ATTENTION_GUARD)));
      break;
    case SETTING_SLEEP_MODES:
      snprintf(value, value_size, "%s", sleep_mode_name());
      break;
    case SETTING_RETURN_ORIGIN:
      if (s_origin_return_available) {
        const uint16_t tenths = progress_tenths(s_candidate_origin, s_remote_total_count);
        snprintf(value, value_size, "%u.%u%%", tenths / 10, tenths % 10);
      } else {
        snprintf(value, value_size, "-");
      }
      break;
  }
}

static const char *bookmark_book_title(const BookmarkRecord *bookmark) {
  for (uint8_t index = 0; index < s_library_count; index += 1) {
    if (memcmp(bookmark->book_id, s_library_book_ids[index], 16) == 0) {
      return s_library_titles[index];
    }
  }
  return localized("Unknown book", "Unbekanntes Buch");
}

static void bookmark_created_label(const BookmarkRecord *bookmark, char *target,
                                   size_t target_size) {
  const time_t created = (time_t)bookmark->created_at;
  const time_t now = time(NULL);
  const struct tm *created_local = localtime(&created);
  if (created_local == NULL || bookmark->created_at == 0) {
    snprintf(target, target_size, "%s", localized("Earlier", "Früher"));
    return;
  }
  const struct tm created_parts = *created_local;
  char clock[8];
  strftime(clock, sizeof(clock), "%H:%M", &created_parts);
  const uint32_t created_day = local_day_number(created);
  const uint32_t today = local_day_number(now);
  char date[22];
  if (created_day == today) {
    snprintf(date, sizeof(date), "%s %s", localized("Today", "Heute"), clock);
  } else if (created_day + 1 == today) {
    snprintf(date, sizeof(date), "%s %s", localized("Yesterday", "Gestern"), clock);
  } else if (s_language == APP_LANGUAGE_GERMAN) {
    strftime(date, sizeof(date), "%d.%m.%y  %H:%M", &created_parts);
  } else {
    strftime(date, sizeof(date), "%d %b %y  %H:%M", &created_parts);
  }
  if (bookmark->kind == 1) {
    snprintf(target, target_size, "%s · %s", localized("Sleep", "Schlaf"), date);
  } else {
    snprintf(target, target_size, "%s", date);
  }
}

static void draw_bookmarks(GContext *ctx, GRect bounds) {
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, localized("Bookmarks", "Lesezeichen"),
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(8, 5, bounds.size.w - 16, 24), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
  if (s_bookmarks.count == 0) {
    graphics_context_set_text_color(ctx, GColorLightGray);
    graphics_draw_text(ctx, localized("No bookmarks yet", "Noch keine Lesezeichen"),
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(12, 80, bounds.size.w - 24, 44), GTextOverflowModeWordWrap,
                       GTextAlignmentCenter, NULL);
    return;
  }
  uint8_t first = s_bookmark_selected > 2 ? s_bookmark_selected - 2 : 0;
  if (first + 5 > s_bookmarks.count) first = s_bookmarks.count > 5 ? s_bookmarks.count - 5 : 0;
  for (uint8_t visible = 0; visible < 5 && first + visible < s_bookmarks.count; visible += 1) {
    const uint8_t index = first + visible;
    const BookmarkRecord *bookmark = &s_bookmarks.records[index];
    const int16_t top = 31 + visible * 37;
    if (index == s_bookmark_selected) {
      graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkGreen, GColorDarkGray));
      graphics_fill_rect(ctx, GRect(4, top - 2, bounds.size.w - 8, 34), 4, GCornersAll);
    }
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, bookmark_book_title(bookmark), fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(10, top - 1, bounds.size.w - 66, 19), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);
    static char percent[12];
    const uint16_t tenths = progress_tenths(bookmark->word_index, bookmark->total_word_count);
    snprintf(percent, sizeof(percent), "%u.%u%%", tenths / 10, tenths % 10);
    graphics_context_set_text_color(ctx, GColorLightGray);
    graphics_draw_text(ctx, percent, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(bounds.size.w - 58, top - 1, 50, 19), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentRight, NULL);
    char created[32];
    bookmark_created_label(bookmark, created, sizeof(created));
    graphics_draw_text(ctx, created, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(10, top + 15, bounds.size.w - 20, 18), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);
  }
}

static void draw_bookmark_dialog(GContext *ctx, GRect bounds) {
  if (s_bookmarks.count == 0 || s_bookmark_selected >= s_bookmarks.count) {
    return;
  }
  const BookmarkRecord *bookmark = &s_bookmarks.records[s_bookmark_selected];
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, localized("Bookmark", "Lesezeichen"),
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(8, 3, bounds.size.w - 16, 24), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);
  graphics_context_set_text_color(ctx, GColorLightGray);
  graphics_draw_text(ctx, bookmark_book_title(bookmark), fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(10, 24, bounds.size.w - 20, 20), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);
  static char jump[32];
  const uint16_t tenths = progress_tenths(bookmark->word_index, bookmark->total_word_count);
  snprintf(jump, sizeof(jump), localized("Jump to %u.%u%%", "Zu %u.%u%% springen"),
           tenths / 10, tenths % 10);
  const char *choices[3] = {jump, localized("Cancel", "Abbrechen"), localized("Delete", "Löschen")};
  for (uint8_t index = 0; index < 3; index += 1) {
    const GRect row = GRect(12, 50 + index * 53, bounds.size.w - 24, 39);
    if (index == s_bookmark_dialog_selected) {
      graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkGreen, GColorDarkGray));
      graphics_fill_rect(ctx, row, 4, GCornersAll);
    }
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, choices[index], fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(row.origin.x + 5, row.origin.y + 7, row.size.w - 10, 23),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }
}

static void draw_settings(GContext *ctx, GRect bounds) {
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, localized("Settings", "Einstellungen"), fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(8, 5, bounds.size.w - 16, 24), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
  uint8_t first = s_settings_selected > 2 ? s_settings_selected - 2 : 0;
  if (first + 6 > SETTING_ROW_COUNT) {
    first = SETTING_ROW_COUNT - 6;
  }
  for (uint8_t visible = 0; visible < 6; visible += 1) {
    const uint8_t row = first + visible;
    const int16_t top = 32 + visible * 32;
    if (row == s_settings_selected) {
      graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkGreen, GColorDarkGray));
      graphics_fill_rect(ctx, GRect(4, top - 1, bounds.size.w - 8, 30), 4, GCornersAll);
    }
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, setting_label(row), fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(10, top + 2, bounds.size.w - 118, 22), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);
    char value[24];
    setting_value(row, value, sizeof(value));
    graphics_context_set_text_color(ctx, GColorLightGray);
    graphics_draw_text(ctx, value, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(bounds.size.w - 103, top + 2, 93, 22), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentRight, NULL);
  }
}

static uint8_t display_visible_row_count(void) {
  return s_settings.profile == READER_PROFILE_CUSTOM ? DISPLAY_ROW_COUNT : 4;
}

static uint8_t display_actual_row(uint8_t visible_row) {
  if (s_settings.profile == READER_PROFILE_CUSTOM) {
    const uint8_t custom_rows[DISPLAY_ROW_COUNT] = {
        DISPLAY_PROFILE, DISPLAY_TITLE, DISPLAY_WPM, DISPLAY_PROGRESS,
        DISPLAY_GUIDES, DISPLAY_ORP_VISIBLE, DISPLAY_PAUSED_CONTEXT,
        DISPLAY_ORP_COLOR, DISPLAY_BACKLIGHT,
    };
    return custom_rows[visible_row];
  }
  return visible_row == 0 ? DISPLAY_PROFILE
                          : visible_row == 1 ? DISPLAY_PAUSED_CONTEXT
                          : visible_row == 2 ? DISPLAY_ORP_COLOR : DISPLAY_BACKLIGHT;
}

static const char *display_label(uint8_t row) {
  const char *labels[DISPLAY_ROW_COUNT] = {
      localized("Profile", "Profil"), localized("ORP color", "ORP-Farbe"),
      localized("Title", "Titel"), localized("WPM display", "WPM-Anzeige"),
      localized("Progress", "Fortschritt"), localized("Guides", "Linien"),
      localized("ORP marker", "ORP-Marker"), localized("Backlight", "Beleuchtung"),
      localized("Pause context", "Pausenkontext"),
  };
  return row < DISPLAY_ROW_COUNT ? labels[row] : "";
}

static void display_value(uint8_t row, char *value, size_t value_size) {
  switch (row) {
    case DISPLAY_PROFILE:
      snprintf(value, value_size, "%s", profile_name());
      break;
    case DISPLAY_ORP_COLOR:
      snprintf(value, value_size, "%s", orp_color_name());
      break;
    case DISPLAY_TITLE:
      snprintf(value, value_size, "%s", on_off(custom_display_enabled(CUSTOM_SHOW_TITLE)));
      break;
    case DISPLAY_WPM:
      snprintf(value, value_size, "%s", on_off(custom_display_enabled(CUSTOM_SHOW_WPM)));
      break;
    case DISPLAY_PROGRESS:
      snprintf(value, value_size, "%s", on_off(custom_display_enabled(CUSTOM_SHOW_PROGRESS)));
      break;
    case DISPLAY_GUIDES:
      snprintf(value, value_size, "%s", on_off(custom_display_enabled(CUSTOM_SHOW_GUIDES)));
      break;
    case DISPLAY_ORP_VISIBLE:
      snprintf(value, value_size, "%s", on_off(custom_display_enabled(CUSTOM_HIGHLIGHT_ORP)));
      break;
    case DISPLAY_BACKLIGHT:
      snprintf(value, value_size, "%s", s_settings.backlight_mode == BACKLIGHT_ALWAYS
          ? localized("Always", "Immer")
          : s_settings.backlight_mode == BACKLIGHT_SYSTEM
              ? "System" : localized("Smart", "Intelligent"));
      break;
    case DISPLAY_PAUSED_CONTEXT:
      snprintf(value, value_size, "%s", on_off(behavior_enabled(READER_BEHAVIOR_PAUSED_CONTEXT)));
      break;
  }
}

static void draw_display_settings(GContext *ctx, GRect bounds) {
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, localized("Active display", "Anzeige aktiv"), fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(8, 5, bounds.size.w - 16, 24), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
  const uint8_t row_count = display_visible_row_count();
  const uint8_t visible_count = row_count < 6 ? row_count : 6;
  if (s_display_selected >= row_count) {
    s_display_selected = row_count - 1;
  }
  uint8_t first = s_display_selected > 2 ? s_display_selected - 2 : 0;
  if (first + visible_count > row_count) {
    first = row_count - visible_count;
  }
  for (uint8_t visible = 0; visible < visible_count; visible += 1) {
    const uint8_t visible_row = first + visible;
    const uint8_t row = display_actual_row(visible_row);
    const int16_t top = 32 + visible * 32;
    if (visible_row == s_display_selected) {
      graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkGreen, GColorDarkGray));
      graphics_fill_rect(ctx, GRect(4, top - 1, bounds.size.w - 8, 30), 4, GCornersAll);
    }
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, display_label(row), fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(10, top + 2, bounds.size.w - 118, 22), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);
    char value[24];
    display_value(row, value, sizeof(value));
    graphics_context_set_text_color(ctx, GColorLightGray);
    graphics_draw_text(ctx, value, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(bounds.size.w - 103, top + 2, 93, 22), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentRight, NULL);
  }
}

static void format_stat_count(uint32_t count, char *value, size_t value_size) {
  if (count >= 1000000) {
    snprintf(value, value_size, "%lu.%luM", (unsigned long)(count / 1000000),
             (unsigned long)((count / 100000) % 10));
  } else if (count >= 1000) {
    snprintf(value, value_size, "%lu.%luk", (unsigned long)(count / 1000),
             (unsigned long)((count / 100) % 10));
  } else {
    snprintf(value, value_size, "%lu", (unsigned long)count);
  }
}

static void format_stat_duration(uint32_t seconds, char *value, size_t value_size) {
  if (seconds >= 3600) {
    snprintf(value, value_size, "%luh %02lum", (unsigned long)(seconds / 3600),
             (unsigned long)((seconds / 60) % 60));
  } else if (seconds >= 60 || seconds == 0) {
    snprintf(value, value_size, "%lu min", (unsigned long)((seconds + 59) / 60));
  } else {
    snprintf(value, value_size, "%lu s", (unsigned long)seconds);
  }
}

static void draw_statistics(GContext *ctx, GRect bounds) {
  graphics_context_set_text_color(ctx, GColorWhite);
  const char *heading = s_statistics_page == 0
      ? localized("Current book 1/2", "Aktuelles Buch 1/2")
      : localized("Overall 2/2", "Gesamt 2/2");
  graphics_draw_text(ctx, heading, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(8, 5, bounds.size.w - 16, 24), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
  if (s_statistics_page == 1) {
    const char *labels[8] = {
        localized("Active time", "Aktive Zeit"), localized("Words read", "Wörter gelesen"),
        localized("Books begun", "Bücher begonnen"), localized("Books done", "Bücher beendet"),
        localized("Avg session", "Ø Sitzung"), localized("Avg active day", "Ø Lesetag"),
        localized("Streak now/best", "Serie akt./max."), localized("Reading days", "Lesetage"),
    };
    char values[8][20];
    format_stat_duration(s_state.reading_seconds, values[0], sizeof(values[0]));
    format_stat_count(s_state.total_words_read, values[1], sizeof(values[1]));
    snprintf(values[2], sizeof(values[2]), "%u", s_state.books_started);
    snprintf(values[3], sizeof(values[3]), "%u", s_state.books_completed);
    format_stat_duration(s_state.session_count > 0 ? s_state.reading_seconds / s_state.session_count : 0,
                         values[4], sizeof(values[4]));
    format_stat_duration(s_state.active_day_count > 0
                             ? s_state.reading_seconds / s_state.active_day_count : 0,
                         values[5], sizeof(values[5]));
    snprintf(values[6], sizeof(values[6]), "%u/%u %s", effective_current_streak(),
             s_state.longest_streak, localized("d", "T"));
    snprintf(values[7], sizeof(values[7]), "%u", s_state.active_day_count);
    for (uint8_t row = 0; row < 8; row += 1) {
      const int16_t top = 32 + row * 24;
      graphics_context_set_text_color(ctx, GColorLightGray);
      graphics_draw_text(ctx, labels[row], fonts_get_system_font(FONT_KEY_GOTHIC_14),
                         GRect(8, top, bounds.size.w - 82, 20), GTextOverflowModeTrailingEllipsis,
                         GTextAlignmentLeft, NULL);
      graphics_context_set_text_color(ctx, GColorWhite);
      graphics_draw_text(ctx, values[row], fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                         GRect(bounds.size.w - 81, top, 73, 20), GTextOverflowModeTrailingEllipsis,
                         GTextAlignmentRight, NULL);
    }
    return;
  }
  graphics_context_set_text_color(ctx, GColorLightGray);
  graphics_draw_text(ctx, s_remote_loaded ? s_remote_title : "", fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(8, 29, bounds.size.w - 16, 20), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
  const uint32_t absolute = s_state.absolute_word_index;
  const uint32_t total = s_remote_loaded ? s_remote_total_count : s_state.total_word_count;
  const uint16_t tenths = progress_tenths(absolute, total);
  const uint32_t remaining = total > absolute ? total - absolute - 1 : 0;
  const uint32_t stabilized_wpm = s_state.reading_seconds >= 10
      ? (s_state.total_words_read * 60UL) / s_state.reading_seconds : s_state.wpm;
  const uint32_t eta_seconds = stabilized_wpm > 0 ? (remaining * 60UL) / stabilized_wpm : 0;
  const uint32_t session_wpm = s_session_active_ms >= 1000
      ? (uint32_t)(((uint64_t)s_session_words * 60000ULL) / s_session_active_ms) : s_state.wpm;
  const char *labels[6] = {
      localized("Progress", "Fortschritt"), localized("Position", "Position"),
      localized("Time left", "Restzeit"), localized("Session time", "Sitzungszeit"),
      localized("Session words", "Sitzungswörter"), localized("Session speed", "Sitzungstempo"),
  };
  char values[6][20];
  snprintf(values[0], sizeof(values[0]), "%u.%u%%", tenths / 10, tenths % 10);
  char position_text[10];
  char total_text[10];
  format_stat_count(absolute, position_text, sizeof(position_text));
  format_stat_count(total, total_text, sizeof(total_text));
  snprintf(values[1], sizeof(values[1]), "%s/%s", position_text, total_text);
  const uint32_t eta_minutes = (eta_seconds + 59) / 60;
  snprintf(values[2], sizeof(values[2]), "%lu min",
           (unsigned long)(eta_minutes > 9999 ? 9999 : eta_minutes));
  format_stat_duration(s_session_active_ms / 1000, values[3], sizeof(values[3]));
  format_stat_count(s_session_words, values[4], sizeof(values[4]));
  snprintf(values[5], sizeof(values[5]), "%lu WPM", (unsigned long)session_wpm);
  for (uint8_t row = 0; row < 6; row += 1) {
    const int16_t top = 52 + row * 28;
    graphics_context_set_text_color(ctx, GColorLightGray);
    graphics_draw_text(ctx, labels[row], fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(8, top, bounds.size.w - 82, 22), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, values[row], fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(bounds.size.w - 81, top, 73, 22), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentRight, NULL);
  }
}

static void draw_chapters(GContext *ctx, GRect bounds) {
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, localized("Chapters", "Kapitel"), fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(8, 4, bounds.size.w - 16, 24), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
  if (s_chapter_count == 0) {
    return;
  }
  uint8_t first = s_chapter_selected > 2 ? s_chapter_selected - 2 : 0;
  if (first + 5 > s_chapter_count) {
    first = s_chapter_count > 5 ? s_chapter_count - 5 : 0;
  }
  for (uint8_t row = 0; row < 5 && first + row < s_chapter_count; row += 1) {
    const uint8_t index = first + row;
    const int16_t top = 31 + row * 37;
    if (index == s_chapter_selected) {
      graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkGreen, GColorDarkGray));
      graphics_fill_rect(ctx, GRect(4, top - 2, bounds.size.w - 8, 34), 4, GCornersAll);
    }
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, s_chapter_titles[index], fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(12, top, bounds.size.w - 24, 24), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);
  }
}

static void draw_origin_dialog(GContext *ctx, GRect bounds) {
  static char question[96];
  const uint16_t tenths = progress_tenths(s_candidate_origin, s_remote_total_count);
  snprintf(question, sizeof(question),
           localized("Return to the previous reading position at %u.%u%%?",
                     "Zur vorherigen Leseposition bei %u.%u%% zurückkehren?"),
           tenths / 10, tenths % 10);
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, question, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(10, 18, bounds.size.w - 20, 82), GTextOverflowModeWordWrap,
                     GTextAlignmentCenter, NULL);
  const char *choices[2] = {localized("Return", "Zurück"), localized("Stay", "Bleiben")};
  for (uint8_t index = 0; index < 2; index += 1) {
    const GRect row = GRect(18, 112 + index * 43, bounds.size.w - 36, 36);
    if (index == s_origin_dialog_selected) {
      graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkGreen, GColorDarkGray));
      graphics_fill_rect(ctx, row, 4, GCornersAll);
    }
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, choices[index], fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(row.origin.x + 6, row.origin.y + 5, row.size.w - 12, 24),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }
}

static void loading_timer_callback(void *context) {
  s_loading_timer = NULL;
  if (!s_loading_mode) {
    return;
  }
  const bool hard_loading = s_loading_presentation != LOADING_PRESENTATION_CORNER ||
      s_waiting_for_book || s_waiting_for_chapters || !s_remote_loaded;
  if (hard_loading && s_loading_started_sec > 0 &&
      time(NULL) - s_loading_started_sec >= LOADING_TIMEOUT_SECONDS) {
    const bool show_gallery = s_waiting_for_book || s_waiting_for_chapters || !s_remote_loaded;
    s_loading_mode = false;
    s_loading_presentation = LOADING_PRESENTATION_CORNER;
    s_loading_started_sec = 0;
    s_waiting_for_book = false;
    s_waiting_for_chapters = false;
    s_resume_request_pending = false;
    s_pending_book_request = false;
    s_pending_chapter_request = false;
    s_pending_context_navigation = false;
    s_pending_reader_navigation = false;
    s_resume_after_window = false;
    stop_playback_clock();
    cancel_prefetch_timeout();
    s_window_request_sent = false;
    s_request_outbox_active = false;
    s_chapter_mode = false;
    s_gallery_mode = show_gallery;
    update_backlight();
    APP_LOG(APP_LOG_LEVEL_WARNING, "Loading timed out");
    if (s_reader_layer != NULL) {
      layer_mark_dirty(s_reader_layer);
    }
    return;
  }
  s_loading_frame = (s_loading_frame + 1) % 8;
  if (s_reader_layer != NULL) {
    layer_mark_dirty(s_reader_layer);
  }
  s_loading_timer = app_timer_register(240, loading_timer_callback, NULL);
}

static void set_loading(bool loading) {
  set_loading_with_presentation(loading, LOADING_PRESENTATION_CORNER);
}

static void set_loading_with_presentation(bool loading, LoadingPresentation presentation) {
  s_loading_mode = loading;
  s_loading_presentation = loading ? presentation : LOADING_PRESENTATION_CORNER;
  s_loading_started_sec = loading ? time(NULL) : 0;
  if (loading && s_loading_timer == NULL) {
    s_loading_timer = app_timer_register(240, loading_timer_callback, NULL);
  } else if (!loading && s_loading_timer != NULL) {
    app_timer_cancel(s_loading_timer);
    s_loading_timer = NULL;
  }
}

static void draw_spinner(GContext *ctx, GPoint center) {
  static const int8_t outer_x[] = {0, 6, 8, 6, 0, -6, -8, -6};
  static const int8_t outer_y[] = {-8, -6, 0, 6, 8, 6, 0, -6};
  static const int8_t inner_x[] = {0, 3, 4, 3, 0, -3, -4, -3};
  static const int8_t inner_y[] = {-4, -3, 0, 3, 4, 3, 0, -3};
  graphics_context_set_stroke_width(ctx, 2);
  for (uint8_t index = 0; index < 8; index += 1) {
    const uint8_t age = (index + 8 - s_loading_frame) % 8;
    graphics_context_set_stroke_color(
        ctx, age == 0 ? GColorWhite : age <= 2 ? GColorLightGray : GColorDarkGray);
    graphics_draw_line(ctx, GPoint(center.x + inner_x[index], center.y + inner_y[index]),
                       GPoint(center.x + outer_x[index], center.y + outer_y[index]));
  }
  graphics_context_set_stroke_width(ctx, 1);
}

static void draw_loading(GContext *ctx, GRect bounds) {
  draw_spinner(ctx, GPoint(bounds.size.w / 2, bounds.size.h / 2 - 14));
  graphics_context_set_text_color(ctx, GColorLightGray);
  const char *label = s_loading_presentation == LOADING_PRESENTATION_CHAPTER
                          ? localized("Loading chapter", "Kapitel wird geladen")
                          : localized("Loading book", "Buch wird geladen");
  graphics_draw_text(ctx, label, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(8, bounds.size.h / 2 + 8, bounds.size.w - 16, 22),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void draw_corner_loading(GContext *ctx, GRect bounds) {
  const GPoint center = GPoint(bounds.size.w - 13, 13);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, 11);
  draw_spinner(ctx, center);
}

static bool attention_status_visible(void) {
  return monitor_active() && !s_attention_prompt && !s_sleep_settling &&
      active_monitor_mode() == SLEEP_MODE_ATTENTION && !playback_requested() &&
      !s_context_mode;
}

static void draw_attention_shield(GContext *ctx, GRect bounds) {
  const int16_t right = bounds.size.w - (current_position_is_bookmarked() ? 32 : 8);
  const GPoint points[] = {
      GPoint(right - 6, 7), GPoint(right - 1, 5), GPoint(right + 4, 7),
      GPoint(right + 4, 12), GPoint(right + 3, 16), GPoint(right - 1, 19),
      GPoint(right - 5, 16), GPoint(right - 6, 12), GPoint(right - 6, 7),
  };
  graphics_context_set_stroke_color(ctx, GColorLightGray);
  for (uint8_t index = 1; index < ARRAY_LENGTH(points); index += 1) {
    graphics_draw_line(ctx, points[index - 1], points[index]);
  }
}

static void draw_sleep_status(GContext *ctx, GRect bounds) {
  if (!monitor_active()) return;
  const SleepMode active_mode = active_monitor_mode();
  if (!s_attention_prompt && !s_sleep_settling &&
      active_mode == SLEEP_MODE_ATTENTION) {
    if (attention_status_visible()) draw_attention_shield(ctx, bounds);
    return;
  }
  if (playback_requested() && !s_attention_prompt && !s_sleep_settling) return;
  const time_t now = time(NULL);
  const time_t deadline = s_attention_prompt ? s_attention_prompt_deadline : s_sleep_deadline;
  const uint32_t remaining = deadline > now ? (uint32_t)(deadline - now) : 0;
  if (s_attention_prompt || s_sleep_settling) {
    const uint32_t grace = attention_grace_seconds();
    const uint32_t elapsed = remaining < grace ? grace - remaining : 0;
    const GRect dial = GRect(bounds.size.w / 2 - 27, 128, 54, 54);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_circle(ctx, GPoint(bounds.size.w / 2, 155), 30);
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorGreen, GColorWhite));
    uint32_t progress_angle = (uint32_t)(((uint64_t)elapsed * TRIG_MAX_ANGLE) / grace);
    if (s_sleep_settling) {
      const uint32_t settle_elapsed = current_millis() - s_sleep_settle_started_ms;
      progress_angle = settle_elapsed >= 650 ? 0
          : (uint32_t)(((uint64_t)(650 - settle_elapsed) * TRIG_MAX_ANGLE) / 650);
    }
    graphics_fill_radial(ctx, dial, GOvalScaleModeFitCircle, 5, 0, progress_angle);
    static char seconds[8];
    const uint16_t display_remaining = remaining > 999 ? 999 : (uint16_t)remaining;
    if (s_sleep_settling) seconds[0] = 0;
    else snprintf(seconds, sizeof(seconds), "%u", display_remaining);
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, seconds, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                       GRect(dial.origin.x, dial.origin.y + 14, dial.size.w, 28),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    return;
  }
  if (active_mode != SLEEP_MODE_TIMER) return;
  static char timer[12];
  snprintf(timer, sizeof(timer), "%lu:%02lu", (unsigned long)(remaining / 60),
           (unsigned long)(remaining % 60));
  const GFont timer_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  const int16_t timer_width = text_width(ctx, timer, timer_font);
  const int16_t moon_size = 14;
  const int16_t gap = 4;
  const int16_t group_width = moon_size + gap + timer_width;
  const int16_t group_x = bounds.size.w - 8 - group_width;
  const GPoint moon_center = GPoint(group_x + moon_size / 2, 11);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, moon_center, moon_size / 2);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, GPoint(moon_center.x + 4, moon_center.y - 3), moon_size / 2);
  graphics_context_set_text_color(ctx, GColorLightGray);
  graphics_draw_text(ctx, timer, timer_font,
                     GRect(group_x + moon_size + gap, 4, timer_width + 2, 20),
                     GTextOverflowModeFill, GTextAlignmentLeft, NULL);
}

static uint8_t sleep_shortcut_option_count(void) {
  if (!s_sleep_session_active) return 3;
  return s_settings.sleep_mode == SLEEP_MODE_TIMER ? 4 : 3;
}

static const char *sleep_shortcut_label(uint8_t index) {
  if (!s_sleep_session_active) {
    if (index == 0) return localized("Timer", "Timer");
    if (index == 1) return behavior_enabled(READER_BEHAVIOR_ATTENTION_GUARD)
        ? localized("Attention off", "Aufmerksamkeit aus")
        : localized("Attention on", "Aufmerksamkeit ein");
    return localized("Cancel", "Abbrechen");
  }
  if (s_settings.sleep_mode == SLEEP_MODE_TIMER) {
    if (index == 0) return localized("Stop", "Beenden");
    if (index == 1) return localized("Extend", "Verlängern");
    if (index == 2) return localized("Attention", "Aufmerksamkeit");
    return localized("Cancel", "Abbrechen");
  }
  if (index == 0) return localized("Stop", "Beenden");
  if (index == 1) return localized("Timer", "Timer");
  return localized("Cancel", "Abbrechen");
}

static void sleep_shortcut_detail(uint8_t index, char *target, size_t target_size) {
  target[0] = 0;
  if (!s_sleep_session_active) {
    if (index == 0) snprintf(target, target_size, "%u min", sleep_duration_minutes());
    if (index == 1) snprintf(target, target_size, "%u min", attention_interval_minutes());
    return;
  }
  if (s_settings.sleep_mode == SLEEP_MODE_TIMER) {
    if (index == 1) snprintf(target, target_size, "+%u min", sleep_duration_minutes());
    if (index == 2) snprintf(target, target_size, "%u min", attention_interval_minutes());
    return;
  }
  if (index == 1) snprintf(target, target_size, "%u min", sleep_duration_minutes());
}

static void draw_sleep_shortcut(GContext *ctx, GRect bounds) {
  const uint8_t count = sleep_shortcut_option_count();
  const int16_t panel_width = bounds.size.w - 28;
  const int16_t header_height = 42;
  const int16_t row_height = 32;
  const int16_t panel_height = header_height + count * row_height + 8;
  const GRect panel = GRect((bounds.size.w - panel_width) / 2,
                            (bounds.size.h - panel_height) / 2,
                            panel_width, panel_height);

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, panel, 4, GCornersAll);
  graphics_context_set_stroke_color(ctx, GColorLightGray);
  graphics_draw_round_rect(ctx, panel, 4);
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, localized("Sleep mode", "Schlafmodus"),
                     fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(panel.origin.x + 10, panel.origin.y + 4, panel.size.w - 20, 20),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
  graphics_context_set_text_color(ctx, GColorLightGray);
  const char *status = !s_sleep_session_active
      ? (behavior_enabled(READER_BEHAVIOR_ATTENTION_GUARD)
          ? localized("Attention active", "Aufmerksamkeit aktiv")
          : localized("Choose a mode", "Modus wählen"))
      : s_settings.sleep_mode == SLEEP_MODE_TIMER
          ? localized("Timer active", "Timer aktiv")
          : localized("Attention active", "Aufmerksamkeit aktiv");
  graphics_draw_text(ctx, status, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(panel.origin.x + 10, panel.origin.y + 20, panel.size.w - 20, 19),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_draw_line(ctx, GPoint(panel.origin.x + 7, panel.origin.y + header_height - 2),
                     GPoint(panel.origin.x + panel.size.w - 7,
                            panel.origin.y + header_height - 2));

  for (uint8_t index = 0; index < count; index += 1) {
    const GRect row = GRect(panel.origin.x + 5,
                            panel.origin.y + header_height + index * row_height,
                            panel.size.w - 10, row_height - 2);
    const bool selected = index == s_sleep_shortcut_selected;
    if (selected) {
      graphics_context_set_fill_color(ctx, GColorDarkGray);
      graphics_fill_rect(ctx, row, 2, GCornersAll);
      graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorGreen, GColorWhite));
      graphics_fill_rect(ctx, GRect(row.origin.x, row.origin.y + 6, 3, row.size.h - 12),
                         1, GCornersAll);
    }
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, sleep_shortcut_label(index),
                       fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(row.origin.x + 10, row.origin.y + 4, row.size.w - 68, 22),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    char detail[16];
    sleep_shortcut_detail(index, detail, sizeof(detail));
    if (detail[0]) {
      graphics_context_set_text_color(ctx, selected ? GColorWhite : GColorLightGray);
      graphics_draw_text(ctx, detail, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                         GRect(row.origin.x + row.size.w - 60, row.origin.y + 4, 51, 22),
                         GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
    }
  }
}

static void reader_update_proc(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  if (s_sleep_shortcut_mode) {
    draw_sleep_shortcut(ctx, bounds);
    return;
  }
  if (s_origin_dialog_mode) {
    draw_origin_dialog(ctx, bounds);
    return;
  }
  if (s_bookmark_dialog_mode) {
    draw_bookmark_dialog(ctx, bounds);
    return;
  }

  const bool corner_loading = s_loading_mode &&
      s_loading_presentation == LOADING_PRESENTATION_CORNER;
  if (s_loading_mode && !corner_loading) {
    draw_loading(ctx, bounds);
    return;
  }

  if (s_gallery_mode) {
    draw_gallery(ctx, bounds);
    if (corner_loading) {
      draw_corner_loading(ctx, bounds);
    }
    return;
  }

  if (s_chapter_mode) {
    draw_chapters(ctx, bounds);
    if (corner_loading) {
      draw_corner_loading(ctx, bounds);
    }
    return;
  }

  if (s_reader_menu_mode) {
    draw_reader_menu(ctx, bounds);
    if (corner_loading) {
      draw_corner_loading(ctx, bounds);
    }
    return;
  }

  if (s_bookmark_mode) {
    draw_bookmarks(ctx, bounds);
    if (corner_loading) draw_corner_loading(ctx, bounds);
    return;
  }

  if (s_sleep_mode_menu) {
    draw_sleep_settings(ctx, bounds);
    return;
  }

  if (s_settings_mode) {
    draw_settings(ctx, bounds);
    if (corner_loading) {
      draw_corner_loading(ctx, bounds);
    }
    return;
  }

  if (s_display_mode) {
    draw_display_settings(ctx, bounds);
    if (corner_loading) {
      draw_corner_loading(ctx, bounds);
    }
    return;
  }

  if (s_statistics_mode) {
    draw_statistics(ctx, bounds);
    if (corner_loading) {
      draw_corner_loading(ctx, bounds);
    }
    return;
  }

  if (!s_remote_loaded || word_count() == 0) {
    graphics_context_set_text_color(ctx, GColorLightGray);
    graphics_draw_text(ctx, localized("No book loaded", "Kein Buch geladen"), fonts_get_system_font(FONT_KEY_GOTHIC_18),
                       GRect(8, 86, bounds.size.w - 16, 30), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter, NULL);
    if (corner_loading) {
      draw_corner_loading(ctx, bounds);
    }
    return;
  }

  const bool has_bookmarks = current_position_is_bookmarked();
  if (s_remote_loaded && reader_display_enabled(CUSTOM_SHOW_TITLE)) {
    int16_t right_inset = corner_loading || has_bookmarks ? 40 : 16;
    if (monitor_active() && !s_attention_prompt && !s_sleep_settling &&
        !playback_requested()) {
      if (active_monitor_mode() == SLEEP_MODE_TIMER) {
        right_inset = 76;
      } else if (active_monitor_mode() == SLEEP_MODE_ATTENTION && !s_context_mode) {
        right_inset = has_bookmarks ? 58 : 36;
      }
    }
    graphics_context_set_text_color(ctx, GColorLightGray);
    graphics_draw_text(ctx, s_remote_title, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(8, 4, bounds.size.w - right_inset, 20),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);
  }

  if (s_context_mode) {
    draw_context(ctx, bounds);
  } else {
    draw_rsvp_word(ctx, bounds);
  }
  draw_footer(ctx, bounds);
  draw_wpm_feedback(ctx, bounds);
  draw_progress_feedback(ctx, bounds);
  if (has_bookmarks && !corner_loading) {
    draw_bookmark_status(ctx, bounds);
  }
  if (corner_loading) {
    draw_corner_loading(ctx, bounds);
  }
  draw_sleep_status(ctx, bounds);
}

static void schedule_next_word(void);
static void request_window_at(uint32_t start_index, WindowTarget target);
static void request_rewind_sentence(void);
static void request_next_sentence(void);
static void activate_pending_window(void);
static bool activate_previous_window(void);
static void ensure_prefetch(void);
static void store_sleep_bookmark(uint32_t position);
static bool send_progress_packet(void);
static void begin_attention_prompt(void);
static void finish_sleep_session(void);
static void confirm_attention(AttentionConfirmMode source);
static void accel_tap_handler(AccelAxisType axis, int32_t direction);
static void arm_attention_guard(void);
static void sleep_tick_callback(void *context);

static SleepMode active_monitor_mode(void) {
  if (s_attention_prompt) return s_attention_prompt_mode;
  if (s_sleep_session_active) return (SleepMode)s_settings.sleep_mode;
  if (s_attention_guard_active) return SLEEP_MODE_ATTENTION;
  if (s_remote_loaded && behavior_enabled(READER_BEHAVIOR_ATTENTION_GUARD)) {
    return SLEEP_MODE_ATTENTION;
  }
  return SLEEP_MODE_OFF;
}

static bool monitor_active(void) {
  return s_attention_guard_active || s_sleep_session_active || s_attention_prompt ||
         s_sleep_settling ||
         (s_remote_loaded && behavior_enabled(READER_BEHAVIOR_ATTENTION_GUARD));
}

static bool monitor_needs_ticks(void) {
  return active_monitor_mode() == SLEEP_MODE_TIMER || playback_requested() ||
         s_attention_prompt || s_sleep_settling;
}

static void schedule_monitor_tick(uint32_t delay_ms) {
  if (s_sleep_timer != NULL) app_timer_cancel(s_sleep_timer);
  s_sleep_timer = NULL;
  if (monitor_active() && monitor_needs_ticks()) {
    s_sleep_timer = app_timer_register(delay_ms, sleep_tick_callback, NULL);
  }
}

static void arm_attention_guard(void) {
  if (!s_remote_loaded || s_sleep_session_active ||
      !behavior_enabled(READER_BEHAVIOR_ATTENTION_GUARD)) {
    return;
  }
  s_attention_guard_active = true;
  s_attention_stop_pending = false;
  s_sleep_phase_start_position = current_absolute_position();
  s_sleep_deadline = time(NULL) + (time_t)attention_interval_minutes() * 60;
  schedule_monitor_tick(1000);
}

static void set_attention_guard_enabled(bool enabled) {
  if (enabled) {
    s_settings.behavior_flags |= READER_BEHAVIOR_ATTENTION_GUARD;
    if (playback_requested()) arm_attention_guard();
  } else {
    s_settings.behavior_flags &= ~READER_BEHAVIOR_ATTENTION_GUARD;
    s_attention_guard_active = false;
    s_attention_stop_pending = false;
    if (!s_sleep_session_active && s_sleep_timer != NULL) {
      app_timer_cancel(s_sleep_timer);
      s_sleep_timer = NULL;
    }
  }
}

static void timer_callback(void *context) {
  s_timer = NULL;
  if (!s_playing || s_context_mode) {
    return;
  }
  commit_reading_time();
  const uint8_t segment_count = word_segment_count(word_at(s_word_index));
  if (s_word_segment_index + 1 < segment_count) {
    s_word_segment_index += 1;
    layer_mark_dirty(s_reader_layer);
    schedule_next_word();
    return;
  }
  s_word_segment_index = 0;
  record_reading_word();
  if ((s_attention_stop_pending || s_sleep_end_pending) && word_ends_sentence_at(s_word_index)) {
    begin_attention_prompt();
    return;
  }
  if (s_word_index + 1 < word_count()) {
    s_word_index += 1;
    persist_periodically();
    if (s_remote_loaded && s_word_index + 20 >= word_count()) {
      ensure_prefetch();
    }
    layer_mark_dirty(s_reader_layer);
    schedule_next_word();
  } else {
    if (s_pending_loaded) {
      activate_pending_window();
      return;
    }
    s_resume_after_window = s_playing;
    stop_playback_clock();
    update_backlight();
    persist_state();
    ensure_prefetch();
    set_loading_with_presentation(true, LOADING_PRESENTATION_CORNER);
    layer_mark_dirty(s_reader_layer);
  }
}

static void schedule_next_word(void) {
  if (s_timer != NULL) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
  if (s_playing && !s_context_mode && s_remote_loaded && word_count() > 0) {
    s_timer = app_timer_register(duration_for_current_segment(), timer_callback, NULL);
  }
}

static void set_playing(bool playing) {
  if (!s_remote_loaded || s_loading_mode || s_gallery_mode || s_chapter_mode ||
      s_reader_menu_mode || s_settings_mode || s_display_mode || s_statistics_mode || s_bookmark_mode ||
      s_sleep_mode_menu) {
    return;
  }
  const bool was_playing = s_playing;
  if (was_playing) {
    commit_reading_time();
  }
  s_resume_after_window = false;
  s_playing = playing;
  if (playing && !was_playing) {
    if (!s_session_started) {
      s_session_words = 0;
      s_session_active_ms = 0;
      s_session_started = true;
    }
    s_play_tick_ms = current_millis();
    if (!s_sleep_session_active) arm_attention_guard();
    if (active_monitor_mode() == SLEEP_MODE_ATTENTION) {
      s_sleep_deadline = time(NULL) + (time_t)attention_interval_minutes() * 60;
      schedule_monitor_tick(1000);
    }
  } else {
    s_play_tick_ms = 0;
    if (active_monitor_mode() == SLEEP_MODE_ATTENTION && !s_attention_prompt &&
        !s_sleep_settling) {
      s_attention_stop_pending = false;
      s_sleep_deadline = time(NULL) + (time_t)attention_interval_minutes() * 60;
      schedule_monitor_tick(1000);
    }
  }
  update_backlight();
  persist_state();
  schedule_next_word();
  if (playing) {
    ensure_prefetch();
  }
  layer_mark_dirty(s_reader_layer);
}

static void sleep_close_callback(void *context) {
  s_sleep_close_timer = NULL;
  window_stack_pop_all(true);
}

static void finish_sleep_session(void) {
  const SleepMode finished_mode = active_monitor_mode();
  s_attention_stop_pending = false;
  s_sleep_end_pending = false;
  s_attention_prompt = false;
  s_sleep_settling = false;
  const uint8_t bookmark_timing = finished_mode == SLEEP_MODE_ATTENTION
      ? s_settings.attention_bookmark_timing
      : s_settings.sleep_bookmark_timing;
  store_sleep_bookmark(bookmark_timing == SLEEP_BOOKMARK_START
                           ? s_sleep_phase_start_position
                           : current_absolute_position());
  s_sleep_session_active = false;
  s_attention_guard_active = false;
  set_playing(false);
  persist_state();
  s_progress_pending = true;
  send_progress_packet();
  if (finished_mode == SLEEP_MODE_TIMER) vibes_double_pulse();
  if (s_sleep_close_timer == NULL) {
    s_sleep_close_timer = app_timer_register(700, sleep_close_callback, NULL);
  }
}

static void begin_attention_prompt(void) {
  s_attention_prompt_mode = s_attention_stop_pending ? SLEEP_MODE_ATTENTION
                                                     : SLEEP_MODE_TIMER;
  s_attention_stop_pending = false;
  s_sleep_end_pending = false;
  s_sleep_resume_after_settle = playback_requested();
  set_playing(false);
  s_attention_prompt = true;
  s_sleep_settling = false;
  s_attention_prompt_deadline = time(NULL) + attention_grace_seconds();
  if (s_attention_prompt_mode == SLEEP_MODE_TIMER) vibes_short_pulse();
  schedule_monitor_tick(1000);
  if (s_reader_layer != NULL) layer_mark_dirty(s_reader_layer);
}

static void sleep_tick_callback(void *context) {
  s_sleep_timer = NULL;
  if (!monitor_active()) return;
  if (s_sleep_settling) {
    if (current_millis() - s_sleep_settle_started_ms >= 650) {
      s_sleep_settling = false;
      if (s_sleep_resume_after_settle) set_playing(true);
    }
    if (s_reader_layer != NULL) layer_mark_dirty(s_reader_layer);
    schedule_monitor_tick(s_sleep_settling ? 80 : 1000);
    return;
  }
  const time_t now = time(NULL);
  if (s_attention_prompt) {
    if (now >= s_attention_prompt_deadline) {
      finish_sleep_session();
      return;
    }
  } else if (now >= s_sleep_deadline) {
    if (active_monitor_mode() == SLEEP_MODE_ATTENTION) {
      if (!playback_requested()) {
        s_sleep_deadline = now + (time_t)attention_interval_minutes() * 60;
        schedule_monitor_tick(1000);
        return;
      }
      s_attention_stop_pending = true;
    } else {
      s_sleep_end_pending = true;
      if (!playback_requested()) begin_attention_prompt();
    }
  }
  if (s_reader_layer != NULL) layer_mark_dirty(s_reader_layer);
  schedule_monitor_tick(1000);
}

static void start_sleep_session(void) {
  if (!s_remote_loaded || s_settings.sleep_mode == SLEEP_MODE_OFF) return;
  s_attention_guard_active = false;
  s_sleep_session_active = true;
  s_attention_prompt = false;
  s_sleep_settling = false;
  s_attention_stop_pending = false;
  s_sleep_end_pending = false;
  s_sleep_phase_start_position = current_absolute_position();
  const uint8_t phase_minutes = s_settings.sleep_mode == SLEEP_MODE_ATTENTION
      ? attention_interval_minutes() : sleep_duration_minutes();
  s_sleep_deadline = time(NULL) + (time_t)phase_minutes * 60;
  schedule_monitor_tick(1000);
}

static void stop_sleep_session(void) {
  s_sleep_session_active = false;
  s_attention_prompt = false;
  s_sleep_settling = false;
  s_attention_stop_pending = false;
  s_sleep_end_pending = false;
  if (s_sleep_timer != NULL) app_timer_cancel(s_sleep_timer);
  s_sleep_timer = NULL;
  arm_attention_guard();
}

static void close_sleep_shortcut(bool resume) {
  s_sleep_shortcut_mode = false;
  if (resume && s_sleep_shortcut_was_playing) set_playing(true);
  if (s_reader_layer != NULL) layer_mark_dirty(s_reader_layer);
}

static void open_sleep_shortcut(void) {
  if (!s_remote_loaded || s_loading_mode || s_gallery_mode || s_chapter_mode ||
      s_reader_menu_mode || s_settings_mode || s_display_mode || s_statistics_mode ||
      s_bookmark_mode || s_sleep_mode_menu || s_attention_prompt || s_sleep_settling) {
    return;
  }
  s_sleep_shortcut_was_playing = playback_requested();
  set_playing(false);
  s_sleep_shortcut_mode = true;
  s_sleep_shortcut_selected = 0;
  if (s_reader_layer != NULL) layer_mark_dirty(s_reader_layer);
}

static void activate_sleep_shortcut_selection(void) {
  const uint8_t selected = s_sleep_shortcut_selected;
  if (!s_sleep_session_active) {
    if (selected == 0) {
      s_settings.sleep_mode = SLEEP_MODE_TIMER;
      persist_settings();
      start_sleep_session();
      close_sleep_shortcut(true);
    } else if (selected == 1) {
      set_attention_guard_enabled(
          !behavior_enabled(READER_BEHAVIOR_ATTENTION_GUARD));
      persist_settings();
      close_sleep_shortcut(true);
    } else {
      close_sleep_shortcut(true);
    }
    return;
  }

  if (s_settings.sleep_mode == SLEEP_MODE_TIMER) {
    if (selected == 0) {
      stop_sleep_session();
    } else if (selected == 1) {
      s_sleep_deadline += (time_t)sleep_duration_minutes() * 60;
    } else if (selected == 2) {
      stop_sleep_session();
      set_attention_guard_enabled(true);
      persist_settings();
    }
    close_sleep_shortcut(true);
    return;
  }

  if (selected == 0) {
    stop_sleep_session();
  } else if (selected == 1) {
    stop_sleep_session();
    s_settings.sleep_mode = SLEEP_MODE_TIMER;
    persist_settings();
    start_sleep_session();
  }
  close_sleep_shortcut(true);
}

static void confirm_attention(AttentionConfirmMode source) {
  if (!s_attention_prompt) return;
  const AttentionConfirmMode configured = (AttentionConfirmMode)s_settings.attention_confirm_mode;
  if (configured != ATTENTION_CONFIRM_ANY && configured != source) return;
  s_attention_prompt = false;
  s_sleep_settling = true;
  s_sleep_settle_started_ms = current_millis();
  s_sleep_phase_start_position = current_absolute_position();
  const uint8_t phase_minutes = s_attention_prompt_mode == SLEEP_MODE_TIMER
      ? 10 : attention_interval_minutes();
  s_sleep_deadline = time(NULL) + (time_t)phase_minutes * 60;
  schedule_monitor_tick(80);
  if (s_reader_layer != NULL) layer_mark_dirty(s_reader_layer);
}

static void note_attention_activity(void) {
  if (!monitor_active() || active_monitor_mode() != SLEEP_MODE_ATTENTION ||
      s_attention_prompt || s_sleep_settling) {
    return;
  }
  s_sleep_phase_start_position = current_absolute_position();
  s_attention_stop_pending = false;
  s_sleep_deadline = time(NULL) + (time_t)attention_interval_minutes() * 60;
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  if (s_attention_prompt) confirm_attention(ATTENTION_CONFIRM_SHAKE);
  else note_attention_activity();
}

static void begin_major_jump(uint32_t target) {
  s_candidate_active = true;
  s_candidate_origin = s_state.absolute_word_index;
  s_candidate_target = target;
  s_candidate_forward = target > s_candidate_origin;
  s_candidate_active_ms = 0;
  s_candidate_words = 0;
  s_origin_return_available = true;
  s_origin_dialog_mode = false;
  s_origin_dialog_offered = false;
}

static void request_selected_book_at(uint32_t start_index, bool resume_at_sentence,
                                     LoadingPresentation loading_presentation) {
  if (s_library_count == 0 || s_waiting_for_book) {
    return;
  }
  if (!s_pending_book_request) {
    memcpy(s_pending_book_id, s_library_book_ids[s_library_selected], sizeof(s_pending_book_id));
    s_pending_book_revision = s_library_revisions[s_library_selected];
    s_book_request_attempts = 0;
  }
  const bool same_book = s_remote_loaded && s_library_revisions[s_library_selected] == s_remote_revision &&
      memcmp(s_library_book_ids[s_library_selected], s_remote_book_id, 16) == 0;
  if (!same_book && loading_presentation == LOADING_PRESENTATION_BOOK) {
    s_candidate_active = false;
    s_origin_return_available = false;
    s_candidate_active_ms = 0;
    s_candidate_words = 0;
  }
  s_pending_book_start = start_index;
  s_pending_book_sentence_resume = resume_at_sentence;
  s_pending_book_loading_presentation = loading_presentation;
  stop_playback_clock();
  s_resume_after_window = false;
  if (s_timer != NULL) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
  update_backlight();
  persist_state();
  if (s_window_request_sent || s_request_outbox_active) {
    s_pending_book_request = true;
    s_gallery_mode = false;
    s_chapter_mode = false;
    s_reader_menu_mode = false;
    s_settings_mode = false;
    s_display_mode = false;
    s_statistics_mode = false;
    s_bookmark_mode = false;
    stop_playback_clock();
    update_backlight();
    set_loading_with_presentation(true, loading_presentation);
    layer_mark_dirty(s_reader_layer);
    schedule_transport_followup();
    return;
  }
  char request[24];
  snprintf(request, sizeof(request), "%s:%lu", resume_at_sentence ? "resume" : "jump",
           (unsigned long)start_index);
  const uint16_t request_length = (uint16_t)strlen(request);
  const uint32_t request_id = next_request_id();
  memset(s_control_packet, 0, sizeof(s_control_packet));
  memcpy(s_control_packet, "PRSP", 4);
  s_control_packet[4] = PROTOCOL_VERSION;
  s_control_packet[5] = MSG_REQUEST;
  s_control_packet[7] = HEADER_LENGTH;
  write_u32_be(s_control_packet + 8, request_id);
  write_u32_be(s_control_packet + 12, s_pending_book_revision);
  write_u32_be(s_control_packet + 16, start_index);
  write_u16_be(s_control_packet + 22, 1);
  write_u16_be(s_control_packet + 24, request_length);
  memcpy(s_control_packet + 30, s_pending_book_id, 16);
  memcpy(s_control_packet + HEADER_LENGTH, request, request_length);
  write_u32_be(s_control_packet + 26, crc32_bytes(s_control_packet + HEADER_LENGTH, request_length));

  DictionaryIterator *out_iter;
  AppMessageResult result = app_message_outbox_begin(&out_iter);
  if (result != APP_MSG_OK) {
    s_pending_book_request = true;
    set_loading_with_presentation(true, loading_presentation);
    schedule_transport_followup();
    return;
  }
  dict_write_data(out_iter, MESSAGE_KEY_PACKET, s_control_packet, HEADER_LENGTH + request_length);
  result = app_message_outbox_send();
  if (result != APP_MSG_OK) {
    s_pending_book_request = true;
    set_loading_with_presentation(true, loading_presentation);
    schedule_transport_followup();
    return;
  }
  if (s_book_request_attempts < UINT8_MAX) {
    s_book_request_attempts += 1;
  }
  s_request_outbox_active = true;
  s_waiting_for_book = true;
  s_pending_book_request = false;
  s_gallery_mode = false;
  s_chapter_mode = false;
  s_reader_menu_mode = false;
  s_settings_mode = false;
  s_display_mode = false;
  s_statistics_mode = false;
  s_bookmark_mode = false;
  s_resume_request_pending = false;
  s_remote_loaded = false;
  s_pending_loaded = false;
  s_after_loaded = false;
  s_previous_loaded = false;
  expect_response(request_id, start_index, start_index, s_pending_book_id,
                  s_pending_book_revision, WINDOW_TARGET_CURRENT, false);
  s_navigation_request = false;
  stop_playback_clock();
  s_resume_after_window = false;
  update_backlight();
  set_loading_with_presentation(true, loading_presentation);
  layer_mark_dirty(s_reader_layer);
}

static void request_selected_book(LoadingPresentation loading_presentation) {
  const uint32_t start_index = s_library_total_count[s_library_selected] == 0
      ? 0
      : s_library_resume_index[s_library_selected] < s_library_total_count[s_library_selected]
          ? s_library_resume_index[s_library_selected]
          : s_library_total_count[s_library_selected] - 1;
  request_selected_book_at(start_index, behavior_enabled(READER_BEHAVIOR_SENTENCE_RESUME),
                           loading_presentation);
}

static void request_library_sync(void) {
  if (s_request_outbox_active || s_window_request_sent) {
    s_library_sync_pending = true;
    return;
  }
  const char *request = "sync";
  const uint16_t request_length = 4;
  memset(s_control_packet, 0, sizeof(s_control_packet));
  memcpy(s_control_packet, "PRSP", 4);
  s_control_packet[4] = PROTOCOL_VERSION;
  s_control_packet[5] = MSG_REQUEST;
  s_control_packet[7] = HEADER_LENGTH;
  write_u32_be(s_control_packet + 8, next_request_id());
  write_u16_be(s_control_packet + 22, 1);
  write_u16_be(s_control_packet + 24, request_length);
  memcpy(s_control_packet + HEADER_LENGTH, request, request_length);
  write_u32_be(s_control_packet + 26,
               crc32_bytes(s_control_packet + HEADER_LENGTH, request_length));

  DictionaryIterator *out_iter;
  if (app_message_outbox_begin(&out_iter) != APP_MSG_OK) return;
  dict_write_data(out_iter, MESSAGE_KEY_PACKET, s_control_packet,
                  HEADER_LENGTH + request_length);
  if (app_message_outbox_send() == APP_MSG_OK) {
    s_request_outbox_active = true;
    s_library_sync_outbox_active = true;
    s_library_sync_pending = false;
  }
}

static void request_selected_chapters(void) {
  if (s_library_count == 0 || s_waiting_for_chapters) {
    return;
  }
  if (s_window_request_sent || s_request_outbox_active) {
    s_pending_chapter_request = true;
    s_reader_menu_mode = false;
    s_gallery_mode = false;
    set_loading_with_presentation(true, LOADING_PRESENTATION_CHAPTER);
    layer_mark_dirty(s_reader_layer);
    return;
  }
  const char *request = "chapters";
  const uint16_t request_length = 8;
  const uint32_t request_id = next_request_id();
  memset(s_control_packet, 0, sizeof(s_control_packet));
  memcpy(s_control_packet, "PRSP", 4);
  s_control_packet[4] = PROTOCOL_VERSION;
  s_control_packet[5] = MSG_REQUEST;
  s_control_packet[7] = HEADER_LENGTH;
  write_u32_be(s_control_packet + 8, request_id);
  write_u32_be(s_control_packet + 12, s_library_revisions[s_library_selected]);
  write_u16_be(s_control_packet + 22, 1);
  write_u16_be(s_control_packet + 24, request_length);
  memcpy(s_control_packet + 30, s_library_book_ids[s_library_selected], 16);
  memcpy(s_control_packet + HEADER_LENGTH, request, request_length);
  write_u32_be(s_control_packet + 26, crc32_bytes(s_control_packet + HEADER_LENGTH, request_length));

  DictionaryIterator *out_iter;
  if (app_message_outbox_begin(&out_iter) != APP_MSG_OK) {
    return;
  }
  dict_write_data(out_iter, MESSAGE_KEY_PACKET, s_control_packet, HEADER_LENGTH + request_length);
  if (app_message_outbox_send() != APP_MSG_OK) {
    return;
  }
  s_request_outbox_active = true;
  expect_response(request_id, 0, 0, s_library_book_ids[s_library_selected],
                  s_library_revisions[s_library_selected], WINDOW_TARGET_CURRENT, false);
  s_waiting_for_chapters = true;
  s_pending_chapter_request = false;
  s_gallery_mode = false;
  s_reader_menu_mode = false;
  set_loading_with_presentation(true, LOADING_PRESENTATION_CHAPTER);
  layer_mark_dirty(s_reader_layer);
}

static void toggle_context_mode(void) {
  if (!s_remote_loaded || s_loading_mode) {
    return;
  }
  s_context_mode = !s_context_mode;
  if (s_context_mode) {
    s_context_page_start = s_word_index;
    s_context_page_end = s_word_index + 1;
    s_context_history_count = 0;
    s_context_find_last_page = false;
    s_context_last_page_end = 0;
  } else {
    s_word_index = s_context_page_start < word_count() ? s_context_page_start : s_word_index;
    s_word_segment_index = 0;
  }
  stop_playback_clock();
  s_resume_after_window = false;
  if (s_timer != NULL) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
  update_backlight();
  persist_state();
  if (s_context_mode) {
    ensure_prefetch();
  }
  layer_mark_dirty(s_reader_layer);
}

static bool show_context_absolute(uint32_t absolute_start) {
  if (absolute_start >= s_remote_start_index &&
      absolute_start < s_remote_start_index + s_remote_word_count) {
    s_context_page_start = (uint16_t)(absolute_start - s_remote_start_index);
  } else if (s_pending_loaded && absolute_start >= s_next_start_index &&
             absolute_start < s_next_start_index + s_next_word_count) {
    s_resume_context_after_window = true;
    activate_pending_window();
    s_context_page_start = (uint16_t)(absolute_start - s_remote_start_index);
  } else if (s_previous_loaded && absolute_start >= s_previous_start_index &&
             absolute_start < s_previous_start_index + s_previous_word_count) {
    if (!activate_previous_window()) return false;
    s_context_page_start = (uint16_t)(absolute_start - s_remote_start_index);
  } else {
    return false;
  }
  s_context_page_end = s_context_page_start + 1;
  s_word_index = s_context_page_start;
  s_word_segment_index = 0;
  return true;
}

static void open_reader_menu(void) {
  if (!s_remote_loaded || s_loading_mode || s_chapter_mode || s_settings_mode || s_display_mode ||
      s_statistics_mode || s_bookmark_mode || s_sleep_mode_menu || s_gallery_mode) {
    return;
  }
  s_context_mode = false;
  s_reader_menu_mode = true;
  s_reader_menu_selected = MENU_CHAPTERS;
  stop_playback_clock();
  s_resume_after_window = false;
  if (s_timer != NULL) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
  update_backlight();
  persist_state();
  layer_mark_dirty(s_reader_layer);
}

static void return_to_navigation_origin(void) {
  const uint32_t origin = s_candidate_origin;
  s_candidate_active = false;
  s_candidate_active_ms = 0;
  s_candidate_words = 0;
  s_origin_return_available = false;
  s_origin_dialog_mode = false;
  confirm_position(origin);
  s_progress_pending = true;
  request_selected_book_at(origin, false, LOADING_PRESENTATION_CORNER);
}

static void remove_bookmark_at(uint8_t index) {
  if (index >= s_bookmarks.count) return;
  s_bookmarks.count -= 1;
  if (index < s_bookmarks.count) {
    memmove(&s_bookmarks.records[index], &s_bookmarks.records[index + 1],
            sizeof(BookmarkRecord) * (s_bookmarks.count - index));
  }
  memset(&s_bookmarks.records[s_bookmarks.count], 0, sizeof(BookmarkRecord));
}

static void delete_bookmark_at(uint8_t index) {
  if (index >= s_bookmarks.count) return;
  remove_bookmark_at(index);
  if (s_bookmark_selected >= s_bookmarks.count && s_bookmark_selected > 0) {
    s_bookmark_selected -= 1;
  }
  persist_bookmarks();
}

static void insert_bookmark_newest(const BookmarkRecord *record) {
  const uint8_t retained = s_bookmarks.count < BOOKMARK_MAX_COUNT
      ? s_bookmarks.count : BOOKMARK_MAX_COUNT - 1;
  if (retained > 0) {
    memmove(&s_bookmarks.records[1], &s_bookmarks.records[0],
            sizeof(BookmarkRecord) * retained);
  }
  s_bookmarks.records[0] = *record;
  s_bookmarks.count = retained + 1;
  s_bookmark_selected = 0;
}

static void store_sleep_bookmark(uint32_t position) {
  if (!s_remote_loaded || s_remote_total_count == 0) return;
  for (uint8_t index = 0; index < s_bookmarks.count; index += 1) {
    const BookmarkRecord *record = &s_bookmarks.records[index];
    if (record->kind == 1 && memcmp(record->book_id, s_remote_book_id, 16) == 0) {
      remove_bookmark_at(index);
      break;
    }
  }
  BookmarkRecord record = {
    .book_revision = s_remote_revision,
    .word_index = position < s_remote_total_count ? position : s_remote_total_count - 1,
    .total_word_count = s_remote_total_count,
    .created_at = (uint32_t)time(NULL),
    .kind = 1,
  };
  memcpy(record.book_id, s_remote_book_id, 16);
  insert_bookmark_newest(&record);
  persist_bookmarks();
}

static void add_bookmark(void) {
  if (!s_remote_loaded || s_loading_mode || s_gallery_mode || s_chapter_mode ||
      s_reader_menu_mode || s_settings_mode || s_display_mode || s_statistics_mode ||
      s_bookmark_mode || s_sleep_mode_menu) {
    return;
  }
  set_playing(false);
  const uint32_t position = current_absolute_position();
  for (uint8_t index = 0; index < s_bookmarks.count; index += 1) {
    const BookmarkRecord *record = &s_bookmarks.records[index];
    if (record->kind == 0 && record->word_index == position &&
        memcmp(record->book_id, s_remote_book_id, 16) == 0) {
      delete_bookmark_at(index);
      vibes_double_pulse();
      layer_mark_dirty(s_reader_layer);
      return;
    }
    if (record->kind == 0 && memcmp(record->book_id, s_remote_book_id, 16) == 0 &&
        record->word_index > position - (position > 20 ? 20 : position) &&
        record->word_index < position + 20) {
      remove_bookmark_at(index);
      break;
    }
  }
  BookmarkRecord record = {
    .book_revision = s_remote_revision,
    .word_index = position,
    .total_word_count = s_remote_total_count,
    .created_at = (uint32_t)time(NULL),
    .kind = 0,
  };
  memcpy(record.book_id, s_remote_book_id, 16);
  insert_bookmark_newest(&record);
  persist_bookmarks();
  vibes_short_pulse();
  layer_mark_dirty(s_reader_layer);
}

static void open_selected_bookmark(void) {
  if (s_bookmarks.count == 0 || s_bookmark_selected >= s_bookmarks.count) return;
  const BookmarkRecord *bookmark = &s_bookmarks.records[s_bookmark_selected];
  for (uint8_t index = 0; index < s_library_count; index += 1) {
    if (memcmp(bookmark->book_id, s_library_book_ids[index], 16) == 0) {
      const bool same_book = s_remote_loaded && memcmp(bookmark->book_id, s_remote_book_id, 16) == 0;
      s_library_selected = index;
      const uint32_t target = s_library_total_count[index] > 0
          ? (bookmark->word_index < s_library_total_count[index]
              ? bookmark->word_index : s_library_total_count[index] - 1) : 0;
      if (same_book) begin_major_jump(target);
      s_bookmark_mode = false;
      s_bookmark_dialog_mode = false;
      request_selected_book_at(target, false, LOADING_PRESENTATION_CORNER);
      return;
    }
  }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  note_attention_activity();
  if (s_sleep_shortcut_mode) {
    activate_sleep_shortcut_selection();
    return;
  }
  if (s_attention_prompt) {
    confirm_attention(s_touch_dispatching ? ATTENTION_CONFIRM_TOUCH : ATTENTION_CONFIRM_BUTTON);
    return;
  }
  if (s_bookmark_dialog_mode) {
    if (s_bookmark_dialog_selected == 0) {
      open_selected_bookmark();
    } else if (s_bookmark_dialog_selected == 1) {
      s_bookmark_dialog_mode = false;
      layer_mark_dirty(s_reader_layer);
    } else {
      delete_bookmark_at(s_bookmark_selected);
      s_bookmark_dialog_mode = false;
      layer_mark_dirty(s_reader_layer);
    }
    return;
  }
  if (s_origin_dialog_mode) {
    if (s_origin_dialog_selected == 0) {
      return_to_navigation_origin();
    } else {
      s_origin_dialog_mode = false;
      layer_mark_dirty(s_reader_layer);
    }
    return;
  }
  if (s_bookmark_mode) {
    if (s_bookmarks.count > 0) {
      s_bookmark_dialog_mode = true;
      s_bookmark_dialog_selected = 0;
      layer_mark_dirty(s_reader_layer);
    }
    return;
  }
  if (s_sleep_mode_menu) {
    switch (s_sleep_selected) {
      case SLEEP_ROW_MODE:
        s_settings.sleep_mode = (s_settings.sleep_mode + 1) % SLEEP_MODE_COUNT;
        break;
      case SLEEP_ROW_DURATION:
        if (s_settings.sleep_mode == SLEEP_MODE_ATTENTION) {
          s_settings.attention_interval_index = (s_settings.attention_interval_index + 1) % 5;
        } else {
          s_settings.sleep_duration_index = (s_settings.sleep_duration_index + 1) % 5;
        }
        break;
      case SLEEP_ROW_GRACE:
        s_settings.attention_grace_index = (s_settings.attention_grace_index + 1) % 4;
        break;
      case SLEEP_ROW_CONFIRM:
        s_settings.attention_confirm_mode =
            (s_settings.attention_confirm_mode + 1) % ATTENTION_CONFIRM_COUNT;
        break;
      case SLEEP_ROW_BOOKMARK:
        *selected_bookmark_timing() = *selected_bookmark_timing() == SLEEP_BOOKMARK_START
            ? SLEEP_BOOKMARK_END : SLEEP_BOOKMARK_START;
        break;
      case SLEEP_ROW_START:
        if (s_sleep_session_active) stop_sleep_session();
        else start_sleep_session();
        s_sleep_mode_menu = false;
        persist_settings();
        layer_mark_dirty(s_reader_layer);
        return;
    }
    persist_settings();
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_settings_mode) {
    switch (s_settings_selected) {
      case SETTING_LANGUAGE:
        s_language = s_language == APP_LANGUAGE_ENGLISH ? APP_LANGUAGE_GERMAN : APP_LANGUAGE_ENGLISH;
        s_account_settings_pending = true;
        s_account_settings_awaiting_confirmation = false;
        schedule_transport_followup();
        break;
      case SETTING_LIBRARY_SORT:
        s_library_sort = (LibrarySort)((s_library_sort + 1) % LIBRARY_SORT_COUNT);
        sort_library_entries();
        s_account_settings_pending = true;
        s_account_settings_awaiting_confirmation = false;
        schedule_transport_followup();
        break;
      case SETTING_WPM:
        s_state.wpm = s_state.wpm >= MAX_WPM ? MIN_WPM
                                             : (uint16_t)(s_state.wpm + (s_state.wpm < LOW_WPM_MAX
                                                                            ? LOW_WPM_STEP
                                                                            : HIGH_WPM_STEP));
        if (s_state.wpm > MAX_WPM) {
          s_state.wpm = MAX_WPM;
        }
        persist_state();
        break;
      case SETTING_RESUME:
        s_settings.behavior_flags ^= READER_BEHAVIOR_SENTENCE_RESUME;
        break;
      case SETTING_LONG_WORDS:
        s_settings.behavior_flags ^= READER_BEHAVIOR_ADAPTIVE_WORDS;
        break;
      case SETTING_TOUCH:
        s_settings.behavior_flags ^= READER_BEHAVIOR_TOUCH;
        apply_touch_setting();
        break;
      case SETTING_REWIND:
        s_settings.behavior_flags ^= READER_BEHAVIOR_CURRENT_SENTENCE_FIRST;
        break;
      case SETTING_ATTENTION_GUARD:
        set_attention_guard_enabled(
            !behavior_enabled(READER_BEHAVIOR_ATTENTION_GUARD));
        break;
      case SETTING_SLEEP_MODES:
        s_settings_mode = false;
        s_sleep_mode_menu = true;
        s_sleep_selected = SLEEP_ROW_MODE;
        layer_mark_dirty(s_reader_layer);
        return;
      case SETTING_RETURN_ORIGIN:
        if (s_origin_return_available) return_to_navigation_origin();
        return;
    }
    persist_settings();
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_display_mode) {
    switch (display_actual_row(s_display_selected)) {
      case DISPLAY_PROFILE:
        s_settings.profile = (s_settings.profile + 1) % (READER_PROFILE_CUSTOM + 1);
        break;
      case DISPLAY_ORP_COLOR:
        cycle_orp_color();
        break;
      case DISPLAY_TITLE:
        s_settings.custom_display_flags ^= CUSTOM_SHOW_TITLE;
        s_settings.profile = READER_PROFILE_CUSTOM;
        break;
      case DISPLAY_WPM:
        s_settings.custom_display_flags ^= CUSTOM_SHOW_WPM;
        s_settings.profile = READER_PROFILE_CUSTOM;
        break;
      case DISPLAY_PROGRESS:
        s_settings.custom_display_flags ^= CUSTOM_SHOW_PROGRESS;
        s_settings.profile = READER_PROFILE_CUSTOM;
        break;
      case DISPLAY_GUIDES:
        s_settings.custom_display_flags ^= CUSTOM_SHOW_GUIDES;
        s_settings.profile = READER_PROFILE_CUSTOM;
        break;
      case DISPLAY_ORP_VISIBLE:
        s_settings.custom_display_flags ^= CUSTOM_HIGHLIGHT_ORP;
        s_settings.profile = READER_PROFILE_CUSTOM;
        break;
      case DISPLAY_BACKLIGHT:
        s_settings.backlight_mode = (s_settings.backlight_mode + 1) % BACKLIGHT_MODE_COUNT;
        update_backlight();
        break;
      case DISPLAY_PAUSED_CONTEXT:
        s_settings.behavior_flags ^= READER_BEHAVIOR_PAUSED_CONTEXT;
        break;
    }
    persist_settings();
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_statistics_mode) {
    s_statistics_page = (s_statistics_page + 1) % 2;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_reader_menu_mode) {
    switch (s_reader_menu_selected) {
      case MENU_CHAPTERS:
        s_reader_menu_mode = false;
        request_selected_chapters();
        return;
      case MENU_BOOKMARKS:
        s_reader_menu_mode = false;
        s_bookmark_mode = true;
        s_bookmark_selected = 0;
        break;
      case MENU_LIBRARY:
        s_reader_menu_mode = false;
        s_gallery_mode = true;
        request_library_sync();
        break;
      case MENU_SETTINGS:
        s_reader_menu_mode = false;
        s_settings_mode = true;
        break;
      case MENU_DISPLAY:
        s_reader_menu_mode = false;
        s_display_mode = true;
        break;
      case MENU_STATISTICS:
        s_reader_menu_mode = false;
        s_statistics_mode = true;
        s_statistics_page = 0;
        break;
    }
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_chapter_mode) {
    begin_major_jump(s_chapter_start_indices[s_chapter_selected]);
    s_chapter_mode = false;
    request_selected_book_at(s_chapter_start_indices[s_chapter_selected], false,
                             LOADING_PRESENTATION_CHAPTER);
    return;
  }
  if (s_gallery_mode) {
    request_selected_book(LOADING_PRESENTATION_BOOK);
    return;
  }
  if (s_context_mode) {
    s_context_mode = false;
    update_backlight();
    schedule_next_word();
    layer_mark_dirty(s_reader_layer);
    return;
  }
  set_playing(!playback_requested());
}

static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  note_attention_activity();
  if (s_select_button_down && s_up_button_down) {
    if (!s_button_chord_consumed) add_bookmark();
    s_button_chord_consumed = true;
    return;
  }
  if (s_bookmark_mode || s_sleep_mode_menu) {
    return;
  }
  if (s_gallery_mode) {
    request_selected_chapters();
    return;
  }
  if (!s_remote_loaded || s_loading_mode || s_chapter_mode || s_settings_mode || s_display_mode ||
      s_statistics_mode || s_reader_menu_mode || s_sleep_mode_menu) {
    return;
  }
  open_reader_menu();
}

static void up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_up_button_down && s_down_button_down) {
    if (!s_button_chord_consumed) open_sleep_shortcut();
    s_button_chord_consumed = true;
    return;
  }
  if (s_select_button_down && s_up_button_down) {
    if (!s_button_chord_consumed) add_bookmark();
    s_button_chord_consumed = true;
    return;
  }
  if (!s_remote_loaded || s_loading_mode || s_chapter_mode || s_settings_mode || s_display_mode ||
      s_statistics_mode || s_reader_menu_mode || s_gallery_mode || s_sleep_mode_menu) {
    return;
  }
  toggle_context_mode();
}

static void down_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_up_button_down && s_down_button_down) {
    if (!s_button_chord_consumed) open_sleep_shortcut();
    s_button_chord_consumed = true;
    return;
  }
  if (!s_remote_loaded || s_loading_mode || s_gallery_mode || s_chapter_mode ||
      s_reader_menu_mode || s_settings_mode || s_display_mode || s_statistics_mode || s_sleep_mode_menu) {
    return;
  }
  s_settings.progress_display_mode =
      (s_settings.progress_display_mode + 1) % PROGRESS_DISPLAY_MODE_COUNT;
  persist_settings();
  s_progress_feedback_until_ms = current_millis() + PROGRESS_FEEDBACK_MS;
  if (s_feedback_timer != NULL) {
    app_timer_cancel(s_feedback_timer);
  }
  s_feedback_timer = app_timer_register(PROGRESS_FEEDBACK_MS, feedback_timer_callback, NULL);
  layer_mark_dirty(s_reader_layer);
}

static void raw_button_down_handler(ClickRecognizerRef recognizer, void *context) {
  note_attention_activity();
  const ButtonId id = click_recognizer_get_button_id(recognizer);
  if (id == BUTTON_ID_SELECT) s_select_button_down = true;
  if (id == BUTTON_ID_UP) s_up_button_down = true;
  if (id == BUTTON_ID_DOWN) s_down_button_down = true;
}

static void raw_button_up_handler(ClickRecognizerRef recognizer, void *context) {
  const ButtonId id = click_recognizer_get_button_id(recognizer);
  if (id == BUTTON_ID_SELECT) s_select_button_down = false;
  if (id == BUTTON_ID_UP) s_up_button_down = false;
  if (id == BUTTON_ID_DOWN) s_down_button_down = false;
  if (!s_select_button_down && !s_up_button_down && !s_down_button_down) {
    s_button_chord_consumed = false;
  }
}

static void adjust_wpm(int delta) {
  int next = s_state.wpm;
  if (delta > 0) {
    next += next < LOW_WPM_MAX ? LOW_WPM_STEP : HIGH_WPM_STEP;
    if (next > LOW_WPM_MAX && s_state.wpm < LOW_WPM_MAX) {
      next = LOW_WPM_MAX;
    }
  } else {
    next -= next <= LOW_WPM_MAX ? LOW_WPM_STEP : HIGH_WPM_STEP;
    if (next < LOW_WPM_MAX && s_state.wpm > LOW_WPM_MAX) {
      next = LOW_WPM_MAX;
    }
  }
  if (next < MIN_WPM) {
    next = MIN_WPM;
  }
  if (next > MAX_WPM) {
    next = MAX_WPM;
  }
  s_state.wpm = (uint16_t)next;
  if (s_playing && (s_settings.profile == READER_PROFILE_FOCUS ||
                    (s_settings.profile == READER_PROFILE_CUSTOM &&
                     !custom_display_enabled(CUSTOM_SHOW_WPM)))) {
    s_wpm_feedback_until_ms = current_millis() + WPM_FEEDBACK_MS;
    if (s_feedback_timer != NULL) {
      app_timer_cancel(s_feedback_timer);
    }
    s_feedback_timer = app_timer_register(WPM_FEEDBACK_MS, feedback_timer_callback, NULL);
  }
  persist_state();
  schedule_next_word();
  layer_mark_dirty(s_reader_layer);
}

static void feedback_timer_callback(void *context) {
  s_feedback_timer = NULL;
  if (s_reader_layer != NULL) {
    layer_mark_dirty(s_reader_layer);
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_sleep_shortcut_mode) {
    const uint8_t count = sleep_shortcut_option_count();
    s_sleep_shortcut_selected = s_sleep_shortcut_selected == 0
        ? count - 1 : s_sleep_shortcut_selected - 1;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_attention_prompt) {
    confirm_attention(ATTENTION_CONFIRM_BUTTON);
    return;
  }
  if (s_bookmark_dialog_mode) {
    s_bookmark_dialog_selected = s_bookmark_dialog_selected == 0 ? 2
                                                                  : s_bookmark_dialog_selected - 1;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_origin_dialog_mode) {
    s_origin_dialog_selected = 0;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_bookmark_mode) {
    if (s_bookmarks.count > 0) {
      s_bookmark_selected = s_bookmark_selected == 0 ? s_bookmarks.count - 1 : s_bookmark_selected - 1;
      layer_mark_dirty(s_reader_layer);
    }
    return;
  }
  if (s_sleep_mode_menu) {
    s_sleep_selected = s_sleep_selected == 0 ? SLEEP_ROW_COUNT - 1 : s_sleep_selected - 1;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_settings_mode) {
    s_settings_selected = s_settings_selected == 0 ? SETTING_ROW_COUNT - 1 : s_settings_selected - 1;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_display_mode) {
    const uint8_t row_count = display_visible_row_count();
    s_display_selected = s_display_selected == 0 ? row_count - 1 : s_display_selected - 1;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_statistics_mode) {
    s_statistics_page = s_statistics_page == 0 ? 1 : 0;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_reader_menu_mode) {
    s_reader_menu_selected = s_reader_menu_selected == 0
                                 ? MENU_DESTINATION_COUNT - 1
                                 : s_reader_menu_selected - 1;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_chapter_mode) {
    if (s_chapter_count > 0) {
      s_chapter_selected = s_chapter_selected == 0 ? s_chapter_count - 1 : s_chapter_selected - 1;
      layer_mark_dirty(s_reader_layer);
    }
    return;
  }
  if (s_gallery_mode) {
    if (s_library_count > 0) {
      s_library_selected = s_library_selected == 0 ? s_library_count - 1 : s_library_selected - 1;
      layer_mark_dirty(s_reader_layer);
    }
    return;
  }
  if (s_context_mode) {
    if (s_context_history_count > 0) {
      if (show_context_absolute(s_context_history[--s_context_history_count])) {
        persist_state();
        ensure_prefetch();
        layer_mark_dirty(s_reader_layer);
      }
    } else if (s_context_page_start > 0) {
      s_context_last_page_end = s_context_page_start;
      s_context_find_last_page = true;
      s_context_page_start -= 1;
      layer_mark_dirty(s_reader_layer);
    } else if (s_remote_loaded && activate_previous_window()) {
      s_context_last_page_end = word_count();
      s_context_find_last_page = true;
      persist_state();
      ensure_prefetch();
    } else if (s_remote_loaded && s_remote_start_index > 0 && !s_window_request_sent) {
      s_resume_context_after_window = true;
      s_pending_loaded = false;
      s_after_loaded = false;
      s_previous_loaded = false;
      request_window_at(s_remote_start_index > READER_WINDOW_MAX_WORDS
                            ? s_remote_start_index - READER_WINDOW_MAX_WORDS
                            : 0,
                        WINDOW_TARGET_CURRENT);
    } else if (s_remote_loaded && s_remote_start_index > 0) {
      s_pending_context_navigation = true;
      s_pending_context_start = s_remote_start_index > READER_WINDOW_MAX_WORDS
                                    ? s_remote_start_index - READER_WINDOW_MAX_WORDS
                                    : 0;
      set_loading(true);
    }
    return;
  }
  adjust_wpm(1);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_sleep_shortcut_mode) {
    s_sleep_shortcut_selected =
        (s_sleep_shortcut_selected + 1) % sleep_shortcut_option_count();
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_attention_prompt) {
    confirm_attention(ATTENTION_CONFIRM_BUTTON);
    return;
  }
  if (s_bookmark_dialog_mode) {
    s_bookmark_dialog_selected = (s_bookmark_dialog_selected + 1) % 3;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_origin_dialog_mode) {
    s_origin_dialog_selected = 1;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_bookmark_mode) {
    if (s_bookmarks.count > 0) {
      s_bookmark_selected = (s_bookmark_selected + 1) % s_bookmarks.count;
      layer_mark_dirty(s_reader_layer);
    }
    return;
  }
  if (s_sleep_mode_menu) {
    s_sleep_selected = (s_sleep_selected + 1) % SLEEP_ROW_COUNT;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_settings_mode) {
    s_settings_selected = (s_settings_selected + 1) % SETTING_ROW_COUNT;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_display_mode) {
    s_display_selected = (s_display_selected + 1) % display_visible_row_count();
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_statistics_mode) {
    s_statistics_page = (s_statistics_page + 1) % 2;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_reader_menu_mode) {
    s_reader_menu_selected = (s_reader_menu_selected + 1) % MENU_DESTINATION_COUNT;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_chapter_mode) {
    if (s_chapter_count > 0) {
      s_chapter_selected = (s_chapter_selected + 1) % s_chapter_count;
      layer_mark_dirty(s_reader_layer);
    }
    return;
  }
  if (s_gallery_mode) {
    if (s_library_count > 0) {
      s_library_selected = (s_library_selected + 1) % s_library_count;
      layer_mark_dirty(s_reader_layer);
    }
    return;
  }
  if (s_context_mode) {
    const uint16_t available_words = context_word_count();
    if (s_context_page_end > s_context_page_start && s_context_page_end < available_words) {
      if (s_context_history_count < sizeof(s_context_history) / sizeof(s_context_history[0])) {
        s_context_history[s_context_history_count++] =
            s_remote_start_index + s_context_page_start;
      }
      const uint16_t next_page_start = s_context_page_end;
      if (next_page_start >= s_remote_word_count && context_has_contiguous_next_window()) {
        const uint16_t next_window_offset = next_page_start - s_remote_word_count;
        s_resume_context_after_window = true;
        activate_pending_window();
        s_context_page_start = next_window_offset;
        s_context_page_end = next_window_offset + 1;
      } else {
        s_context_page_start = next_page_start;
      }
      s_word_index = s_context_page_start;
      s_word_segment_index = 0;
      persist_state();
      ensure_prefetch();
      layer_mark_dirty(s_reader_layer);
    } else if (s_remote_loaded && s_pending_loaded) {
      s_resume_context_after_window = true;
      activate_pending_window();
      s_context_page_start = 0;
      s_context_page_end = 1;
    } else if (s_remote_loaded && s_remote_start_index + s_remote_word_count < s_remote_total_count &&
               !s_window_request_sent) {
      s_resume_context_after_window = true;
      request_window_at(s_remote_start_index + s_remote_word_count, WINDOW_TARGET_CURRENT);
    } else if (s_remote_loaded && s_window_request_sent) {
      s_resume_context_after_window = true;
      s_pending_context_navigation = true;
      s_pending_context_start = s_remote_start_index + s_remote_word_count;
      set_loading(true);
    }
    return;
  }
  adjust_wpm(-1);
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  note_attention_activity();
  if (s_sleep_shortcut_mode) {
    close_sleep_shortcut(true);
    return;
  }
  if (s_attention_prompt) {
    confirm_attention(ATTENTION_CONFIRM_BUTTON);
    return;
  }
  if (s_bookmark_dialog_mode) {
    s_bookmark_dialog_mode = false;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_origin_dialog_mode) {
    s_origin_dialog_mode = false;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_bookmark_mode) {
    s_bookmark_mode = false;
    s_reader_menu_mode = true;
    s_reader_menu_selected = MENU_BOOKMARKS;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_sleep_mode_menu) {
    s_sleep_mode_menu = false;
    s_settings_mode = true;
    s_settings_selected = SETTING_SLEEP_MODES;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_settings_mode) {
    s_settings_mode = false;
    s_reader_menu_mode = true;
    s_reader_menu_selected = MENU_SETTINGS;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_display_mode) {
    s_display_mode = false;
    s_reader_menu_mode = true;
    s_reader_menu_selected = MENU_DISPLAY;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_statistics_mode) {
    s_statistics_mode = false;
    s_bookmark_mode = false;
    s_reader_menu_mode = true;
    s_reader_menu_selected = MENU_STATISTICS;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_reader_menu_mode) {
    s_reader_menu_mode = false;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_chapter_mode) {
    s_chapter_mode = false;
    s_gallery_mode = true;
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_gallery_mode) {
    if (s_remote_loaded) {
      s_gallery_mode = false;
      layer_mark_dirty(s_reader_layer);
    } else {
      window_stack_pop(true);
    }
    return;
  }
  if (s_context_mode) {
    s_context_mode = false;
    update_backlight();
    schedule_next_word();
    layer_mark_dirty(s_reader_layer);
    return;
  }

  if (s_candidate_active && s_candidate_forward && s_origin_return_available &&
      !s_origin_dialog_offered) {
    set_playing(false);
    s_origin_dialog_mode = true;
    s_origin_dialog_offered = true;
    s_origin_dialog_selected = 0;
    layer_mark_dirty(s_reader_layer);
    return;
  }

  request_rewind_sentence();
}

static void back_multi_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Match two quick Back presses while still delaying a single press until release.
  back_click_handler(recognizer, context);
  back_click_handler(recognizer, context);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 650, select_long_click_handler, NULL);
  window_raw_click_subscribe(BUTTON_ID_SELECT, raw_button_down_handler, raw_button_up_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_long_click_subscribe(BUTTON_ID_UP, 650, up_long_click_handler, NULL);
  window_raw_click_subscribe(BUTTON_ID_UP, raw_button_down_handler, raw_button_up_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_DOWN, 650, down_long_click_handler, NULL);
  window_raw_click_subscribe(BUTTON_ID_DOWN, raw_button_down_handler, raw_button_up_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
  // Long Back is reserved by PebbleOS. A multi-click recognizer keeps the single action
  // from firing on button-down, so holding Back exits without first rewinding.
  window_multi_click_subscribe(BUTTON_ID_BACK, 2, 2, 250, true, back_multi_click_handler);
}

#ifdef _PBL_API_EXISTS_touch_service_subscribe
static bool list_or_menu_mode(void) {
  return s_gallery_mode || s_chapter_mode || s_reader_menu_mode || s_settings_mode ||
         s_display_mode || s_statistics_mode || s_bookmark_mode || s_sleep_mode_menu;
}

static bool tap_select_visible_row(int16_t y) {
  if (s_bookmark_dialog_mode) {
    for (uint8_t index = 0; index < 3; index += 1) {
      const int16_t top = 50 + index * 53;
      if (y >= top && y < top + 39) {
        s_bookmark_dialog_selected = index;
        return true;
      }
    }
    return false;
  }
  if (s_bookmark_mode) {
    uint8_t first = s_bookmark_selected > 2 ? s_bookmark_selected - 2 : 0;
    if (first + 5 > s_bookmarks.count) first = s_bookmarks.count > 5 ? s_bookmarks.count - 5 : 0;
    for (uint8_t visible = 0; visible < 5 && first + visible < s_bookmarks.count; visible += 1) {
      const int16_t top = 31 + visible * 37;
      if (y >= top - 2 && y < top + 32) {
        s_bookmark_selected = first + visible;
        return true;
      }
    }
    return false;
  }
  if (s_settings_mode) {
    uint8_t first = s_settings_selected > 2 ? s_settings_selected - 2 : 0;
    if (first + 6 > SETTING_ROW_COUNT) {
      first = SETTING_ROW_COUNT - 6;
    }
    for (uint8_t visible = 0; visible < 6; visible += 1) {
      const int16_t top = 32 + visible * 32;
      if (y >= top - 1 && y < top + 29) {
        s_settings_selected = first + visible;
        return true;
      }
    }
    return false;
  }
  if (s_sleep_mode_menu) {
    for (uint8_t row = 0; row < SLEEP_ROW_COUNT; row += 1) {
      const int16_t top = 32 + row * 32;
      if (y >= top - 1 && y < top + 29) {
        s_sleep_selected = row;
        return true;
      }
    }
    return false;
  }
  if (s_display_mode) {
    const uint8_t row_count = display_visible_row_count();
    const uint8_t visible_count = row_count < 6 ? row_count : 6;
    uint8_t first = s_display_selected > 2 ? s_display_selected - 2 : 0;
    if (first + visible_count > row_count) {
      first = row_count - visible_count;
    }
    for (uint8_t visible = 0; visible < visible_count; visible += 1) {
      const int16_t top = 32 + visible * 32;
      if (y >= top - 1 && y < top + 29) {
        s_display_selected = first + visible;
        return true;
      }
    }
    return false;
  }
  if (s_statistics_mode) {
    return false;
  }
  if (s_reader_menu_mode) {
    uint8_t first = s_reader_menu_selected > 2 ? s_reader_menu_selected - 2 : 0;
    if (first + 6 > MENU_DESTINATION_COUNT) first = MENU_DESTINATION_COUNT - 6;
    for (uint8_t visible = 0; visible < 6; visible += 1) {
      const uint8_t index = first + visible;
      const int16_t top = 32 + visible * 32;
      if (y >= top - 1 && y < top + 29) {
        s_reader_menu_selected = index;
        return true;
      }
    }
    return false;
  }
  if (s_chapter_mode) {
    uint8_t first = s_chapter_selected > 2 ? s_chapter_selected - 2 : 0;
    if (first + 5 > s_chapter_count) {
      first = s_chapter_count > 5 ? s_chapter_count - 5 : 0;
    }
    for (uint8_t visible = 0; visible < 5 && first + visible < s_chapter_count; visible += 1) {
      const int16_t top = 31 + visible * 37;
      if (y >= top - 2 && y < top + 32) {
        s_chapter_selected = first + visible;
        return true;
      }
    }
    return false;
  }
  if (s_gallery_mode) {
    uint8_t first = s_library_selected > 0 ? s_library_selected - 1 : 0;
    if (first + 3 > s_library_count) {
      first = s_library_count > 3 ? s_library_count - 3 : 0;
    }
    for (uint8_t visible = 0; visible < 3 && first + visible < s_library_count; visible += 1) {
      const int16_t top = 29 + visible * 63;
      if (y >= top - 2 && y < top + 58) {
        s_library_selected = first + visible;
        return true;
      }
    }
  }
  return false;
}

static void touch_service_handler(const TouchEvent *event, void *context) {
  if (event->type == TouchEvent_Touchdown) {
    note_attention_activity();
    s_touch_start = GPoint(event->x, event->y);
    s_touch_candidate = !event->non_navigational;
    return;
  }
  if (event->type == TouchEvent_PositionUpdate) {
    return;
  }
  if (event->type != TouchEvent_Liftoff || !s_touch_candidate) {
    return;
  }
  s_touch_candidate = false;
  if (!behavior_enabled(READER_BEHAVIOR_TOUCH) || s_loading_mode) {
    return;
  }
  const int16_t dx = event->x - s_touch_start.x;
  const int16_t dy = event->y - s_touch_start.y;
  const int16_t abs_x = dx < 0 ? -dx : dx;
  const int16_t abs_y = dy < 0 ? -dy : dy;
  if (abs_x < 14 && abs_y < 14) {
    if (!list_or_menu_mode() || tap_select_visible_row(event->y)) {
      s_touch_dispatching = true;
      select_click_handler(NULL, NULL);
      s_touch_dispatching = false;
    }
  } else if (abs_y >= 24 && abs_y >= abs_x) {
    if (dy < 0) {
      down_click_handler(NULL, NULL);
    } else {
      up_click_handler(NULL, NULL);
    }
  } else if (abs_x >= 24 && abs_x > abs_y && dx < 0) {
    if (s_context_mode) {
      down_click_handler(NULL, NULL);
    } else if (!list_or_menu_mode()) {
      request_next_sentence();
    }
  } else if (abs_x >= 24 && abs_x > abs_y) {
    if (s_context_mode) {
      up_click_handler(NULL, NULL);
    } else {
      back_click_handler(NULL, NULL);
    }
  }
}
#endif

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  const GRect bounds = layer_get_bounds(window_layer);
  s_reader_layer = layer_create(bounds);
  layer_set_update_proc(s_reader_layer, reader_update_proc);
  layer_add_child(window_layer, s_reader_layer);
#ifdef _PBL_API_EXISTS_touch_service_subscribe
  window_set_touch_bridge_disabled(window, true);
#endif
  apply_touch_setting();
}

static void window_unload(Window *window) {
  layer_destroy(s_reader_layer);
  s_reader_layer = NULL;
}

static void cancel_prefetch_timeout(void) {
  if (s_prefetch_timeout_timer != NULL) {
    app_timer_cancel(s_prefetch_timeout_timer);
    s_prefetch_timeout_timer = NULL;
  }
}

static void prefetch_timeout_callback(void *context) {
  s_prefetch_timeout_timer = NULL;
  if (!s_window_request_sent) {
    return;
  }
  APP_LOG(APP_LOG_LEVEL_WARNING, "Reader response timed out at %lu",
          (unsigned long)s_expected_start_index);
  s_window_request_sent = false;
  s_request_outbox_active = false;
  if (s_waiting_for_book) {
    s_waiting_for_book = false;
    if (s_book_request_attempts < 3) {
      s_pending_book_request = true;
      schedule_transport_followup();
    } else {
      s_pending_book_request = false;
      set_loading(false);
      s_gallery_mode = true;
      if (s_reader_layer != NULL) {
        layer_mark_dirty(s_reader_layer);
      }
    }
    return;
  }
  const uint32_t retry_start = s_expected_start_index;
  const WindowTarget retry_target = s_window_target;
  if (retry_target != WINDOW_TARGET_CURRENT && s_remote_loaded &&
      retry_start < s_remote_total_count) {
    if (s_prefetch_retry_count < UINT8_MAX) s_prefetch_retry_count += 1;
    request_window_at(retry_start, retry_target);
    if (s_reader_layer != NULL) layer_mark_dirty(s_reader_layer);
    return;
  }
  schedule_transport_followup();
}

static void expect_response(uint32_t request_id, uint32_t chunk_id, uint32_t start_index,
                            const uint8_t *book_id, uint32_t revision, WindowTarget target,
                            bool exact_start) {
  cancel_prefetch_timeout();
  s_expected_request_id = request_id;
  s_expected_chunk_id = chunk_id;
  s_expected_start_index = start_index;
  s_expected_revision = revision;
  memcpy(s_expected_book_id, book_id, sizeof(s_expected_book_id));
  s_expected_start_exact = exact_start;
  if (s_prefetch_retry_start != start_index || s_prefetch_retry_target != target) {
    s_prefetch_retry_count = 0;
    s_prefetch_retry_start = start_index;
    s_prefetch_retry_target = target;
  }
  s_window_target = target;
  s_window_request_sent = true;
  s_prefetch_timeout_timer = app_timer_register(PREFETCH_TIMEOUT_MS, prefetch_timeout_callback, NULL);
}

#define COPY_READER_WINDOW(destination, source) do { \
  memcpy(s_##destination##_words, s_##source##_words, sizeof(s_##destination##_words)); \
  memcpy(s_##destination##_paragraph_end, s_##source##_paragraph_end, sizeof(s_##destination##_paragraph_end)); \
  memcpy(s_##destination##_chapter_start, s_##source##_chapter_start, sizeof(s_##destination##_chapter_start)); \
  memcpy(s_##destination##_sentence_start, s_##source##_sentence_start, sizeof(s_##destination##_sentence_start)); \
  memcpy(s_##destination##_focus_index, s_##source##_focus_index, sizeof(s_##destination##_focus_index)); \
  memcpy(s_##destination##_base_duration, s_##source##_base_duration, sizeof(s_##destination##_base_duration)); \
  memcpy(s_##destination##_title, s_##source##_title, sizeof(s_##destination##_title)); \
  s_##destination##_word_count = s_##source##_word_count; \
  s_##destination##_start_index = s_##source##_start_index; \
  s_##destination##_total_count = s_##source##_total_count; \
} while (0)

static void activate_pending_window(void) {
  if (!s_pending_loaded) {
    return;
  }
  const bool continue_context = s_context_mode || s_resume_context_after_window;
  const bool continue_playing = s_playing || s_resume_after_window;
  COPY_READER_WINDOW(previous, remote);
  s_previous_loaded = true;
  COPY_READER_WINDOW(remote, next);
  if (s_window_request_sent && s_window_target == WINDOW_TARGET_AFTER) {
    s_window_target = WINDOW_TARGET_NEXT;
  }
  if (s_after_loaded) {
    COPY_READER_WINDOW(next, after);
    s_pending_loaded = true;
    s_after_loaded = false;
  } else {
    s_pending_loaded = false;
  }
  s_resume_after_window = false;
  s_word_index = 0;
  s_word_segment_index = 0;
  s_context_mode = continue_context;
  if (continue_context) {
    s_context_page_start = 0;
    s_context_page_end = 1;
    s_context_find_last_page = false;
    s_context_last_page_end = 0;
  }
  s_resume_context_after_window = false;
  s_playing = continue_playing && !s_context_mode;
  s_play_tick_ms = s_playing ? current_millis() : 0;
  update_backlight();
  set_loading(false);
  layer_mark_dirty(s_reader_layer);
  schedule_next_word();
  schedule_transport_followup();
}

static bool activate_previous_window(void) {
  if (!s_previous_loaded ||
      s_previous_start_index + s_previous_word_count != s_remote_start_index) {
    return false;
  }
  const bool continue_context = s_context_mode || s_resume_context_after_window;
  const bool had_next = s_pending_loaded;
  if (had_next) {
    COPY_READER_WINDOW(after, next);
  }
  s_after_loaded = had_next;
  COPY_READER_WINDOW(next, remote);
  s_pending_loaded = true;
  COPY_READER_WINDOW(remote, previous);
  s_previous_loaded = false;
  s_word_index = s_remote_word_count > 0 ? s_remote_word_count - 1 : 0;
  s_word_segment_index = 0;
  s_context_mode = continue_context;
  if (continue_context) {
    s_context_page_start = s_word_index;
    s_context_page_end = s_word_index + 1;
    s_context_last_page_end = 0;
  }
  s_resume_context_after_window = false;
  s_resume_after_window = false;
  stop_playback_clock();
  update_backlight();
  set_loading(false);
  if (s_reader_layer != NULL) {
    layer_mark_dirty(s_reader_layer);
  }
  schedule_transport_followup();
  return true;
}

#undef COPY_READER_WINDOW

static void request_window_command_at(const char *command, uint32_t start_index, WindowTarget target) {
  if (!s_remote_loaded || s_window_request_sent || start_index >= s_remote_total_count) {
    return;
  }
  char request[24];
  snprintf(request, sizeof(request), "%s:%lu", command, (unsigned long)start_index);
  const uint16_t request_length = (uint16_t)strlen(request);
  const uint32_t request_id = next_request_id();
  memset(s_control_packet, 0, sizeof(s_control_packet));
  memcpy(s_control_packet, "PRSP", 4);
  s_control_packet[4] = PROTOCOL_VERSION;
  s_control_packet[5] = MSG_REQUEST;
  s_control_packet[7] = HEADER_LENGTH;
  write_u32_be(s_control_packet + 8, request_id);
  write_u32_be(s_control_packet + 12, s_remote_revision);
  write_u32_be(s_control_packet + 16, start_index);
  write_u16_be(s_control_packet + 20, 0);
  write_u16_be(s_control_packet + 22, 1);
  write_u16_be(s_control_packet + 24, request_length);
  memcpy(s_control_packet + 30, s_remote_book_id, sizeof(s_remote_book_id));
  memcpy(s_control_packet + HEADER_LENGTH, request, request_length);
  write_u32_be(s_control_packet + 26, crc32_bytes(s_control_packet + HEADER_LENGTH, request_length));

  DictionaryIterator *out_iter;
  AppMessageResult result = app_message_outbox_begin(&out_iter);
  if (result != APP_MSG_OK) {
    return;
  }
  dict_write_data(out_iter, MESSAGE_KEY_PACKET, s_control_packet, HEADER_LENGTH + request_length);
  result = app_message_outbox_send();
  if (result != APP_MSG_OK) {
    return;
  }
  s_request_outbox_active = true;
  expect_response(request_id, start_index, start_index, s_remote_book_id, s_remote_revision, target,
                  strcmp(command, "window") == 0);
  s_navigation_request = target == WINDOW_TARGET_CURRENT;
  if (target == WINDOW_TARGET_CURRENT) {
    stop_playback_clock();
    update_backlight();
    if (s_timer != NULL) {
      app_timer_cancel(s_timer);
      s_timer = NULL;
    }
    set_loading(true);
  }
}

static void request_window_at(uint32_t start_index, WindowTarget target) {
  request_window_command_at("window", start_index, target);
}

static void request_rewind_sentence(void) {
  if (!s_remote_loaded || s_loading_mode) {
    return;
  }
  set_playing(false);
  const uint16_t current_start = sentence_start_at_or_before(s_word_index);
  if (behavior_enabled(READER_BEHAVIOR_CURRENT_SENTENCE_FIRST) && s_word_index != current_start) {
    s_word_index = current_start;
    s_word_segment_index = 0;
    persist_state();
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (current_start > 0) {
    s_word_index = sentence_start_at_or_before(current_start - 1);
    s_word_segment_index = 0;
    persist_state();
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (s_remote_start_index == 0) {
    s_word_index = 0;
    s_word_segment_index = 0;
    persist_state();
    layer_mark_dirty(s_reader_layer);
    return;
  }
  if (activate_previous_window()) {
    s_word_index = sentence_start_at_or_before(s_remote_word_count - 1);
    s_word_segment_index = 0;
    persist_state();
    ensure_prefetch();
    layer_mark_dirty(s_reader_layer);
    return;
  }
  const uint32_t absolute_index = s_remote_start_index + current_start;
  if (s_window_request_sent || s_request_outbox_active) {
    s_pending_reader_navigation = true;
    snprintf(s_pending_reader_command, sizeof(s_pending_reader_command), "rewind");
    s_pending_reader_start = absolute_index;
    set_loading_with_presentation(true, LOADING_PRESENTATION_CORNER);
    layer_mark_dirty(s_reader_layer);
    return;
  }
  s_pending_reader_navigation = true;
  snprintf(s_pending_reader_command, sizeof(s_pending_reader_command), "rewind");
  s_pending_reader_start = absolute_index;
  const uint32_t previous_start = s_remote_start_index > READER_WINDOW_MAX_WORDS
      ? s_remote_start_index - READER_WINDOW_MAX_WORDS : 0;
  set_loading_with_presentation(true, LOADING_PRESENTATION_CORNER);
  request_window_at(previous_start, WINDOW_TARGET_PREVIOUS);
}

static void request_next_sentence(void) {
  if (!s_remote_loaded || s_loading_mode) {
    return;
  }
  set_playing(false);
  for (uint16_t index = s_word_index + 1; index < word_count(); index += 1) {
    if (s_remote_sentence_start[index] || word_ends_sentence_at(index - 1)) {
      s_word_index = index;
      s_word_segment_index = 0;
      persist_state();
      layer_mark_dirty(s_reader_layer);
      return;
    }
  }
  const uint32_t absolute_index = s_remote_start_index + s_word_index;
  if (s_window_request_sent || s_request_outbox_active) {
    s_pending_reader_navigation = true;
    snprintf(s_pending_reader_command, sizeof(s_pending_reader_command), "next");
    s_pending_reader_start = absolute_index;
    set_loading(true);
    layer_mark_dirty(s_reader_layer);
    return;
  }
  request_window_command_at("next", absolute_index, WINDOW_TARGET_CURRENT);
}

static void ensure_prefetch(void) {
  if (!s_remote_loaded || s_window_request_sent || s_waiting_for_book || s_loading_mode ||
      s_gallery_mode || s_chapter_mode || s_reader_menu_mode || s_settings_mode || s_display_mode ||
      s_statistics_mode) {
    return;
  }
  if (!s_pending_loaded && s_remote_start_index + s_remote_word_count < s_remote_total_count) {
    request_window_at(s_remote_start_index + s_remote_word_count, WINDOW_TARGET_NEXT);
    return;
  }
  if (s_pending_loaded && !s_after_loaded &&
      (playback_requested() || s_word_index + 20 >= s_remote_word_count) &&
      s_next_start_index + s_next_word_count < s_next_total_count) {
    request_window_at(s_next_start_index + s_next_word_count, WINDOW_TARGET_AFTER);
    return;
  }
  if (!s_previous_loaded && s_remote_start_index > 0) {
    const uint32_t previous_start = s_remote_start_index > READER_WINDOW_MAX_WORDS
        ? s_remote_start_index - READER_WINDOW_MAX_WORDS : 0;
    request_window_at(previous_start, WINDOW_TARGET_PREVIOUS);
  }
}

static bool parse_reader_window(const uint8_t *data, uint16_t length) {
  if (length < READER_WINDOW_HEADER_BYTES || memcmp(data, "RWND", 4) != 0 || data[4] != READER_WINDOW_VERSION) {
    return false;
  }

  const uint16_t count = read_u16_be(data + 6);
  const uint8_t focus_offset = data[5];
  const uint32_t start_index = read_u32_be(data + 8);
  const uint32_t total_count = read_u32_be(data + 12);
  const uint8_t title_length = data[16];
  if (count == 0 || count > READER_WINDOW_MAX_WORDS || focus_offset >= count ||
      title_length > READER_WINDOW_MAX_TITLE_BYTES ||
      total_count == 0 || start_index >= total_count || count > total_count - start_index) {
    return false;
  }

  uint16_t record_offset = READER_WINDOW_HEADER_BYTES;
  for (uint16_t index = 0; index < count; index += 1) {
    if ((uint32_t)record_offset + READER_WINDOW_RECORD_HEADER_BYTES > length) {
      return false;
    }
    const uint8_t word_length = data[record_offset];
    if (word_length > READER_WINDOW_MAX_WORD_BYTES ||
        (uint32_t)record_offset + READER_WINDOW_RECORD_HEADER_BYTES + word_length > length) {
      return false;
    }
    record_offset += READER_WINDOW_RECORD_HEADER_BYTES + word_length;
  }
  if (record_offset != length) {
    return false;
  }

  WindowTarget target = s_window_target;
  if (s_expected_start_exact && start_index != s_expected_start_index) {
    return false;
  }
  if (target != WINDOW_TARGET_CURRENT) {
    if (focus_offset != 0) {
      return false;
    }
    if (total_count != s_remote_total_count) {
      return false;
    }
    const uint32_t remote_end = s_remote_start_index + s_remote_word_count;
    const uint32_t next_end = s_next_start_index + s_next_word_count;
    if (start_index == remote_end) {
      target = WINDOW_TARGET_NEXT;
    } else if (s_pending_loaded && start_index == next_end) {
      target = WINDOW_TARGET_AFTER;
    } else if (start_index + count == s_remote_start_index) {
      target = WINDOW_TARGET_PREVIOUS;
    } else if ((s_previous_loaded && start_index + count == s_previous_start_index) ||
               (s_after_loaded && start_index == s_after_start_index + s_after_word_count)) {
      cancel_prefetch_timeout();
      s_window_request_sent = false;
      return true;
    } else {
      return false;
    }
  }

  char (*words)[READER_WINDOW_MAX_WORD_BYTES + 1] = target == WINDOW_TARGET_NEXT
      ? s_next_words
      : target == WINDOW_TARGET_AFTER ? s_after_words
      : target == WINDOW_TARGET_PREVIOUS ? s_previous_words : s_remote_words;
  bool *paragraph_end = target == WINDOW_TARGET_NEXT
      ? s_next_paragraph_end
      : target == WINDOW_TARGET_AFTER ? s_after_paragraph_end
      : target == WINDOW_TARGET_PREVIOUS ? s_previous_paragraph_end : s_remote_paragraph_end;
  bool *chapter_start = target == WINDOW_TARGET_NEXT
      ? s_next_chapter_start
      : target == WINDOW_TARGET_AFTER ? s_after_chapter_start
      : target == WINDOW_TARGET_PREVIOUS ? s_previous_chapter_start : s_remote_chapter_start;
  bool *sentence_start = target == WINDOW_TARGET_NEXT
      ? s_next_sentence_start
      : target == WINDOW_TARGET_AFTER ? s_after_sentence_start
      : target == WINDOW_TARGET_PREVIOUS ? s_previous_sentence_start : s_remote_sentence_start;
  uint8_t *focus_index = target == WINDOW_TARGET_NEXT
      ? s_next_focus_index
      : target == WINDOW_TARGET_AFTER ? s_after_focus_index
      : target == WINDOW_TARGET_PREVIOUS ? s_previous_focus_index : s_remote_focus_index;
  uint16_t *base_duration = target == WINDOW_TARGET_NEXT
      ? s_next_base_duration
      : target == WINDOW_TARGET_AFTER ? s_after_base_duration
      : target == WINDOW_TARGET_PREVIOUS ? s_previous_base_duration : s_remote_base_duration;
  char *window_title = target == WINDOW_TARGET_NEXT
      ? s_next_title
      : target == WINDOW_TARGET_AFTER ? s_after_title
      : target == WINDOW_TARGET_PREVIOUS ? s_previous_title : s_remote_title;
  const bool was_playing = playback_requested();
  memset(window_title, 0, READER_WINDOW_MAX_TITLE_BYTES + 1);
  memcpy(window_title, data + 17, title_length);
  record_offset = READER_WINDOW_HEADER_BYTES;
  for (uint16_t index = 0; index < count; index += 1) {
    const uint8_t *record = data + record_offset;
    const uint8_t word_length = record[0];
    memset(words[index], 0, sizeof(words[index]));
    memcpy(words[index], record + 4, word_length);
    paragraph_end[index] = (record[1] & 1) != 0;
    chapter_start[index] = (record[1] & 2) != 0;
    sentence_start[index] = (record[1] & 4) != 0;
    focus_index[index] = (record[1] & 0x80) != 0 ? (record[1] >> 3) & 0x0f : UINT8_MAX;
    base_duration[index] = read_u16_be(record + 2);
    record_offset += READER_WINDOW_RECORD_HEADER_BYTES + word_length;
  }

  cancel_prefetch_timeout();
  s_window_request_sent = false;
  if (target == WINDOW_TARGET_NEXT) {
    s_next_word_count = count;
    s_next_start_index = start_index;
    s_next_total_count = total_count;
    s_pending_loaded = true;
    if (s_context_mode && s_reader_layer != NULL) {
      layer_mark_dirty(s_reader_layer);
    }
    if (s_resume_after_window || s_resume_context_after_window) {
      if (s_resume_context_after_window && s_pending_context_navigation &&
          s_pending_context_start == start_index) {
        s_pending_context_navigation = false;
      }
      activate_pending_window();
    }
  } else if (target == WINDOW_TARGET_AFTER) {
    s_after_word_count = count;
    s_after_start_index = start_index;
    s_after_total_count = total_count;
    s_after_loaded = true;
  } else if (target == WINDOW_TARGET_PREVIOUS) {
    s_previous_word_count = count;
    s_previous_start_index = start_index;
    s_previous_total_count = total_count;
    s_previous_loaded = true;
    if (s_pending_reader_navigation && strcmp(s_pending_reader_command, "rewind") == 0) {
      s_pending_reader_navigation = false;
      if (activate_previous_window()) {
        s_word_index = sentence_start_at_or_before(s_remote_word_count - 1);
        s_word_segment_index = 0;
        persist_state();
      }
    }
  } else {
    const bool was_context = s_context_mode;
    const bool same_state_book = s_state.book_revision == s_remote_revision &&
        memcmp(s_state.book_id, s_remote_book_id, sizeof(s_state.book_id)) == 0;
    s_remote_word_count = count;
    s_remote_start_index = start_index;
    s_remote_total_count = total_count;
    s_remote_loaded = true;
    s_waiting_for_book = false;
    s_book_request_attempts = 0;
    for (uint8_t index = 0; index < s_library_count; index += 1) {
      if (s_library_revisions[index] == s_remote_revision &&
          memcmp(s_library_book_ids[index], s_remote_book_id, 16) == 0) {
        s_library_selected = index;
        break;
      }
    }
    s_gallery_mode = false;
    s_chapter_mode = false;
    s_reader_menu_mode = false;
    s_settings_mode = false;
    s_display_mode = false;
    s_statistics_mode = false;
    s_resume_after_window = false;
    s_pending_loaded = false;
    s_after_loaded = false;
    s_previous_loaded = false;
    s_word_index = focus_offset;
    s_word_segment_index = 0;
    if (!same_state_book) {
      s_state.absolute_word_index = start_index;
      s_state.furthest_word_index = start_index;
      for (uint8_t index = 0; index < s_library_count; index += 1) {
        if (s_library_revisions[index] == s_remote_revision &&
            memcmp(s_library_book_ids[index], s_remote_book_id, 16) == 0) {
          s_state.absolute_word_index = s_library_resume_index[index];
          s_state.furthest_word_index = s_library_progress_index[index];
          break;
        }
      }
    }
    s_context_mode = s_navigation_request && was_context;
    s_resume_context_after_window = false;
    s_navigation_request = false;
    s_playing = was_playing;
    s_play_tick_ms = s_playing ? current_millis() : 0;
    update_backlight();
    set_loading(false);
    persist_state();
    if (s_reader_layer != NULL) {
      layer_mark_dirty(s_reader_layer);
    }
    schedule_next_word();
  }
  return true;
}

static void receive_data_packet(const uint8_t *packet, uint16_t length) {
  const uint16_t sequence_number = read_u16_be(packet + 20);
  const uint16_t sequence_count = read_u16_be(packet + 22);
  const uint16_t payload_length = read_u16_be(packet + 24);
  const uint32_t request_id = read_u32_be(packet + 8);
  const uint32_t chunk_id = read_u32_be(packet + 16);

  if (sequence_count > 128 || payload_length > length - HEADER_LENGTH ||
      (uint32_t)sequence_count * 160UL < payload_length) {
    send_control_packet(MSG_NACK, packet, length, "window_size");
    return;
  }

  if (sequence_number == 0) {
    const bool is_library = payload_length >= 4 && memcmp(packet + HEADER_LENGTH, "LIBR", 4) == 0;
    if (!is_library &&
        (!s_window_request_sent || request_id != s_expected_request_id || chunk_id != s_expected_chunk_id ||
         read_u32_be(packet + 12) != s_expected_revision ||
         memcmp(packet + 30, s_expected_book_id, sizeof(s_expected_book_id)) != 0)) {
      return;
    }
    if (s_window_next_sequence > 0 && s_window_next_sequence < s_window_sequence_count &&
        request_id == s_window_request_id && chunk_id == s_window_chunk_id &&
        sequence_count == s_window_sequence_count) {
      return;
    }
    s_window_buffer_length = 0;
    s_window_next_sequence = 0;
    s_window_sequence_count = sequence_count;
    s_window_request_id = request_id;
    s_window_chunk_id = chunk_id;
    const bool is_catalog = is_library ||
        (payload_length >= 4 && memcmp(packet + HEADER_LENGTH, "CHAP", 4) == 0);
    if (!is_catalog) {
      s_remote_revision = read_u32_be(packet + 12);
      memcpy(s_remote_book_id, packet + 30, sizeof(s_remote_book_id));
    }
  }

  if (request_id != s_window_request_id || chunk_id != s_window_chunk_id ||
      sequence_count != s_window_sequence_count) {
    send_control_packet(MSG_NACK, packet, length, "window_changed");
    return;
  }
  if (sequence_number < s_window_next_sequence) {
    return;
  }
  if (sequence_number != s_window_next_sequence) {
    send_control_packet(MSG_NACK, packet, length, "out_of_order");
    return;
  }
  if (s_window_buffer == NULL ||
      (uint32_t)s_window_buffer_length + payload_length > READER_WINDOW_BUFFER_BYTES) {
    send_control_packet(MSG_NACK, packet, length, "window_buffer");
    return;
  }

  memcpy(s_window_buffer + s_window_buffer_length, packet + HEADER_LENGTH, payload_length);
  s_window_buffer_length += payload_length;
  s_window_next_sequence += 1;
  if (s_window_next_sequence == s_window_sequence_count) {
    const bool valid = memcmp(s_window_buffer, "LIBR", 4) == 0
                           ? parse_library(s_window_buffer, s_window_buffer_length)
                           : memcmp(s_window_buffer, "CHAP", 4) == 0
                               ? parse_chapters(s_window_buffer, s_window_buffer_length)
                               : parse_reader_window(s_window_buffer, s_window_buffer_length);
    if (!valid) {
      send_control_packet(MSG_NACK, packet, length, "window_invalid");
      return;
    }
    schedule_transport_followup();
  }
}

static void inbox_received_callback(DictionaryIterator *iter, void *context) {
  Tuple *packet_tuple = dict_find(iter, MESSAGE_KEY_PACKET);
  if (packet_tuple == NULL || packet_tuple->type != TUPLE_BYTE_ARRAY) {
    return;
  }

  const uint8_t *packet = packet_tuple->value->data;
  const uint16_t length = packet_tuple->length;
  if (!validate_packet(packet, length)) {
    send_control_packet(MSG_NACK, packet, length, "invalid");
    return;
  }

  const uint8_t message_type = packet[5];
  if (message_type == MSG_DATA) {
    receive_data_packet(packet, length);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "inbox dropped: %d", (int)reason);
}

static bool send_progress_packet(void) {
  if (!s_progress_pending || !s_remote_loaded || s_remote_total_count == 0 ||
      s_window_request_sent || s_waiting_for_book) {
    return false;
  }
  uint8_t payload[12];
  uint32_t absolute_index = s_state.absolute_word_index;
  if (absolute_index >= s_remote_total_count) absolute_index = s_remote_total_count - 1;
  write_u32_be(payload, absolute_index);
  write_u32_be(payload + 4, s_remote_total_count);
  write_u32_be(payload + 8, s_state.furthest_word_index);

  memset(s_control_packet, 0, sizeof(s_control_packet));
  memcpy(s_control_packet, "PRSP", 4);
  s_control_packet[4] = PROTOCOL_VERSION;
  s_control_packet[5] = MSG_PROGRESS;
  s_control_packet[7] = HEADER_LENGTH;
  write_u32_be(s_control_packet + 8, next_request_id());
  write_u32_be(s_control_packet + 12, s_remote_revision);
  write_u32_be(s_control_packet + 16, absolute_index);
  write_u16_be(s_control_packet + 22, 1);
  write_u16_be(s_control_packet + 24, sizeof(payload));
  memcpy(s_control_packet + 30, s_remote_book_id, sizeof(s_remote_book_id));
  memcpy(s_control_packet + HEADER_LENGTH, payload, sizeof(payload));
  write_u32_be(s_control_packet + 26, crc32_bytes(payload, sizeof(payload)));

  DictionaryIterator *out_iter;
  if (app_message_outbox_begin(&out_iter) != APP_MSG_OK) {
    return false;
  }
  dict_write_data(out_iter, MESSAGE_KEY_PACKET, s_control_packet, HEADER_LENGTH + sizeof(payload));
  if (app_message_outbox_send() != APP_MSG_OK) {
    return false;
  }
  s_progress_pending = false;
  s_progress_outbox_active = true;
  return true;
}

static bool send_account_settings_packet(void) {
  if (!s_account_settings_pending || s_window_request_sent || s_waiting_for_book) {
    return false;
  }
  uint8_t payload[3] = {1, (uint8_t)s_language, (uint8_t)s_library_sort};
  memset(s_control_packet, 0, sizeof(s_control_packet));
  memcpy(s_control_packet, "PRSP", 4);
  s_control_packet[4] = PROTOCOL_VERSION;
  s_control_packet[5] = MSG_SETTINGS;
  s_control_packet[7] = HEADER_LENGTH;
  write_u32_be(s_control_packet + 8, next_request_id());
  write_u16_be(s_control_packet + 22, 1);
  write_u16_be(s_control_packet + 24, sizeof(payload));
  memcpy(s_control_packet + HEADER_LENGTH, payload, sizeof(payload));
  write_u32_be(s_control_packet + 26, crc32_bytes(payload, sizeof(payload)));

  DictionaryIterator *out_iter;
  if (app_message_outbox_begin(&out_iter) != APP_MSG_OK) {
    return false;
  }
  dict_write_data(out_iter, MESSAGE_KEY_PACKET, s_control_packet, HEADER_LENGTH + sizeof(payload));
  if (app_message_outbox_send() != APP_MSG_OK) {
    return false;
  }
  s_account_settings_pending = false;
  s_account_settings_awaiting_confirmation = true;
  s_settings_outbox_active = true;
  return true;
}

static void run_transport_followup(void) {
  if (s_resume_request_pending && !s_waiting_for_book) {
    request_selected_book(LOADING_PRESENTATION_BOOK);
    return;
  }
  if (s_pending_book_request && !s_window_request_sent) {
    const uint32_t start = s_pending_book_start;
    const bool sentence_resume = s_pending_book_sentence_resume;
    const LoadingPresentation loading_presentation = s_pending_book_loading_presentation;
    request_selected_book_at(start, sentence_resume, loading_presentation);
    return;
  }
  if (s_pending_reader_navigation && !s_window_request_sent) {
    if (strcmp(s_pending_reader_command, "rewind") == 0 && s_remote_start_index > 0) {
      if (activate_previous_window()) {
        s_pending_reader_navigation = false;
        s_word_index = sentence_start_at_or_before(s_remote_word_count - 1);
        s_word_segment_index = 0;
        persist_state();
        ensure_prefetch();
        return;
      }
      const uint32_t previous_start = s_remote_start_index > READER_WINDOW_MAX_WORDS
          ? s_remote_start_index - READER_WINDOW_MAX_WORDS : 0;
      request_window_at(previous_start, WINDOW_TARGET_PREVIOUS);
      return;
    }
    char command[sizeof(s_pending_reader_command)];
    snprintf(command, sizeof(command), "%s", s_pending_reader_command);
    const uint32_t start = s_pending_reader_start;
    s_pending_reader_navigation = false;
    request_window_command_at(command, start, WINDOW_TARGET_CURRENT);
    return;
  }
  if (s_pending_context_navigation && !s_window_request_sent) {
    const uint32_t start = s_pending_context_start;
    s_pending_context_navigation = false;
    s_pending_loaded = false;
    s_after_loaded = false;
    request_window_at(start, WINDOW_TARGET_CURRENT);
    return;
  }
  if (s_pending_chapter_request && !s_window_request_sent) {
    request_selected_chapters();
    return;
  }
  if (s_library_sync_pending && !s_window_request_sent && !s_request_outbox_active) {
    request_library_sync();
    return;
  }
  if (s_remote_loaded && !s_pending_loaded && !s_loading_mode && !s_context_mode &&
      s_remote_start_index + s_remote_word_count < s_remote_total_count) {
    request_window_at(s_remote_start_index + s_remote_word_count, WINDOW_TARGET_NEXT);
    return;
  }
  if (send_progress_packet()) {
    return;
  }
  if (send_account_settings_packet()) {
    return;
  }
  ensure_prefetch();
}

static void transport_followup_callback(void *context) {
  s_transport_followup_timer = NULL;
  run_transport_followup();
}

static void schedule_transport_followup(void) {
  if (s_transport_followup_timer == NULL) {
    s_transport_followup_timer = app_timer_register(TRANSPORT_FOLLOWUP_MS, transport_followup_callback, NULL);
  }
}

static void outbox_sent_callback(DictionaryIterator *iter, void *context) {
  s_progress_outbox_active = false;
  s_settings_outbox_active = false;
  s_request_outbox_active = false;
  s_library_sync_outbox_active = false;
  run_transport_followup();
}

static void outbox_failed_callback(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "outbox failed: %d", (int)reason);
  if (s_progress_outbox_active) {
    s_progress_outbox_active = false;
    s_progress_pending = true;
    return;
  }
  if (s_settings_outbox_active) {
    s_settings_outbox_active = false;
    s_account_settings_awaiting_confirmation = false;
    s_account_settings_pending = true;
    schedule_transport_followup();
    return;
  }
  if (s_library_sync_outbox_active) {
    s_library_sync_outbox_active = false;
    s_request_outbox_active = false;
    return;
  }
  if (!s_request_outbox_active) {
    return;
  }
  s_request_outbox_active = false;
  cancel_prefetch_timeout();
  s_window_request_sent = false;
  if (s_waiting_for_chapters) {
    s_waiting_for_chapters = false;
    set_loading(false);
    s_gallery_mode = true;
    if (s_reader_layer != NULL) {
      layer_mark_dirty(s_reader_layer);
    }
    return;
  }
  if (s_waiting_for_book || s_window_target == WINDOW_TARGET_CURRENT) {
    const bool was_waiting_for_book = s_waiting_for_book;
    s_waiting_for_book = false;
    if (was_waiting_for_book && s_book_request_attempts < 3) {
      s_pending_book_request = true;
      schedule_transport_followup();
      return;
    }
    set_loading(false);
    if (!s_remote_loaded) {
      s_gallery_mode = true;
    }
    if (s_reader_layer != NULL) {
      layer_mark_dirty(s_reader_layer);
    }
    return;
  }
  if (s_resume_after_window) {
    s_resume_after_window = false;
    update_backlight();
    persist_state();
    if (s_reader_layer != NULL) {
      layer_mark_dirty(s_reader_layer);
    }
  }
  schedule_transport_followup();
}

static void init(void) {
  s_window_buffer = malloc(READER_WINDOW_BUFFER_BYTES);
  if (s_window_buffer == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Unable to allocate transfer buffer");
  }
  s_previous_cache = calloc(1, sizeof(ReaderWindowCache));
  s_next_cache = calloc(1, sizeof(ReaderWindowCache));
  s_after_cache = calloc(1, sizeof(ReaderWindowCache));
  if (s_previous_cache == NULL || s_next_cache == NULL || s_after_cache == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Unable to allocate reader window caches");
  }
  s_library_covers = malloc(LIBRARY_MAX_BOOKS * LIBRARY_COVER_BYTES);
  if (s_library_covers == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Unable to allocate cover cache");
  }
  s_library_cover_palettes = malloc(LIBRARY_MAX_BOOKS * LIBRARY_COVER_PALETTE_BYTES);
  if (s_library_cover_palettes == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Unable to allocate cover palettes");
  }
  load_state();
  load_settings();
  load_bookmarks();
  light_enable(false);
  s_backlight_forced = false;
  update_backlight();
  s_window = window_create();
  window_set_click_config_provider(s_window, click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers){
                                           .load = window_load,
                                           .unload = window_unload,
                                       });
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_open(512, 128);
  accel_tap_service_subscribe(accel_tap_handler);
  window_stack_push(s_window, false);
  set_loading_with_presentation(true, LOADING_PRESENTATION_BOOK);
}

static void deinit(void) {
  if (s_timer != NULL) {
    app_timer_cancel(s_timer);
  }
  if (s_loading_timer != NULL) {
    app_timer_cancel(s_loading_timer);
  }
  if (s_feedback_timer != NULL) {
    app_timer_cancel(s_feedback_timer);
  }
  if (s_transport_followup_timer != NULL) {
    app_timer_cancel(s_transport_followup_timer);
  }
  if (s_sleep_timer != NULL) app_timer_cancel(s_sleep_timer);
  if (s_sleep_close_timer != NULL) app_timer_cancel(s_sleep_close_timer);
  accel_tap_service_unsubscribe();
  cancel_backlight_release();
  cancel_prefetch_timeout();
#ifdef _PBL_API_EXISTS_touch_service_subscribe
  if (s_touch_subscribed) {
    touch_service_unsubscribe();
    s_touch_subscribed = false;
  }
#endif
  persist_state();
  persist_settings();
  persist_bookmarks();
  light_enable(false);
  window_destroy(s_window);
  free(s_window_buffer);
  s_window_buffer = NULL;
  free(s_previous_cache);
  s_previous_cache = NULL;
  free(s_next_cache);
  s_next_cache = NULL;
  free(s_after_cache);
  s_after_cache = NULL;
  free(s_library_covers);
  s_library_covers = NULL;
  free(s_library_cover_palettes);
  s_library_cover_palettes = NULL;
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
