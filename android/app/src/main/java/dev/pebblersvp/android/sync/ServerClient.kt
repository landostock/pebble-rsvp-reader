package dev.pebblersvp.android.sync

import dev.pebblersvp.android.pairing.PairingCredentials
import dev.pebblersvp.android.cache.CachedProgress
import org.json.JSONObject
import java.io.ByteArrayOutputStream
import java.io.IOException
import java.net.HttpURLConnection
import java.net.URL

sealed interface LibraryResponse {
    data object NotModified : LibraryResponse
    data class Changed(val json: String, val etag: String) : LibraryResponse
}

class ServerClient(private val credentials: PairingCredentials) {
    fun fetchLibrary(etag: String?): LibraryResponse {
        val connection = open("/api/library")
        if (!etag.isNullOrBlank()) {
            connection.setRequestProperty("If-None-Match", etag)
        }
        return connection.useResponse { response ->
            when (response.status) {
                HttpURLConnection.HTTP_NOT_MODIFIED -> LibraryResponse.NotModified
                HttpURLConnection.HTTP_OK -> LibraryResponse.Changed(
                    json = response.readBody(MaxManifestBytes).toString(Charsets.UTF_8),
                    etag = response.connection.getHeaderField("ETag")
                        ?: throw IOException("Library response is missing ETag"),
                )
                else -> throw response.toServerError()
            }
        }
    }

    fun downloadReaderPackage(bookId: String): ByteArray =
        download("/api/books/$bookId/reader-package", MaxReaderPackageBytes)

    fun downloadCover(bookId: String): ByteArray =
        download("/api/books/$bookId/cover", MaxCoverBytes)

    fun uploadProgress(progress: CachedProgress) {
        val body = JSONObject()
            .put("book_id", progress.bookId.toString())
            .put("book_revision", progress.bookRevision)
            .put("chapter_id", 0)
            .put("paragraph_id", 0)
            .put("sentence_id", 0)
            .put("token_index", progress.tokenIndex)
            .put("furthest_token_index", progress.furthestTokenIndex)
            .put("percentage", progress.percentage)
            .put("timestamp", progress.updatedAt)
            .put("session_id", "pebble-watch")
            .put("reset", false)
            .toString()
            .toByteArray(Charsets.UTF_8)
        val connection = open("/api/progress/${progress.bookId}", "PUT").apply {
            doOutput = true
            setRequestProperty("Content-Type", "application/json")
            setFixedLengthStreamingMode(body.size)
        }
        connection.outputStream.use { it.write(body) }
        connection.useResponse { response ->
            if (response.status != HttpURLConnection.HTTP_OK) throw response.toServerError()
        }
    }

    fun updateReaderSettings(language: String, librarySort: String) {
        require(language in setOf("en", "de"))
        require(librarySort in setOf("last_read", "title", "author", "recent", "progress"))
        val body = JSONObject()
            .put("language", language)
            .put("library_sort", librarySort)
            .toString()
            .toByteArray(Charsets.UTF_8)
        val connection = open("/api/settings", "PUT").apply {
            doOutput = true
            setRequestProperty("Content-Type", "application/json")
            setFixedLengthStreamingMode(body.size)
        }
        connection.outputStream.use { it.write(body) }
        connection.useResponse { response ->
            if (response.status != HttpURLConnection.HTTP_OK) throw response.toServerError()
        }
    }

    private fun download(path: String, limit: Int): ByteArray {
        val connection = open(path)
        return connection.useResponse { response ->
            if (response.status != HttpURLConnection.HTTP_OK) {
                throw response.toServerError()
            }
            response.readBody(limit)
        }
    }

    private fun open(path: String, method: String = "GET"): HttpURLConnection =
        (URL("${credentials.serverUrl}$path").openConnection() as HttpURLConnection).apply {
            requestMethod = method
            connectTimeout = ConnectTimeoutMs
            readTimeout = ReadTimeoutMs
            setRequestProperty("Accept", "application/json, application/octet-stream")
            setRequestProperty("Authorization", "Bearer ${credentials.deviceToken}")
            useCaches = false
        }

    private data class Response(
        val connection: HttpURLConnection,
        val status: Int,
    ) {
        fun readBody(limit: Int): ByteArray {
            val stream = if (status >= 400) connection.errorStream else connection.inputStream
            stream ?: return ByteArray(0)
            stream.use { input ->
                val output = ByteArrayOutputStream()
                val buffer = ByteArray(8 * 1024)
                var total = 0
                while (true) {
                    val count = input.read(buffer)
                    if (count == -1) break
                    total += count
                    if (total > limit) throw IOException("Server response exceeds size limit")
                    output.write(buffer, 0, count)
                }
                return output.toByteArray()
            }
        }

        fun toServerError(): IOException {
            val message = runCatching {
                readBody(MaxErrorBytes).toString(Charsets.UTF_8).takeIf(String::isNotBlank)
            }.getOrNull()
            return IOException("Server returned HTTP $status${message?.let { ": $it" }.orEmpty()}")
        }
    }

    private inline fun <T> HttpURLConnection.useResponse(block: (Response) -> T): T =
        try {
            block(Response(this, responseCode))
        } finally {
            disconnect()
        }

    private companion object {
        const val ConnectTimeoutMs = 10_000
        const val ReadTimeoutMs = 30_000
        const val MaxManifestBytes = 5 * 1024 * 1024
        const val MaxReaderPackageBytes = 64 * 1024 * 1024
        const val MaxCoverBytes = 5 * 1024 * 1024
        const val MaxErrorBytes = 16 * 1024
    }
}
