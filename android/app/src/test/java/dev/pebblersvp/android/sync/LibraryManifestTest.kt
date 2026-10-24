package dev.pebblersvp.android.sync

import org.junit.Assert.assertEquals
import org.junit.Test

class LibraryManifestTest {
    @Test
    fun parsesAuthorsForWatchLibraryMetadata() {
        val manifest = LibraryManifest.parse(
            """
            {
              "library_revision": 3,
              "books": [{
                "book_id": "00000000-0000-0000-0000-000000000001",
                "revision": 4,
                "cover_revision": 2,
                "title": "Ein Titel",
                "authors": ["Ada Autorin", "Max Mustermann"],
                "token_count": 1000,
                "progress_token_index": 120,
                "progress_furthest_token_index": 430
              }]
            }
            """.trimIndent(),
        )

        assertEquals("Ada Autorin, Max Mustermann", manifest.books.single().authors)
        assertEquals(120, manifest.books.single().progressTokenIndex)
        assertEquals(430, manifest.books.single().progressFurthestTokenIndex)
    }

    @Test
    fun parsesPreferencesAndOrdersRecentlyReadBooksFirst() {
        val manifest = LibraryManifest.parse(
            """
            {
              "library_revision": 4,
              "settings": {"language":"de", "library_sort":"last_read"},
              "books": [
                {
                  "book_id":"00000000-0000-0000-0000-000000000001",
                  "revision":1,
                  "title":"Alt",
                  "progress_updated_at":"2026-08-30T10:00:00.000Z"
                },
                {
                  "book_id":"00000000-0000-0000-0000-000000000002",
                  "revision":1,
                  "title":"Neu",
                  "progress_updated_at":"2026-08-31T10:00:00.000Z"
                }
              ]
            }
            """.trimIndent(),
        )

        assertEquals("de", manifest.preferences.language)
        assertEquals("Neu", manifest.orderedBooks().first().title)
    }

    @Test
    fun treatsJsonNullProgressTimestampsAsMissing() {
        val manifest = LibraryManifest.parse(
            """
            {
              "library_revision": 1,
              "books": [{
                "book_id":"00000000-0000-0000-0000-000000000001",
                "revision":1,
                "title":"Test",
                "progress_updated_at":null
              }]
            }
            """.trimIndent(),
        )

        assertEquals("", manifest.books.single().progressUpdatedAt)
    }
}
