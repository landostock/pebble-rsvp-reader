package dev.pebblersvp.android.transport

import dev.pebblersvp.android.cache.CoverCache
import dev.pebblersvp.android.sync.LibraryBook
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import java.nio.file.Files
import java.util.UUID

class LibraryPayloadEncoderTest {
    @Test
    fun encodesBookIdentityAndTitleWithoutRequiringCoverBytes() {
        val directory = Files.createTempDirectory("library-payload-test").toFile()
        try {
            val book = LibraryBook(
                UUID.randomUUID(),
                revision = 4,
                coverRevision = 0,
                title = "Änderung",
                authors = "Ada Autorin",
                tokenCount = 1_000,
                progressTokenIndex = 420,
                progressFurthestTokenIndex = 430,
                progressPercentage = 42,
                progressUpdatedAt = "2026-08-31T09:05:20.000Z",
            )
            val payload = LibraryPayloadEncoder.encode(listOf(book), CoverCache(directory), "de", "author")

            assertEquals(12 + 1068, payload.size)
            assertEquals("LIBR", String(payload, 0, 4, Charsets.US_ASCII))
            assertEquals(6, payload[4].toInt())
            assertEquals(1, payload[5].toInt())
            assertEquals(1, payload[6].toInt())
            assertEquals(2, payload[7].toInt())
            assertEquals(4, java.nio.ByteBuffer.wrap(payload, 28, 4).int)
            assertEquals(9, payload[32].toInt())
            assertTrue(String(payload, 33, 9, Charsets.UTF_8).startsWith("Änder"))
            assertEquals(11, payload[81].toInt())
            assertEquals(420, java.nio.ByteBuffer.wrap(payload, 106, 4).int)
            assertEquals(430, java.nio.ByteBuffer.wrap(payload, 110, 4).int)
            assertEquals(1_000, java.nio.ByteBuffer.wrap(payload, 114, 4).int)
            assertEquals(42, payload[118].toInt())
            assertTrue(java.nio.ByteBuffer.wrap(payload, 119, 4).int > 0)
            assertEquals(0, payload[127].toInt())
        } finally {
            directory.deleteRecursively()
        }
    }

    @Test
    fun limitsWatchLibraryToTwelveBooksWhileReportingFullCount() {
        val directory = Files.createTempDirectory("library-payload-limit-test").toFile()
        try {
            val books = (1..14).map { index ->
                LibraryBook(
                    UUID.randomUUID(),
                    revision = 1,
                    coverRevision = 0,
                    title = "Book $index",
                    authors = "Author",
                    tokenCount = 100,
                    progressTokenIndex = 0,
                    progressPercentage = 0,
                )
            }

            val payload = LibraryPayloadEncoder.encode(books, CoverCache(directory))

            assertEquals(12 + 12 * 1068, payload.size)
            assertEquals(12, payload[5].toInt())
            assertEquals(14, java.nio.ByteBuffer.wrap(payload, 8, 4).int)
        } finally {
            directory.deleteRecursively()
        }
    }
}
