package dev.pebblersvp.android.cache

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import java.nio.file.Files
import java.util.UUID

class CoverCacheTest {
    @Test
    fun pruneDropsOldMissingAndCoverlessEntries() {
        val directory = Files.createTempDirectory("cover-cache-test").toFile()
        try {
            val cache = CoverCache(directory)
            val revisedBook = UUID.randomUUID()
            val coverlessBook = UUID.randomUUID()
            val deletedBook = UUID.randomUUID()
            cache.write(revisedBook, 1, byteArrayOf(1))
            cache.write(revisedBook, 2, byteArrayOf(2))
            cache.write(coverlessBook, 1, byteArrayOf(3))
            cache.write(deletedBook, 1, byteArrayOf(4))

            cache.prune(mapOf(revisedBook to 2L, coverlessBook to 0L))

            assertFalse(cache.has(revisedBook, 1))
            assertTrue(cache.has(revisedBook, 2))
            assertFalse(cache.has(coverlessBook, 1))
            assertFalse(cache.has(deletedBook, 1))
            assertEquals(1, cache.entryCount())
        } finally {
            directory.deleteRecursively()
        }
    }
}
