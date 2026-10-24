package dev.pebblersvp.android.cache

import android.content.Context
import org.json.JSONObject
import java.io.File
import java.io.IOException
import java.text.SimpleDateFormat
import java.util.Locale
import java.util.TimeZone
import java.util.UUID

data class CachedProgress(
    val bookId: UUID,
    val bookRevision: Long,
    val tokenIndex: Int,
    val totalTokenCount: Int,
    val percentage: Int,
    val updatedAt: String,
    val furthestTokenIndex: Int = tokenIndex,
    val reset: Boolean = false,
)

class ProgressCache(private val root: File) {
    constructor(context: Context) : this(File(context.filesDir, "progress"))

    init {
        root.mkdirs()
    }

    fun read(bookId: UUID): CachedProgress? {
        val file = fileFor(bookId)
        if (!file.isFile) return null
        return runCatching {
            val json = JSONObject(file.readText(Charsets.UTF_8))
            CachedProgress(
                bookId = UUID.fromString(json.getString("book_id")),
                bookRevision = json.getLong("book_revision"),
                tokenIndex = json.getInt("token_index"),
                furthestTokenIndex = json.optInt("furthest_token_index", json.getInt("token_index")),
                totalTokenCount = json.getInt("total_token_count"),
                percentage = json.getInt("percentage").coerceIn(0, 100),
                updatedAt = normalizedTimestamp(json.optString("updated_at", "")),
                reset = json.optBoolean("reset", false),
            )
        }.getOrNull()?.takeIf { it.bookId == bookId }
    }

    fun write(progress: CachedProgress): CachedProgress {
        require(progress.bookRevision > 0)
        require(progress.tokenIndex >= 0)
        require(progress.totalTokenCount >= 0)
        val existing = read(progress.bookId)
        val incomingTimestamp = normalizedTimestamp(progress.updatedAt)
        if (existing != null && existing.bookRevision == progress.bookRevision &&
            existing.updatedAt.isNotBlank() &&
            (incomingTimestamp.isBlank() || existing.updatedAt > incomingTimestamp)
        ) {
            return existing
        }
        val tokenIndex = if (progress.totalTokenCount > 0) {
            progress.tokenIndex.coerceAtMost(progress.totalTokenCount - 1)
        } else {
            0
        }
        val furthestTokenIndex = if (progress.reset) {
            0
        } else if (progress.totalTokenCount > 0) {
            maxOf(
                progress.furthestTokenIndex,
                progress.tokenIndex,
                existing?.takeIf { it.bookRevision == progress.bookRevision }?.furthestTokenIndex ?: 0,
            ).coerceAtMost(progress.totalTokenCount - 1)
        } else {
            0
        }
        val canonical = progress.copy(
            tokenIndex = tokenIndex,
            furthestTokenIndex = furthestTokenIndex,
            percentage = if (progress.totalTokenCount <= 1) 0 else
                ((furthestTokenIndex.toLong() * 100L) / (progress.totalTokenCount - 1)).toInt().coerceIn(0, 100),
            updatedAt = incomingTimestamp,
        )
        val json = JSONObject()
            .put("book_id", canonical.bookId.toString())
            .put("book_revision", canonical.bookRevision)
            .put("token_index", canonical.tokenIndex)
            .put("furthest_token_index", canonical.furthestTokenIndex)
            .put("total_token_count", canonical.totalTokenCount)
            .put("percentage", canonical.percentage)
            .put("updated_at", canonical.updatedAt)
            .put("reset", canonical.reset)
        publish(fileFor(progress.bookId), json.toString().toByteArray(Charsets.UTF_8))
        return canonical
    }

    fun prune(activeBookIds: Set<UUID>) {
        root.listFiles()?.filter(File::isFile)?.forEach { file ->
            val bookId = runCatching { UUID.fromString(file.nameWithoutExtension) }.getOrNull()
            if (bookId == null || bookId !in activeBookIds) file.delete()
        }
    }

    fun clear() {
        root.deleteRecursively()
        root.mkdirs()
    }

    private fun publish(target: File, bytes: ByteArray) {
        root.mkdirs()
        val temporary = File(root, "${target.name}.tmp")
        temporary.writeBytes(bytes)
        if (!temporary.renameTo(target)) {
            temporary.delete()
            throw IOException("failed to atomically publish progress cache entry")
        }
    }

    private fun fileFor(bookId: UUID): File = File(root, "$bookId.json")

    private fun normalizedTimestamp(value: String): String {
        if (value.isBlank() || value == "null") return ""
        val parsed = runCatching {
            SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'", Locale.US).apply {
                isLenient = false
                timeZone = TimeZone.getTimeZone("UTC")
            }.parse(value)
        }.getOrNull() ?: return ""
        return if (parsed.time >= 0) value else ""
    }
}
