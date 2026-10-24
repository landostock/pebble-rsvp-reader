package dev.pebblersvp.android

import android.app.Activity
import android.app.AlertDialog
import android.content.res.ColorStateList
import android.graphics.Color
import android.os.Bundle
import android.text.InputType
import android.view.Gravity
import android.view.ViewGroup
import android.widget.Button
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import dev.pebblersvp.android.cache.CoverCache
import dev.pebblersvp.android.cache.CachedLibraryManifest
import dev.pebblersvp.android.cache.LibraryManifestCache
import dev.pebblersvp.android.cache.ReaderPackageCache
import dev.pebblersvp.android.cache.ProgressCache
import dev.pebblersvp.android.pairing.PairingCredentials
import dev.pebblersvp.android.pairing.PairingStore
import dev.pebblersvp.android.protocol.MessageType
import dev.pebblersvp.android.protocol.RsvpPacket
import dev.pebblersvp.android.sync.LibraryManifest
import dev.pebblersvp.android.sync.LibrarySynchronizer
import dev.pebblersvp.android.sync.LibraryBook
import dev.pebblersvp.android.transport.PebblePacketBridge
import dev.pebblersvp.android.transport.ReaderWindowEncoder
import dev.pebblersvp.android.transport.ReaderWindowStartMode
import io.rebble.pebblekit2.client.DefaultPebbleAndroidAppPicker
import io.rebble.pebblekit2.client.PebbleAndroidAppPicker
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlinx.coroutines.delay
import java.nio.charset.StandardCharsets
import java.util.UUID
import kotlin.random.Random

class MainActivity : Activity() {
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main.immediate)
    private lateinit var bridge: PebblePacketBridge
    private lateinit var pairingStore: PairingStore
    private lateinit var packageCache: ReaderPackageCache
    private lateinit var coverCache: CoverCache
    private lateinit var manifestCache: LibraryManifestCache
    private lateinit var progressCache: ProgressCache
    private lateinit var synchronizer: LibrarySynchronizer
    private lateinit var pebbleAppPicker: PebbleAndroidAppPicker

    private lateinit var pairingInput: EditText
    private lateinit var connectionStatus: TextView
    private lateinit var actionStatus: TextView
    private lateinit var diagnostics: TextView
    private lateinit var savePairingButton: Button
    private lateinit var removePairingButton: Button
    private lateinit var syncButton: Button
    private lateinit var clearCacheButton: Button
    private lateinit var bookList: LinearLayout
    private val bookButtons = mutableListOf<Button>()
    private var syncInProgress = false
    private var transferInProgress = false
    private var selectedPebbleApp: String? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        bridge = PebblePacketBridge(this)
        pairingStore = PairingStore(this)
        packageCache = ReaderPackageCache(this)
        coverCache = CoverCache(this)
        manifestCache = LibraryManifestCache(this)
        progressCache = ProgressCache(this)
        synchronizer = LibrarySynchronizer(manifestCache, packageCache, coverCache, progressCache)
        pebbleAppPicker = DefaultPebbleAndroidAppPicker.getInstance(this).apply {
            enableAutoSelect = false
        }
        setContentView(buildContent())
        refreshStatus(getString(R.string.ready))
        refreshPebbleAppSelection()
        if (loadCredentials() != null) {
            syncLibrary()
        }
    }

    private fun buildContent(): ScrollView {
        val content = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(20), dp(24), dp(20), dp(32))
        }
        content.addView(TextView(this).apply {
            text = getString(R.string.companion_title)
            textSize = 26f
            setTextColor(Color.rgb(32, 32, 29))
            setTypeface(typeface, android.graphics.Typeface.BOLD)
            setPadding(0, 0, 0, dp(18))
        })

        content.addView(sectionTitle(R.string.connection_title))
        connectionStatus = bodyText()
        content.addView(connectionStatus)
        pairingInput = EditText(this).apply {
            hint = getString(R.string.pairing_hint)
            inputType = InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_FLAG_MULTI_LINE
            gravity = Gravity.TOP
            minLines = 4
            maxLines = 7
            setPadding(dp(12), dp(10), dp(12), dp(10))
        }
        content.addView(pairingInput, matchWidth())
        savePairingButton = primaryButton(R.string.save_pairing) { savePairing() }
        removePairingButton = secondaryButton(R.string.remove_pairing) { removePairing() }
        content.addView(savePairingButton, matchWidth())
        content.addView(removePairingButton, matchWidth())

        content.addView(sectionTitle(R.string.sync_title))
        syncButton = primaryButton(R.string.sync_now) { syncLibrary() }
        clearCacheButton = secondaryButton(R.string.clear_cache) { clearCache() }
        content.addView(syncButton, matchWidth())
        content.addView(clearCacheButton, matchWidth())

        content.addView(sectionTitle(R.string.library_title))
        bookList = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
        }
        content.addView(bookList, matchWidth())

        content.addView(sectionTitle(R.string.watch_title))
        content.addView(secondaryButton(R.string.choose_pebble_app) { choosePebbleApp() }, matchWidth())
        content.addView(secondaryButton(R.string.open_watch) { openWatchApp() }, matchWidth())
        content.addView(secondaryButton(R.string.send_smoke_test) { sendSmokeTestPacket() }, matchWidth())

        content.addView(sectionTitle(R.string.diagnostics_title))
        actionStatus = bodyText()
        diagnostics = TextView(this).apply {
            textSize = 13f
            setTextColor(Color.rgb(70, 72, 67))
            setTextIsSelectable(true)
            setPadding(0, dp(8), 0, 0)
        }
        content.addView(actionStatus)
        content.addView(diagnostics)

        return ScrollView(this).apply {
            isFillViewport = true
            addView(content, matchWidth())
        }
    }

    private fun savePairing() {
        try {
            val credentials = PairingCredentials.parse(pairingInput.text.toString())
            pairingStore.save(credentials)
            pairingInput.text.clear()
            refreshStatus(getString(R.string.pairing_saved))
            syncLibrary()
        } catch (error: Exception) {
            refreshStatus(error.message ?: error.javaClass.simpleName)
        }
    }

    private fun removePairing() {
        pairingStore.clear()
        refreshStatus(getString(R.string.pairing_removed))
    }

    private fun clearCache() {
        if (syncInProgress || transferInProgress) return
        synchronizer.clear()
        refreshStatus(getString(R.string.cache_cleared))
    }

    private fun syncLibrary() {
        if (syncInProgress || transferInProgress) return
        val credentials = loadCredentials()
        if (credentials == null) {
            refreshStatus(getString(R.string.pair_before_sync))
            return
        }
        syncInProgress = true
        refreshStatus(getString(R.string.syncing))
        scope.launch {
            try {
                val result = withContext(Dispatchers.IO) { synchronizer.sync(credentials) }
                val message = if (result.changed) {
                    getString(
                        R.string.sync_complete,
                        result.bookCount,
                        result.downloadedPackages,
                        result.downloadedCovers,
                    )
                } else {
                    getString(R.string.sync_unchanged)
                }
                refreshStatus(message)
            } catch (error: Exception) {
                refreshStatus(error.message ?: error.javaClass.simpleName)
            } finally {
                syncInProgress = false
                refreshControls()
            }
        }
    }

    private fun sendSmokeTestPacket() {
        scope.launch {
            try {
                val packet = RsvpPacket(
                    messageType = MessageType.Request,
                    requestId = Random.nextInt().toLong() and 0xffffffffL,
                    payload = "ping".toByteArray(StandardCharsets.UTF_8),
                ).encode()
                val result = bridge.sendPacket(packet)
                refreshStatus("Smoke packet sent: $result")
            } catch (error: Exception) {
                refreshStatus(error.message ?: error.javaClass.simpleName)
            }
        }
    }

    private fun sendBookToWatch(book: LibraryBook) {
        if (syncInProgress || transferInProgress) return
        val packageBytes = packageCache.read(book.bookId, book.revision)
        if (packageBytes == null) {
            refreshStatus(getString(R.string.package_missing))
            return
        }
        transferInProgress = true
        refreshControls()
        scope.launch {
            try {
                val window = withContext(Dispatchers.IO) {
                    val progress = progressCache.read(book.bookId)
                    ReaderWindowEncoder.encode(
                        book.bookId,
                        book.revision,
                        packageBytes,
                        startWordIndex = progress?.let {
                            if (it.totalTokenCount > 0) it.tokenIndex.coerceAtMost(it.totalTokenCount - 1) else 0
                        } ?: book.progressTokenIndex,
                        titleOverride = book.title,
                        startMode = ReaderWindowStartMode.SentenceStart,
                    )
                }
                val requestId = Random.nextInt().toLong() and 0xffffffffL
                val chunkId = Random.nextInt().toLong() and 0xffffffffL
                val packets = RsvpPacket.splitPayload(
                    requestId = requestId,
                    bookId = book.bookId,
                    bookRevision = book.revision,
                    chunkId = chunkId,
                    payload = window.payload,
                    maxPayloadLength = AppConstants.DefaultPacketPayloadBytes,
                )
                openWatchForTransfer()
                packets.forEach { packet ->
                    bridge.sendPacket(packet)
                }
                refreshStatus(getString(R.string.book_sent, book.title, window.wordCount, packets.size))
            } catch (error: Exception) {
                refreshStatus(error.message ?: error.javaClass.simpleName)
            } finally {
                transferInProgress = false
                refreshControls()
            }
        }
    }

    private suspend fun openWatchForTransfer() {
        val sender = io.rebble.pebblekit2.client.DefaultPebbleSender(this@MainActivity)
        try {
            sender.startAppOnTheWatch(AppConstants.WatchAppUuid)
        } finally {
            sender.close()
        }
        // Let the watch receive the compact library list before the selected book window.
        delay(1000)
    }

    private fun openWatchApp() {
        scope.launch {
            try {
                val sender = io.rebble.pebblekit2.client.DefaultPebbleSender(this@MainActivity)
                val result = try {
                    sender.startAppOnTheWatch(AppConstants.WatchAppUuid)
                } finally {
                    sender.close()
                }
                refreshStatus("Open command sent: $result")
            } catch (error: Exception) {
                refreshStatus(error.message ?: error.javaClass.simpleName)
            }
        }
    }

    private fun refreshPebbleAppSelection() {
        scope.launch {
            selectedPebbleApp = pebbleAppPicker.getCurrentlySelectedApp()
            val eligible = pebbleAppPicker.getAllEligibleApps()
            if (selectedPebbleApp == null && eligible.size == 1) {
                pebbleAppPicker.selectApp(eligible.single())
                selectedPebbleApp = eligible.single()
            }
            refreshStatus(getString(R.string.ready))
        }
    }

    private fun choosePebbleApp() {
        val packages = pebbleAppPicker.getAllEligibleApps()
        if (packages.isEmpty()) {
            refreshStatus(getString(R.string.no_pebble_app))
            return
        }
        val labels = packages.map { packageName ->
            runCatching {
                val info = packageManager.getApplicationInfo(packageName, 0)
                "${packageManager.getApplicationLabel(info)} ($packageName)"
            }.getOrDefault(packageName)
        }.toTypedArray()
        AlertDialog.Builder(this)
            .setTitle(R.string.choose_pebble_app)
            .setItems(labels) { _, which ->
                scope.launch {
                    pebbleAppPicker.selectApp(packages[which])
                    selectedPebbleApp = packages[which]
                    refreshStatus(getString(R.string.pebble_app_selected, labels[which]))
                }
            }
            .setNegativeButton(android.R.string.cancel, null)
            .show()
    }

    private fun loadCredentials(): PairingCredentials? =
        runCatching { pairingStore.load() }.getOrNull()

    private fun refreshStatus(lastAction: String) {
        val credentials = loadCredentials()
        connectionStatus.text = credentials?.let {
            getString(R.string.paired_with, it.serverUrl)
        } ?: getString(R.string.not_paired)
        actionStatus.text = lastAction
        val cachedManifest = runCatching { manifestCache.read() }.getOrNull()
        val bookCount = cachedManifest?.let {
            runCatching { LibraryManifest.parse(it.json).books.size }.getOrNull()
        } ?: 0
        diagnostics.text = buildString {
            appendLine("Books: $bookCount")
            appendLine("Reader packages: ${packageCache.entryCount()} (${formatBytes(packageCache.approximateBytes())})")
            appendLine("Covers: ${coverCache.entryCount()} (${formatBytes(coverCache.approximateBytes())})")
            appendLine("Last server sync: ${cachedManifest?.syncedAt ?: "-"}")
            appendLine("Protocol version: ${RsvpPacket.ProtocolVersion}")
            appendLine("Pebble app: ${selectedPebbleApp ?: "not selected"}")
            append("Watch app UUID: ${AppConstants.WatchAppUuid}")
        }
        refreshBookButtons(cachedManifest)
        refreshControls()
    }

    private fun refreshBookButtons(cachedManifest: CachedLibraryManifest?) {
        bookList.removeAllViews()
        bookButtons.clear()
        val books = cachedManifest?.let {
            runCatching {
                LibraryManifest.parse(it.json).let { manifest -> manifest.orderedBooks() }
            }.getOrNull()
        }.orEmpty()
        if (books.isEmpty()) {
            bookList.addView(bodyText().apply { text = getString(R.string.no_cached_books) })
            return
        }
        books.forEach { book ->
            val button = secondaryButton(book.title) {
                sendBookToWatch(book)
            }
            bookButtons += button
            bookList.addView(button, matchWidth())
        }
    }

    private fun refreshControls() {
        val paired = loadCredentials() != null
        val busy = syncInProgress || transferInProgress
        pairingInput.isEnabled = !busy
        savePairingButton.isEnabled = !busy
        removePairingButton.isEnabled = paired && !busy
        syncButton.isEnabled = paired && !busy
        clearCacheButton.isEnabled = !busy
        bookButtons.forEach { it.isEnabled = !busy }
    }

    private fun sectionTitle(stringId: Int): TextView = TextView(this).apply {
        text = getString(stringId)
        textSize = 17f
        setTextColor(Color.rgb(32, 32, 29))
        setTypeface(typeface, android.graphics.Typeface.BOLD)
        setPadding(0, dp(22), 0, dp(8))
    }

    private fun bodyText(): TextView = TextView(this).apply {
        textSize = 15f
        setTextColor(Color.rgb(90, 92, 85))
        setPadding(0, 0, 0, dp(10))
    }

    private fun primaryButton(stringId: Int, action: () -> Unit): Button = Button(this).apply {
        text = getString(stringId)
        backgroundTintList = ColorStateList.valueOf(Color.rgb(20, 108, 92))
        setTextColor(Color.WHITE)
        setOnClickListener { action() }
    }

    private fun secondaryButton(stringId: Int, action: () -> Unit): Button = Button(this).apply {
        text = getString(stringId)
        setOnClickListener { action() }
    }

    private fun secondaryButton(label: String, action: () -> Unit): Button = Button(this).apply {
        text = label
        setOnClickListener { action() }
    }

    private fun matchWidth(): LinearLayout.LayoutParams =
        LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()

    private fun formatBytes(bytes: Long): String {
        if (bytes < 1024) return "$bytes B"
        val units = arrayOf("KB", "MB", "GB")
        var value = bytes.toDouble()
        var unit = -1
        while (value >= 1024 && unit + 1 < units.size) {
            value /= 1024
            unit += 1
        }
        return "%.1f %s".format(value, units[unit])
    }

    override fun onDestroy() {
        bridge.close()
        scope.cancel()
        super.onDestroy()
    }
}
