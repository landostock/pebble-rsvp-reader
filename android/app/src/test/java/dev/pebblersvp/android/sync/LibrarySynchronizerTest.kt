package dev.pebblersvp.android.sync

import dev.pebblersvp.android.cache.CachedProgress
import org.junit.Assert.assertEquals
import org.junit.Test
import java.util.UUID

class LibrarySynchronizerTest {
    private val bookId = UUID.fromString("3890b2d2-d572-4842-a5f3-5228319a77e9")

    @Test
    fun rebasesNewerCompatibleLocalProgressOntoCurrentRevision() {
        val local = progress(revision = 1, index = 7_634, updatedAt = "2026-09-01T23:01:14.131Z")
        val book = book(revision = 4, index = 7_313, updatedAt = "2026-09-01T17:48:24.626Z")

        val merged = mergedProgressForBook(local, book)!!

        assertEquals(4, merged.bookRevision)
        assertEquals(7_634, merged.tokenIndex)
        assertEquals(local.updatedAt, merged.updatedAt)
    }

    @Test
    fun keepsNewerServerProgress() {
        val local = progress(revision = 1, index = 7_634, updatedAt = "2026-09-01T17:48:24.626Z")
        val book = book(revision = 4, index = 7_700, updatedAt = "2026-09-01T23:01:14.131Z")

        val merged = mergedProgressForBook(local, book)!!

        assertEquals(4, merged.bookRevision)
        assertEquals(7_700, merged.tokenIndex)
        assertEquals(book.progressUpdatedAt, merged.updatedAt)
    }

    @Test
    fun rejectsLocalPositionWhenTokenCountChanged() {
        val local = progress(revision = 1, index = 7_634, updatedAt = "2026-09-01T23:01:14.131Z")
            .copy(totalTokenCount = 16_000)
        val book = book(revision = 4, index = 7_313, updatedAt = "2026-09-01T17:48:24.626Z")

        val merged = mergedProgressForBook(local, book)!!

        assertEquals(7_313, merged.tokenIndex)
        assertEquals(book.progressUpdatedAt, merged.updatedAt)
    }

    @Test
    fun newerServerResetOverridesLocalProgress() {
        val local = progress(revision = 1, index = 7_634, updatedAt = "2026-09-01T12:00:00.000Z")
        val book = book(revision = 4, index = 0, updatedAt = "2026-09-01T13:00:00.000Z")
            .copy(progressFurthestTokenIndex = 0, progressPercentage = 0, progressReset = true)

        val merged = mergedProgressForBook(local, book)!!

        assertEquals(0, merged.tokenIndex)
        assertEquals(0, merged.furthestTokenIndex)
        assertEquals(true, merged.reset)
    }

    private fun progress(revision: Long, index: Int, updatedAt: String) = CachedProgress(
        bookId = bookId,
        bookRevision = revision,
        tokenIndex = index,
        furthestTokenIndex = index,
        totalTokenCount = 16_613,
        percentage = 45,
        updatedAt = updatedAt,
    )

    private fun book(revision: Long, index: Int, updatedAt: String) = LibraryBook(
        bookId = bookId,
        revision = revision,
        coverRevision = 1,
        title = "Der zerbrochne Krug",
        tokenCount = 16_613,
        progressTokenIndex = index,
        progressFurthestTokenIndex = index,
        progressPercentage = 44,
        progressUpdatedAt = updatedAt,
    )
}
