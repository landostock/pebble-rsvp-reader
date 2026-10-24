package dev.pebblersvp.android.cache

import android.content.Context
import java.io.File
import java.io.IOException
import java.util.UUID

class CoverCache(private val root: File) {
    constructor(context: Context) : this(File(context.filesDir, "covers"))

    init {
        root.mkdirs()
    }

    fun has(bookId: UUID, revision: Long): Boolean =
        fileFor(bookId, revision).isFile

    fun read(bookId: UUID, revision: Long): ByteArray? =
        fileFor(bookId, revision).takeIf(File::isFile)?.readBytes()

    fun write(bookId: UUID, revision: Long, bytes: ByteArray) {
        val target = fileFor(bookId, revision)
        target.parentFile?.mkdirs()
        val temporary = File(target.parentFile, "${target.name}.tmp")
        temporary.writeBytes(bytes)
        if (!temporary.renameTo(target)) {
            temporary.delete()
            throw IOException("failed to atomically publish cover cache entry")
        }
    }

    fun clear() {
        root.deleteRecursively()
        root.mkdirs()
    }

    fun approximateBytes(): Long =
        root.walkTopDown().filter(File::isFile).sumOf(File::length)

    fun entryCount(): Int =
        root.walkTopDown().count { it.isFile && it.name.startsWith(CoverPrefix) }

    fun prune(activeRevisions: Map<UUID, Long>) {
        root.listFiles()?.filter(File::isDirectory)?.forEach { bookDirectory ->
            val bookId = runCatching { UUID.fromString(bookDirectory.name) }.getOrNull()
            val activeRevision = bookId?.let(activeRevisions::get)
            if (activeRevision == null || activeRevision <= 0) {
                bookDirectory.deleteRecursively()
                return@forEach
            }
            bookDirectory.listFiles()?.forEach { file ->
                if (file.name != "$CoverPrefix$activeRevision.bin") {
                    file.delete()
                }
            }
        }
    }

    private fun fileFor(bookId: UUID, revision: Long): File =
        File(File(root, bookId.toString()), "$CoverPrefix$revision.bin")

    private companion object {
        const val CoverPrefix = "cover-r"
    }
}
