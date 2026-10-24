package dev.pebblersvp.android.cache

import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test
import java.nio.file.Files
import java.nio.charset.StandardCharsets
import java.util.UUID

class ReaderPackageCacheTest {
    @Test
    fun reportsMissThenReturnsWrittenPackage() = withCache { cache ->
        val bookId = UUID.randomUUID()
        val payload = byteArrayOf(1, 3, 3, 7)

        assertFalse(cache.has(bookId, 4))
        assertNull(cache.read(bookId, 4))

        cache.write(bookId, 4, payload)

        assertTrue(cache.has(bookId, 4))
        assertArrayEquals(payload, cache.read(bookId, 4))
        assertEquals(1, cache.entryCount())
    }

    @Test
    fun pruneKeepsOnlyCurrentRevisionsAndActiveBooks() = withCache { cache ->
        val activeBook = UUID.randomUUID()
        val deletedBook = UUID.randomUUID()
        cache.write(activeBook, 1, byteArrayOf(1))
        cache.write(activeBook, 2, byteArrayOf(2))
        cache.write(deletedBook, 1, byteArrayOf(3))

        cache.prune(mapOf(activeBook to 2L))

        assertFalse(cache.has(activeBook, 1))
        assertTrue(cache.has(activeBook, 2))
        assertFalse(cache.has(deletedBook, 1))
        assertEquals(1, cache.entryCount())
    }

    @Test
    fun buildsACompactRandomAccessIndexWithoutChangingTheSourcePackage() = withCache { cache ->
        val bookId = UUID.randomUUID()
        val payload = """
            {"metadata":{"title":"Test","recommended_start_token_index":1},"tokens":[
              {"display":"Eins.","sentence_id":0,"duration_ms_at_300wpm":250},
              {"display":"Zwei","sentence_id":1,"paragraph_end":true,"focus_char_index":1}
            ]}
        """.trimIndent().toByteArray(StandardCharsets.UTF_8)
        cache.write(bookId, 3, payload)

        val index = requireNotNull(cache.openIndex(bookId, 3))

        assertEquals(2, index.tokenCount)
        assertEquals(1, index.recommendedStartTokenIndex)
        assertEquals("Zwei", index.token(1).display)
        assertTrue(index.token(1).flags and ReaderPackageIndex.FlagSentenceStart != 0)
        assertArrayEquals(payload, cache.read(bookId, 3))
    }

    private fun withCache(assertions: (ReaderPackageCache) -> Unit) {
        val directory = Files.createTempDirectory("reader-package-cache-test").toFile()
        try {
            assertions(ReaderPackageCache(directory))
        } finally {
            directory.deleteRecursively()
        }
    }
}
