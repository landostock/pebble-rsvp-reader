package dev.pebblersvp.android.cache

import dev.pebblersvp.android.sync.LibraryManifest
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import java.nio.file.Files

class LibraryManifestCacheTest {
    @Test
    fun updatesReaderPreferencesWithoutChangingServerCacheIdentity() {
        val directory = Files.createTempDirectory("library-settings-cache-test").toFile()
        try {
            val cache = LibraryManifestCache(directory)
            cache.write(
                """{"library_revision":1,"books":[],"settings":{"language":"en","library_sort":"last_read"}}""",
                "library-etag",
                "2026-09-01T10:00:00.000Z",
            )

            assertTrue(cache.updateReaderSettings("de", "author"))
            val updated = cache.read()!!
            val manifest = LibraryManifest.parse(updated.json)
            assertEquals("de", manifest.preferences.language)
            assertEquals("author", manifest.preferences.librarySort)
            assertEquals("library-etag", updated.etag)
        } finally {
            directory.deleteRecursively()
        }
    }
}
