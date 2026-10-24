package dev.pebblersvp.android.cache

import org.junit.Assert.assertEquals
import org.junit.Test
import java.nio.file.Files
import java.util.UUID

class ProgressCacheTest {
    @Test
    fun canonicalizesPositionAndRejectsOnlyStaleUpdatesForTheSameRevision() {
        val directory = Files.createTempDirectory("progress-cache-test").toFile()
        try {
            val cache = ProgressCache(directory)
            val bookId = UUID.randomUUID()
            val current = cache.write(
                CachedProgress(bookId, 1, 200, 100, 1, "2026-08-30T12:00:00.000Z"),
            )
            val stale = cache.write(
                CachedProgress(bookId, 1, 20, 100, 20, "2026-08-30T11:00:00.000Z"),
            )
            val newRevision = cache.write(
                CachedProgress(bookId, 2, 10, 50, 0, "2026-08-30T10:00:00.000Z"),
            )

            assertEquals(99, current.tokenIndex)
            assertEquals(99, current.furthestTokenIndex)
            assertEquals(100, current.percentage)
            assertEquals(current, stale)
            assertEquals(2, newRevision.bookRevision)
            assertEquals(20, newRevision.percentage)
            assertEquals(newRevision, cache.read(bookId))
        } finally {
            directory.deleteRecursively()
        }
    }

    @Test
    fun migratesLegacyNullTimestampAndAcceptsTheNextWatchUpdate() {
        val directory = Files.createTempDirectory("progress-cache-null-test").toFile()
        try {
            val bookId = UUID.randomUUID()
            directory.resolve("$bookId.json").writeText(
                """{"book_id":"$bookId","book_revision":1,"token_index":0,"total_token_count":1000,"percentage":0,"updated_at":"null"}""",
            )
            val cache = ProgressCache(directory)

            assertEquals("", cache.read(bookId)?.updatedAt)
            val updated = cache.write(
                CachedProgress(bookId, 1, 430, 1000, 43, "2026-09-01T12:00:00.000Z"),
            )

            assertEquals(430, updated.tokenIndex)
            assertEquals(430, updated.furthestTokenIndex)
            assertEquals(43, updated.percentage)
            assertEquals("2026-09-01T12:00:00.000Z", cache.read(bookId)?.updatedAt)

            val rewound = cache.write(
                CachedProgress(bookId, 1, 410, 1000, 41, "2026-09-01T12:01:00.000Z"),
            )
            assertEquals(410, rewound.tokenIndex)
            assertEquals(430, rewound.furthestTokenIndex)
            assertEquals(43, rewound.percentage)
        } finally {
            directory.deleteRecursively()
        }
    }

    @Test
    fun explicitServerResetClearsMonotonicFurthestPosition() {
        val directory = Files.createTempDirectory("progress-cache-reset-test").toFile()
        try {
            val cache = ProgressCache(directory)
            val bookId = UUID.randomUUID()
            cache.write(CachedProgress(bookId, 1, 430, 1000, 43, "2026-09-01T12:00:00.000Z", 430))

            val reset = cache.write(
                CachedProgress(bookId, 1, 0, 1000, 0, "2026-09-01T13:00:00.000Z", 0, reset = true),
            )

            assertEquals(0, reset.tokenIndex)
            assertEquals(0, reset.furthestTokenIndex)
            assertEquals(0, reset.percentage)
            assertEquals(true, cache.read(bookId)?.reset)
        } finally {
            directory.deleteRecursively()
        }
    }
}
