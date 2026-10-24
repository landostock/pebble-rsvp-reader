package dev.pebblersvp.android.sync

import dev.pebblersvp.android.cache.CoverCache
import dev.pebblersvp.android.cache.LibraryManifestCache
import dev.pebblersvp.android.cache.ReaderPackageCache
import dev.pebblersvp.android.cache.CachedProgress
import dev.pebblersvp.android.cache.ProgressCache
import dev.pebblersvp.android.pairing.PairingCredentials
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.TimeZone

data class SyncResult(
    val changed: Boolean,
    val bookCount: Int,
    val downloadedPackages: Int,
    val downloadedCovers: Int,
)

class LibrarySynchronizer(
    private val manifestCache: LibraryManifestCache,
    private val packageCache: ReaderPackageCache,
    private val coverCache: CoverCache,
    private val progressCache: ProgressCache,
) {
    fun sync(credentials: PairingCredentials): SyncResult {
        val cached = manifestCache.read()
        val response = ServerClient(credentials).fetchLibrary(cached?.etag)
        val result = when (response) {
            LibraryResponse.NotModified -> {
                val manifest = cached?.let { LibraryManifest.parse(it.json) }
                    ?: error("Server returned not modified without a local manifest")
                mergeServerProgress(manifest)
                SyncResult(false, manifest.books.size, 0, 0)
            }
            is LibraryResponse.Changed -> publishChangedLibrary(credentials, response)
        }
        val manifest = manifestCache.read()?.let { LibraryManifest.parse(it.json) }
            ?: error("Synchronized library is missing from the local cache")
        uploadPendingProgress(credentials, manifest)
        return result
    }

    fun clear() {
        manifestCache.clear()
        packageCache.clear()
        coverCache.clear()
        progressCache.clear()
    }

    private fun publishChangedLibrary(
        credentials: PairingCredentials,
        response: LibraryResponse.Changed,
    ): SyncResult {
        val manifest = LibraryManifest.parse(response.json)
        val client = ServerClient(credentials)
        var downloadedPackages = 0
        var downloadedCovers = 0

        for (book in manifest.books) {
            if (!packageCache.has(book.bookId, book.revision)) {
                packageCache.write(
                    book.bookId,
                    book.revision,
                    client.downloadReaderPackage(book.bookId.toString()),
                )
                downloadedPackages += 1
            }
            packageCache.openIndex(book.bookId, book.revision)
            if (book.coverRevision > 0 && !coverCache.has(book.bookId, book.coverRevision)) {
                coverCache.write(
                    book.bookId,
                    book.coverRevision,
                    client.downloadCover(book.bookId.toString()),
                )
                downloadedCovers += 1
            }
        }

        packageCache.prune(manifest.books.associate { it.bookId to it.revision })
        coverCache.prune(manifest.books.associate { it.bookId to it.coverRevision })
        mergeServerProgress(manifest)
        progressCache.prune(manifest.books.mapTo(mutableSetOf()) { it.bookId })
        manifestCache.write(response.json, response.etag, nowIso())
        return SyncResult(true, manifest.books.size, downloadedPackages, downloadedCovers)
    }

    private fun mergeServerProgress(manifest: LibraryManifest) {
        manifest.books.forEach { book ->
            mergedProgressForBook(progressCache.read(book.bookId), book)?.let(progressCache::write)
        }
    }

    private fun uploadPendingProgress(credentials: PairingCredentials, manifest: LibraryManifest) {
        val client = ServerClient(credentials)
        manifest.books.forEach { book ->
            val local = progressCache.read(book.bookId) ?: return@forEach
            if (local.bookRevision == book.revision && local.updatedAt.isNotBlank() &&
                (book.progressUpdatedAt.isBlank() || local.updatedAt > book.progressUpdatedAt)
            ) {
                client.uploadProgress(local)
            }
        }
    }

    private fun nowIso(): String =
        SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'", Locale.US).apply {
            timeZone = TimeZone.getTimeZone("UTC")
        }.format(Date())
}

internal fun mergedProgressForBook(local: CachedProgress?, book: LibraryBook): CachedProgress? {
    val maximumIndex = (book.tokenCount - 1).coerceAtLeast(0)
    val server = book.progressUpdatedAt.takeIf(String::isNotBlank)?.let {
        CachedProgress(
            bookId = book.bookId,
            bookRevision = book.revision,
            tokenIndex = book.progressTokenIndex.coerceIn(0, maximumIndex),
            furthestTokenIndex = book.progressFurthestTokenIndex.coerceIn(0, maximumIndex),
            totalTokenCount = book.tokenCount,
            percentage = book.progressPercentage,
            updatedAt = book.progressUpdatedAt,
            reset = book.progressReset,
        )
    }
    val compatibleLocal = local?.takeIf {
        it.bookId == book.bookId && it.totalTokenCount == book.tokenCount && it.updatedAt.isNotBlank()
    }?.let { candidate ->
        candidate.copy(
            bookRevision = book.revision,
            tokenIndex = candidate.tokenIndex.coerceIn(0, maximumIndex),
            furthestTokenIndex = candidate.furthestTokenIndex.coerceIn(0, maximumIndex),
        )
    }
    return when {
        compatibleLocal == null -> server
        server == null || compatibleLocal.updatedAt > server.updatedAt -> compatibleLocal
        else -> server
    }
}
