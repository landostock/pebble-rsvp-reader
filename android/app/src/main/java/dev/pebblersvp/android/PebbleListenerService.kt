package dev.pebblersvp.android

import android.util.Log
import dev.pebblersvp.android.protocol.MessageType
import dev.pebblersvp.android.protocol.RsvpPacket
import dev.pebblersvp.android.protocol.TransferReceiver
import dev.pebblersvp.android.protocol.TransferStatus
import dev.pebblersvp.android.cache.ReaderPackageCache
import dev.pebblersvp.android.cache.CoverCache
import dev.pebblersvp.android.cache.LibraryManifestCache
import dev.pebblersvp.android.cache.CachedProgress
import dev.pebblersvp.android.cache.ProgressCache
import dev.pebblersvp.android.pairing.PairingStore
import dev.pebblersvp.android.sync.LibraryBook
import dev.pebblersvp.android.sync.LibraryManifest
import dev.pebblersvp.android.sync.LibrarySynchronizer
import dev.pebblersvp.android.sync.ServerClient
import dev.pebblersvp.android.transport.PebblePacketBridge
import dev.pebblersvp.android.transport.LibraryPayloadEncoder
import dev.pebblersvp.android.transport.ChapterPayloadEncoder
import dev.pebblersvp.android.transport.ReaderWindowEncoder
import dev.pebblersvp.android.transport.ReaderWindowStartMode
import io.rebble.pebblekit2.client.BasePebbleListenerService
import io.rebble.pebblekit2.client.DefaultPebbleAndroidAppPicker
import io.rebble.pebblekit2.common.model.PebbleDictionaryItem
import io.rebble.pebblekit2.common.model.ReceiveResult
import io.rebble.pebblekit2.common.model.WatchIdentifier
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlinx.coroutines.withContext
import java.nio.charset.StandardCharsets
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.TimeZone
import java.util.UUID

class PebbleListenerService : BasePebbleListenerService() {
    private val receiver = TransferReceiver()
    private val serviceScope = CoroutineScope(SupervisorJob() + Dispatchers.IO)
    private val outgoingTransferMutex = Mutex()
    private val readerSettingsMutex = Mutex()
    private val progressUploadMutex = Mutex()
    private val requestStateLock = Any()
    private val queuedReaderRequests = mutableSetOf<String>()
    private val completedReaderRequests = ArrayDeque<String>()
    private val packetBridgeDelegate = lazy { PebblePacketBridge(this) }
    private val packetBridge by packetBridgeDelegate
    private val packageCacheDelegate = lazy { ReaderPackageCache(this) }
    private val packageCache by packageCacheDelegate

    override fun onCreate() {
        super.onCreate()
        DefaultPebbleAndroidAppPicker.getInstance(this).enableAutoSelect = false
    }

    override suspend fun onMessageReceived(
        watchappUUID: UUID,
        data: Map<UInt, PebbleDictionaryItem>,
        watch: WatchIdentifier,
    ): ReceiveResult {
        if (watchappUUID != AppConstants.WatchAppUuid) {
            Log.w(Tag, "Rejected message for unexpected watch app UUID $watchappUUID")
            return ReceiveResult.Nack
        }
        val packetItem = data[AppConstants.PacketKey] as? PebbleDictionaryItem.Bytes
        if (packetItem == null) {
            Log.w(Tag, "Received message without Packet key from $watch")
            return ReceiveResult.Nack
        }

        val decodedPacket = runCatching { RsvpPacket.decode(packetItem.value) }.getOrNull()
        if (decodedPacket?.messageType == MessageType.Request) {
            if (String(decodedPacket.payload, StandardCharsets.UTF_8) == "sync") {
                serviceScope.launch { syncAndSendLibrary(watch, sendCachedFirst = true) }
                return ReceiveResult.Ack
            }
            enqueueRequestedWindow(decodedPacket, watch)
            return ReceiveResult.Ack
        }
        if (decodedPacket?.messageType == MessageType.Progress) {
            recordProgress(decodedPacket)?.let { progress ->
                serviceScope.launch { uploadProgress(progress) }
            }
            return ReceiveResult.Ack
        }
        if (decodedPacket?.messageType == MessageType.Settings) {
            val settings = parseReaderSettings(decodedPacket.payload) ?: return ReceiveResult.Nack
            serviceScope.launch { updateReaderSettings(settings.first, settings.second, watch) }
            return ReceiveResult.Ack
        }

        val receipt = receiver.receive(packetItem.value)
        Log.d(Tag, "Received packet status=${receipt.status} from $watch")

        receipt.response?.let { response ->
            runCatching { packetBridge.sendPacketReliably(response, watch) }
                .onFailure { Log.w(Tag, "Failed to send transfer response", it) }
        }

        return when (receipt.status) {
            TransferStatus.Nack -> ReceiveResult.Nack
            else -> ReceiveResult.Ack
        }
    }

    override fun onAppOpened(watchappUUID: UUID, watch: WatchIdentifier) {
        if (watchappUUID != AppConstants.WatchAppUuid) return
        Log.d(Tag, "Watch app opened on $watch")
        serviceScope.launch {
            syncAndSendLibrary(watch, sendCachedFirst = true)
        }
    }

    override fun onAppClosed(watchappUUID: UUID, watch: WatchIdentifier) {
        if (watchappUUID != AppConstants.WatchAppUuid) return
        Log.d(Tag, "Watch app closed on $watch")
    }

    override fun onDestroy() {
        serviceScope.cancel()
        if (packetBridgeDelegate.isInitialized()) packetBridge.close()
        super.onDestroy()
    }

    private suspend fun sendLibrary(watch: WatchIdentifier) {
        outgoingTransferMutex.withLock {
            val cached = LibraryManifestCache(this).read() ?: return@withLock
            val manifest = LibraryManifest.parse(cached.json)
            val books = manifest.orderedBooks(manifest.books.map(::withCachedProgress))
            val payload = LibraryPayloadEncoder.encode(
                books,
                CoverCache(this),
                manifest.preferences.language,
                manifest.preferences.librarySort,
            )
            val packets = RsvpPacket.splitPayload(
                requestId = System.currentTimeMillis() and 0xffffffffL,
                bookId = RsvpPacket.ZeroUuid,
                bookRevision = 0,
                chunkId = 1,
                payload = payload,
                maxPayloadLength = AppConstants.DefaultPacketPayloadBytes,
            )
            packets.forEach { data ->
                packetBridge.sendPacketReliably(data, watch)
            }
            Log.d(Tag, "Sent library list books=${books.size.coerceAtMost(LibraryPayloadEncoder.MaxBooks)} to $watch")
        }
    }

    private suspend fun syncAndSendLibrary(watch: WatchIdentifier, sendCachedFirst: Boolean) {
        if (sendCachedFirst) sendLibrary(watch)
        val credentials = runCatching { PairingStore(this@PebbleListenerService).load() }.getOrNull()
            ?: return
        runCatching {
            readerSettingsMutex.withLock { retryPendingReaderSettings(credentials) }
        }.onFailure { error ->
            Log.w(Tag, "Pending reader settings upload failed", error)
        }
        runCatching {
            LibrarySynchronizer(
                LibraryManifestCache(this@PebbleListenerService),
                packageCache,
                CoverCache(this@PebbleListenerService),
                ProgressCache(this@PebbleListenerService),
            ).sync(credentials)
        }.onSuccess {
            // Always resend: cached progress may have changed even when the server
            // manifest revision itself did not.
            sendLibrary(watch)
        }.onFailure { error ->
            Log.w(Tag, "Background library sync failed; cached library remains available", error)
        }
    }

    private fun enqueueRequestedWindow(packet: RsvpPacket, watch: WatchIdentifier) {
        val key = buildString {
            append(watch)
            append(':').append(packet.requestId)
            append(':').append(packet.bookId)
            append(':').append(packet.bookRevision)
            append(':').append(packet.chunkId)
            append(':').append(packet.payload.contentHashCode())
        }
        synchronized(requestStateLock) {
            if (key in queuedReaderRequests || key in completedReaderRequests) {
                Log.d(Tag, "Ignored duplicate reader request id=${packet.requestId} chunk=${packet.chunkId}")
                return
            }
            queuedReaderRequests += key
        }
        serviceScope.launch {
            try {
                delay(RequestAckGraceMs)
                outgoingTransferMutex.withLock {
                    sendRequestedWindow(packet, watch)
                }
            } finally {
                synchronized(requestStateLock) {
                    queuedReaderRequests -= key
                    completedReaderRequests += key
                    while (completedReaderRequests.size > RecentRequestLimit) {
                        completedReaderRequests.removeFirst()
                    }
                }
            }
        }
    }

    private suspend fun sendRequestedWindow(packet: RsvpPacket, watch: WatchIdentifier) {
        val requestText = String(packet.payload, StandardCharsets.UTF_8)
        val manifestBook = LibraryManifestCache(this).read()?.let { cached ->
            LibraryManifest.parse(cached.json).books.firstOrNull {
                it.bookId == packet.bookId && it.revision == packet.bookRevision
            }
        }
        if (!packageCache.has(packet.bookId, packet.bookRevision)) {
            Log.w(Tag, "Requested reader package is not cached: ${packet.bookId} r${packet.bookRevision}")
            return
        }
        if (requestText == "chapters") {
            val packageBytes = packageCache.read(packet.bookId, packet.bookRevision) ?: return
            sendPayload(packet, watch, ChapterPayloadEncoder.encode(packageBytes))
            return
        }
        val cachedStart = ProgressCache(this).read(packet.bookId)?.tokenIndex
            ?: manifestBook?.progressTokenIndex
            ?: 0
        val request = when {
            requestText == "book" -> cachedStart to ReaderWindowStartMode.SentenceStart
            requestText.startsWith("resume:") -> requestText.removePrefix("resume:").toIntOrNull()
                ?.let { it to ReaderWindowStartMode.SentenceStart }
            requestText.startsWith("jump:") -> requestText.removePrefix("jump:").toIntOrNull()
                ?.let { it to ReaderWindowStartMode.ExactWithContext }
            requestText.startsWith("rewind:") -> requestText.removePrefix("rewind:").toIntOrNull()
                ?.let { it to ReaderWindowStartMode.PreviousSentence }
            requestText.startsWith("next:") -> requestText.removePrefix("next:").toIntOrNull()
                ?.let { it to ReaderWindowStartMode.NextSentence }
            requestText.startsWith("window:") -> requestText.removePrefix("window:").toIntOrNull()
                ?.let { it to ReaderWindowStartMode.Exact }
            else -> null
        }
        if (request == null || request.first < 0) {
            Log.w(Tag, "Ignoring malformed reader request from $watch")
            return
        }
        var (startWordIndex, startMode) = request

        try {
            val window = withContext(Dispatchers.IO) {
                val index = packageCache.openIndex(packet.bookId, packet.bookRevision)
                    ?: error("reader package index is unavailable")
                if (startWordIndex == 0 && manifestBook?.progressUpdatedAt.isNullOrBlank() &&
                    index.recommendedStartTokenIndex > 0
                ) {
                    startWordIndex = index.recommendedStartTokenIndex
                    startMode = ReaderWindowStartMode.SentenceStart
                }
                ReaderWindowEncoder.encode(
                    packet.bookId,
                    packet.bookRevision,
                    index,
                    startWordIndex,
                    titleOverride = manifestBook?.title,
                    startMode = startMode,
                )
            }
            sendPayload(packet, watch, window.payload)
            Log.d(Tag, "Sent requested reader window start=${window.startWordIndex} focus=${window.focusWordIndex} words=${window.wordCount} to $watch")
        } catch (error: Exception) {
            Log.e(Tag, "Failed to send requested reader window to $watch", error)
        }
    }

    private suspend fun sendPayload(packet: RsvpPacket, watch: WatchIdentifier, payload: ByteArray): Int {
        val packets = RsvpPacket.splitPayload(
            requestId = packet.requestId,
            bookId = packet.bookId,
            bookRevision = packet.bookRevision,
            chunkId = packet.chunkId,
            payload = payload,
            maxPayloadLength = AppConstants.DefaultPacketPayloadBytes,
        )
        packets.forEach { data ->
            packetBridge.sendPacketReliably(data, watch)
        }
        return packets.size
    }

    private fun withCachedProgress(book: LibraryBook): LibraryBook {
        val cached = ProgressCache(this).read(book.bookId) ?: return book
        if (cached.bookRevision != book.revision || cached.updatedAt.isBlank() ||
            (book.progressUpdatedAt.isNotBlank() && cached.updatedAt < book.progressUpdatedAt)
        ) return book
        return book.copy(
            tokenCount = maxOf(book.tokenCount, cached.totalTokenCount),
            progressTokenIndex = cached.tokenIndex,
            progressFurthestTokenIndex = cached.furthestTokenIndex,
            progressPercentage = cached.percentage,
            progressUpdatedAt = cached.updatedAt,
            progressReset = cached.reset,
        )
    }

    private fun parseReaderSettings(payload: ByteArray): Pair<String, String>? {
        if (payload.size != 3 || payload[0].toInt() != 1) return null
        val language = when (payload[1].toInt()) {
            0 -> "en"
            1 -> "de"
            else -> return null
        }
        val librarySort = when (payload[2].toInt()) {
            0 -> "last_read"
            1 -> "title"
            2 -> "author"
            3 -> "recent"
            4 -> "progress"
            else -> return null
        }
        return language to librarySort
    }

    private suspend fun updateReaderSettings(language: String, librarySort: String, watch: WatchIdentifier) {
        readerSettingsMutex.withLock {
            LibraryManifestCache(this@PebbleListenerService).updateReaderSettings(language, librarySort)
            savePendingReaderSettings(language, librarySort)
            sendLibrary(watch)
            val credentials = runCatching { PairingStore(this@PebbleListenerService).load() }.getOrNull()
                ?: return@withLock
            runCatching {
                ServerClient(credentials).updateReaderSettings(language, librarySort)
                clearPendingReaderSettings()
                LibrarySynchronizer(
                    LibraryManifestCache(this@PebbleListenerService),
                    packageCache,
                    CoverCache(this@PebbleListenerService),
                    ProgressCache(this@PebbleListenerService),
                ).sync(credentials)
                sendLibrary(watch)
            }.onFailure { error ->
                Log.w(Tag, "Reader settings sync failed", error)
            }
        }
    }

    private fun recordProgress(packet: RsvpPacket): CachedProgress? {
        if (packet.bookId == RsvpPacket.ZeroUuid || packet.bookRevision <= 0 ||
            (packet.payload.size != 8 && packet.payload.size != 12)
        ) {
            Log.w(Tag, "Ignoring malformed progress packet")
            return null
        }
        val payload = ByteBuffer.wrap(packet.payload).order(ByteOrder.BIG_ENDIAN)
        val rawIndex = payload.int.toLong() and 0xffffffffL
        val rawTotal = payload.int.toLong() and 0xffffffffL
        val rawFurthest = if (packet.payload.size >= 12) payload.int.toLong() and 0xffffffffL else rawIndex
        if (rawIndex > Int.MAX_VALUE || rawFurthest > Int.MAX_VALUE || rawTotal > Int.MAX_VALUE || rawTotal <= 0) {
            Log.w(Tag, "Ignoring out-of-range progress packet")
            return null
        }
        val total = rawTotal.toInt()
        val tokenIndex = rawIndex.toInt().coerceIn(0, total - 1)
        val furthestTokenIndex = rawFurthest.toInt().coerceIn(tokenIndex, total - 1)
        return ProgressCache(this).write(
            CachedProgress(
                bookId = packet.bookId,
                bookRevision = packet.bookRevision,
                tokenIndex = tokenIndex,
                furthestTokenIndex = furthestTokenIndex,
                totalTokenCount = total,
                percentage = if (total <= 1) 0 else
                    ((tokenIndex.toLong() * 100L) / (total - 1)).toInt().coerceIn(0, 100),
                updatedAt = nowIso(),
            ),
        )
    }

    private suspend fun uploadProgress(progress: CachedProgress) {
        progressUploadMutex.withLock {
            val credentials = runCatching { PairingStore(this@PebbleListenerService).load() }.getOrNull()
                ?: return@withLock
            val latest = ProgressCache(this@PebbleListenerService).read(progress.bookId) ?: progress
            runCatching { ServerClient(credentials).uploadProgress(latest) }
                .onFailure { Log.w(Tag, "Progress saved locally but server upload failed", it) }
        }
    }

    private fun savePendingReaderSettings(language: String, librarySort: String) {
        getSharedPreferences(ReaderSettingsPreferences, MODE_PRIVATE).edit()
            .putString(PendingLanguage, language)
            .putString(PendingLibrarySort, librarySort)
            .apply()
    }

    private fun clearPendingReaderSettings() {
        getSharedPreferences(ReaderSettingsPreferences, MODE_PRIVATE).edit().clear().apply()
    }

    private fun retryPendingReaderSettings(credentials: dev.pebblersvp.android.pairing.PairingCredentials) {
        val preferences = getSharedPreferences(ReaderSettingsPreferences, MODE_PRIVATE)
        val language = preferences.getString(PendingLanguage, null) ?: return
        val librarySort = preferences.getString(PendingLibrarySort, null) ?: return
        ServerClient(credentials).updateReaderSettings(language, librarySort)
        clearPendingReaderSettings()
    }

    private fun nowIso(): String =
        SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'", Locale.US).apply {
            timeZone = TimeZone.getTimeZone("UTC")
        }.format(Date())

    private companion object {
        const val Tag = "PebbleRsvpListener"
        const val RecentRequestLimit = 24
        const val RequestAckGraceMs = 25L
        const val ReaderSettingsPreferences = "reader-settings-outbox"
        const val PendingLanguage = "language"
        const val PendingLibrarySort = "library-sort"
    }
}
