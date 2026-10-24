package dev.pebblersvp.android.cache

import android.content.Context
import org.json.JSONObject
import java.io.File
import java.io.IOException

data class CachedLibraryManifest(
    val json: String,
    val etag: String,
    val syncedAt: String,
)

class LibraryManifestCache(private val root: File) {
    constructor(context: Context) : this(File(context.filesDir, "library-manifest"))

    fun read(): CachedLibraryManifest? {
        val manifest = File(root, ManifestFile)
        val state = File(root, StateFile)
        if (!manifest.isFile || !state.isFile) return null
        val metadata = JSONObject(state.readText(Charsets.UTF_8))
        return CachedLibraryManifest(
            json = manifest.readText(Charsets.UTF_8),
            etag = metadata.getString("etag"),
            syncedAt = metadata.getString("synced_at"),
        )
    }

    fun write(json: String, etag: String, syncedAt: String) {
        root.mkdirs()
        writeAtomic(File(root, ManifestFile), json.toByteArray(Charsets.UTF_8))
        writeAtomic(
            File(root, StateFile),
            JSONObject().put("etag", etag).put("synced_at", syncedAt).toString().toByteArray(Charsets.UTF_8),
        )
    }

    fun updateReaderSettings(language: String, librarySort: String): Boolean {
        require(language in setOf("en", "de"))
        require(librarySort in setOf("last_read", "title", "author", "recent", "progress"))
        val cached = read() ?: return false
        val rootJson = JSONObject(cached.json)
        val settings = rootJson.optJSONObject("settings") ?: JSONObject().also {
            rootJson.put("settings", it)
        }
        settings.put("language", language)
        settings.put("library_sort", librarySort)
        write(rootJson.toString(), cached.etag, cached.syncedAt)
        return true
    }

    fun clear() {
        root.deleteRecursively()
    }

    private fun writeAtomic(target: File, bytes: ByteArray) {
        val temporary = File(target.parentFile, "${target.name}.tmp")
        temporary.writeBytes(bytes)
        if (!temporary.renameTo(target)) {
            temporary.delete()
            throw IOException("failed to atomically publish library manifest cache")
        }
    }

    private companion object {
        const val ManifestFile = "library.json"
        const val StateFile = "state.json"
    }
}
