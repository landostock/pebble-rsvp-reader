package dev.pebblersvp.android.sync

import org.json.JSONObject
import java.util.UUID

data class LibraryBook(
    val bookId: UUID,
    val revision: Long,
    val coverRevision: Long,
    val title: String,
    val authors: String = "",
    val tokenCount: Int = 0,
    val progressTokenIndex: Int = 0,
    val progressFurthestTokenIndex: Int = progressTokenIndex,
    val progressPercentage: Int = 0,
    val progressUpdatedAt: String = "",
    val progressReset: Boolean = false,
    val createdAt: String = "",
)

data class LibraryPreferences(
    val language: String = "en",
    val librarySort: String = "last_read",
)

data class LibraryManifest(
    val revision: Long,
    val books: List<LibraryBook>,
    val preferences: LibraryPreferences = LibraryPreferences(),
) {
    fun orderedBooks(source: List<LibraryBook> = books): List<LibraryBook> {
        val byTitle = compareBy<LibraryBook, String>(String.CASE_INSENSITIVE_ORDER) { it.title }
        return when (preferences.librarySort) {
            "title" -> source.sortedWith(byTitle)
            "author" -> source.sortedWith(
                compareBy<LibraryBook, String>(String.CASE_INSENSITIVE_ORDER) { it.authors }
                    .then(byTitle),
            )
            "recent" -> source.sortedWith(
                compareByDescending<LibraryBook> { it.createdAt }.then(byTitle),
            )
            "progress" -> source.sortedWith(
                compareByDescending<LibraryBook> { progressTenths(it) }.then(byTitle),
            )
            else -> source.sortedWith(
                compareByDescending<LibraryBook> { hasProgress(it) }
                    .thenByDescending { it.progressUpdatedAt }
                    .thenByDescending { it.createdAt }
                    .then(byTitle),
            )
        }
    }

    companion object {
        fun parse(json: String): LibraryManifest {
            val root = JSONObject(json)
            val booksJson = root.getJSONArray("books")
            val books = buildList {
                for (index in 0 until booksJson.length()) {
                    val book = booksJson.getJSONObject(index)
                    add(
                        LibraryBook(
                            bookId = UUID.fromString(book.getString("book_id")),
                            revision = book.getLong("revision").also { require(it > 0) },
                            coverRevision = book.optLong("cover_revision", 0).also { require(it >= 0) },
                            title = book.optString("title", "Untitled"),
                            authors = book.optJSONArray("authors")?.let { authorsJson ->
                                buildList {
                                    for (authorIndex in 0 until authorsJson.length()) {
                                        authorsJson.optString(authorIndex).trim().takeIf(String::isNotEmpty)?.let(::add)
                                    }
                                }.joinToString(", ")
                            }.orEmpty(),
                            tokenCount = book.optInt("token_count", 0).coerceAtLeast(0),
                            progressTokenIndex = book.optInt("progress_token_index", 0).coerceAtLeast(0),
                            progressFurthestTokenIndex = book.optInt(
                                "progress_furthest_token_index",
                                book.optInt("progress_token_index", 0),
                            ).coerceAtLeast(0),
                            progressPercentage = book.optInt("progress_percentage", 0).coerceIn(0, 100),
                            progressUpdatedAt = normalizedTimestamp(book.optString("progress_updated_at", "")),
                            progressReset = book.optBoolean("progress_reset", false),
                            createdAt = normalizedTimestamp(book.optString("created_at", "")),
                        ),
                    )
                }
            }
            require(books.map(LibraryBook::bookId).distinct().size == books.size) {
                "Library contains duplicate book IDs"
            }
            return LibraryManifest(
                revision = root.getLong("library_revision").also { require(it >= 0) },
                books = books,
                preferences = root.optJSONObject("settings")?.let { settings ->
                    LibraryPreferences(
                        language = settings.optString("language", "en").takeIf { it in setOf("en", "de") } ?: "en",
                        librarySort = settings.optString("library_sort", "last_read").takeIf {
                            it in setOf("last_read", "title", "author", "recent", "progress")
                        } ?: "last_read",
                    )
                } ?: LibraryPreferences(),
            )
        }

        private fun normalizedTimestamp(value: String): String =
            value.takeUnless { it.isBlank() || it == "null" } ?: ""

        private fun progressTenths(book: LibraryBook): Long = if (book.tokenCount <= 1) {
            0
        } else {
            (book.progressFurthestTokenIndex.toLong().coerceAtLeast(0) * 1_000L) / (book.tokenCount - 1)
        }

        private fun hasProgress(book: LibraryBook): Boolean =
            book.progressTokenIndex > 0 || book.progressFurthestTokenIndex > 0
    }
}
