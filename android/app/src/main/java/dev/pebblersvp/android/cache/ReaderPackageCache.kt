package dev.pebblersvp.android.cache

import android.content.Context
import java.io.File
import java.io.IOException
import java.util.UUID

class ReaderPackageCache(private val root: File) {
    constructor(context: Context) : this(File(context.filesDir, "reader-packages"))

    init {
        root.mkdirs()
    }

    private val openIndexes = object : LinkedHashMap<String, ReaderPackageIndex>(3, 0.75f, true) {
        override fun removeEldestEntry(eldest: MutableMap.MutableEntry<String, ReaderPackageIndex>?): Boolean =
            size > MaxOpenIndexes
    }

    fun has(bookId: UUID, revision: Long): Boolean =
        fileFor(bookId, revision).isFile

    fun read(bookId: UUID, revision: Long): ByteArray? =
        fileFor(bookId, revision).takeIf(File::isFile)?.readBytes()

    @Synchronized
    fun openIndex(bookId: UUID, revision: Long): ReaderPackageIndex? {
        val key = "$bookId:$revision"
        val source = fileFor(bookId, revision).takeIf(File::isFile) ?: return null
        val index = indexFileFor(bookId, revision)
        if (!index.isFile || index.lastModified() < source.lastModified() ||
            runCatching { ReaderPackageIndex.open(index) }.isFailure
        ) {
            ReaderPackageIndex.build(source.readBytes(), index)
        }
        return openIndexes[key] ?: ReaderPackageIndex.open(index).also { openIndexes[key] = it }
    }

    fun write(bookId: UUID, revision: Long, bytes: ByteArray) {
        root.mkdirs()
        val target = fileFor(bookId, revision)
        target.parentFile?.mkdirs()
        val temporary = File(target.parentFile, "${target.name}.tmp")
        temporary.writeBytes(bytes)
        if (!temporary.renameTo(target)) {
            temporary.delete()
            throw IOException("failed to atomically publish reader package cache entry")
        }
    }

    fun clear() {
        synchronized(this) { openIndexes.clear() }
        root.deleteRecursively()
        root.mkdirs()
    }

    fun approximateBytes(): Long =
        root.walkTopDown().filter(File::isFile).sumOf(File::length)

    fun entryCount(): Int =
        root.walkTopDown().count { it.isFile && it.name.startsWith(PackagePrefix) }

    fun prune(activeRevisions: Map<UUID, Long>) {
        synchronized(this) {
            openIndexes.keys.removeAll { key ->
                val separator = key.lastIndexOf(':')
                val bookId = runCatching { UUID.fromString(key.substring(0, separator)) }.getOrNull()
                val revision = key.substring(separator + 1).toLongOrNull()
                bookId == null || revision == null || activeRevisions[bookId] != revision
            }
        }
        root.listFiles()?.filter(File::isDirectory)?.forEach { bookDirectory ->
            val bookId = runCatching { UUID.fromString(bookDirectory.name) }.getOrNull()
            val activeRevision = bookId?.let(activeRevisions::get)
            if (activeRevision == null) {
                bookDirectory.deleteRecursively()
                return@forEach
            }
            bookDirectory.listFiles()?.forEach { file ->
                if (file.name != "$PackagePrefix$activeRevision.bin" &&
                    file.name != "$IndexPrefix$activeRevision.bin"
                ) {
                    file.delete()
                }
            }
        }
    }

    private fun fileFor(bookId: UUID, revision: Long): File {
        val bookDirectory = File(root, bookId.toString())
        return File(bookDirectory, "$PackagePrefix$revision.bin")
    }

    private fun indexFileFor(bookId: UUID, revision: Long): File {
        val bookDirectory = File(root, bookId.toString())
        return File(bookDirectory, "$IndexPrefix$revision.bin")
    }

    private companion object {
        const val PackagePrefix = "reader-package-r"
        const val IndexPrefix = "reader-index-r"
        const val MaxOpenIndexes = 2
    }
}
