package dev.pebblersvp.android.transport

import dev.pebblersvp.android.cache.ReaderPackageIndex
import org.json.JSONObject
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.charset.StandardCharsets
import java.util.UUID

data class ReaderWindow(
    val bookId: UUID,
    val revision: Long,
    val payload: ByteArray,
    val startWordIndex: Int,
    val wordCount: Int,
    val totalWordCount: Int,
)

enum class ReaderWindowStartMode {
    Exact,
    SentenceStart,
    PreviousSentence,
    NextSentence,
}

object ReaderWindowEncoder {
    const val Version = 2
    const val MaxWords = 80
    const val MaxWordBytes = 64
    const val MaxTitleBytes = 32
    private const val HeaderBytes = 49
    private const val RecordHeaderBytes = 4

    fun encode(
        bookId: UUID,
        revision: Long,
        packageBytes: ByteArray,
        startWordIndex: Int = 0,
        titleOverride: String? = null,
        startMode: ReaderWindowStartMode = ReaderWindowStartMode.Exact,
    ): ReaderWindow {
        require(revision in 0..0xffffffffL) { "book revision must fit uint32" }
        val root = JSONObject(String(packageBytes, StandardCharsets.UTF_8))
        val metadata = root.optJSONObject("metadata")
        val title = titleOverride?.takeIf(String::isNotBlank)
            ?: metadata?.optString("title", "Untitled")
            ?: "Untitled"
        val tokens = root.getJSONArray("tokens")
        require(startWordIndex in 0 until tokens.length()) { "startWordIndex must point to a token" }
        val resolvedStartIndex = resolveStartIndex(tokens, startWordIndex, startMode)
        val count = minOf(tokens.length() - resolvedStartIndex, MaxWords)
        val output = ByteBuffer.allocate(HeaderBytes + count * (RecordHeaderBytes + MaxWordBytes))
            .order(ByteOrder.BIG_ENDIAN)

        output.put("RWND".toByteArray(StandardCharsets.US_ASCII))
        output.put(Version.toByte())
        output.put(0)
        output.putShort(count.toShort())
        output.putInt(resolvedStartIndex)
        output.putInt(tokens.length())
        putUtf8(output, title, MaxTitleBytes)

        for (index in 0 until count) {
            val tokenIndex = resolvedStartIndex + index
            val token = tokens.getJSONObject(tokenIndex)
            val display = token.optString("display", "")
            val duration = token.optLong("duration_ms_at_300wpm", 200L).coerceIn(0L, 0xffffL)
            val previousChapterId = if (tokenIndex == 0) {
                -1
            } else {
                tokens.optJSONObject(tokenIndex - 1)?.optInt("chapter_id", 0) ?: 0
            }
            var flags = if (token.optBoolean("paragraph_end", false)) 1 else 0
            if (token.optInt("chapter_id", 0) != previousChapterId) {
                flags = flags or 2
            }
            val previousSentenceId = if (tokenIndex == 0) {
                Int.MIN_VALUE
            } else {
                tokens.optJSONObject(tokenIndex - 1)?.optInt("sentence_id", Int.MIN_VALUE) ?: Int.MIN_VALUE
            }
            val sentenceId = token.optInt("sentence_id", Int.MIN_VALUE)
            if (tokenIndex == 0 || (sentenceId != Int.MIN_VALUE && sentenceId != previousSentenceId)) {
                flags = flags or 4
            }
            val focusCharacterIndex = token.optInt("focus_char_index", -1)
            if (focusCharacterIndex in 0..15) {
                flags = flags or 0x80 or (focusCharacterIndex shl 3)
            }
            val encoded = truncateUtf8(display, MaxWordBytes)
            output.put(encoded.size.toByte())
            output.put(flags.toByte())
            output.putShort(duration.toShort())
            output.put(encoded)
        }

        return ReaderWindow(
            bookId = bookId,
            revision = revision,
            payload = output.array().copyOf(output.position()),
            startWordIndex = resolvedStartIndex,
            wordCount = count,
            totalWordCount = tokens.length(),
        )
    }

    fun encode(
        bookId: UUID,
        revision: Long,
        index: ReaderPackageIndex,
        startWordIndex: Int = 0,
        titleOverride: String? = null,
        startMode: ReaderWindowStartMode = ReaderWindowStartMode.Exact,
    ): ReaderWindow {
        require(revision in 0..0xffffffffL) { "book revision must fit uint32" }
        require(startWordIndex in 0 until index.tokenCount) { "startWordIndex must point to a token" }
        val resolvedStartIndex = when (startMode) {
            ReaderWindowStartMode.Exact -> startWordIndex
            ReaderWindowStartMode.SentenceStart -> index.sentenceStartAtOrBefore(startWordIndex)
            ReaderWindowStartMode.PreviousSentence -> {
                val currentStart = index.sentenceStartAtOrBefore(startWordIndex)
                if (currentStart > 0) index.sentenceStartAtOrBefore(currentStart - 1) else 0
            }
            ReaderWindowStartMode.NextSentence -> index.nextSentenceStart(startWordIndex)
        }
        val count = minOf(index.tokenCount - resolvedStartIndex, MaxWords)
        val output = ByteBuffer.allocate(HeaderBytes + count * (RecordHeaderBytes + MaxWordBytes))
            .order(ByteOrder.BIG_ENDIAN)
        output.put("RWND".toByteArray(StandardCharsets.US_ASCII))
        output.put(Version.toByte())
        output.put(0)
        output.putShort(count.toShort())
        output.putInt(resolvedStartIndex)
        output.putInt(index.tokenCount)
        putUtf8(output, titleOverride?.takeIf(String::isNotBlank) ?: index.title, MaxTitleBytes)
        for (tokenIndex in resolvedStartIndex until resolvedStartIndex + count) {
            val token = index.token(tokenIndex)
            val encoded = truncateUtf8(token.display, MaxWordBytes)
            output.put(encoded.size.toByte())
            output.put(token.flags.toByte())
            output.putShort(token.durationMsAt300Wpm.toShort())
            output.put(encoded)
        }
        return ReaderWindow(bookId, revision, output.array().copyOf(output.position()), resolvedStartIndex,
            count, index.tokenCount)
    }

    private fun resolveStartIndex(
        tokens: org.json.JSONArray,
        requestedIndex: Int,
        mode: ReaderWindowStartMode,
    ): Int {
        if (mode == ReaderWindowStartMode.Exact) return requestedIndex
        val currentStart = sentenceStartAtOrBefore(tokens, requestedIndex)
        return when (mode) {
            ReaderWindowStartMode.Exact -> requestedIndex
            ReaderWindowStartMode.SentenceStart -> currentStart
            ReaderWindowStartMode.PreviousSentence -> if (currentStart > 0) {
                sentenceStartAtOrBefore(tokens, currentStart - 1)
            } else 0
            ReaderWindowStartMode.NextSentence -> nextSentenceStart(tokens, requestedIndex)
        }
    }

    private fun sentenceStartAtOrBefore(tokens: org.json.JSONArray, index: Int): Int {
        val sentenceId = tokens.getJSONObject(index).optInt("sentence_id", Int.MIN_VALUE)
        if (sentenceId == Int.MIN_VALUE) return index
        var start = index
        while (start > 0 && tokens.getJSONObject(start - 1).optInt("sentence_id", Int.MIN_VALUE) == sentenceId) {
            start -= 1
        }
        return start
    }

    private fun nextSentenceStart(tokens: org.json.JSONArray, index: Int): Int {
        val sentenceId = tokens.getJSONObject(index).optInt("sentence_id", Int.MIN_VALUE)
        if (sentenceId == Int.MIN_VALUE) return minOf(index + 1, tokens.length() - 1)
        var next = index + 1
        while (next < tokens.length() &&
            tokens.getJSONObject(next).optInt("sentence_id", Int.MIN_VALUE) == sentenceId
        ) {
            next += 1
        }
        return minOf(next, tokens.length() - 1)
    }

    private fun putUtf8(output: ByteBuffer, value: String, maxBytes: Int) {
        val encoded = truncateUtf8(value, maxBytes)
        output.put(encoded.size.toByte())
        output.put(encoded)
        repeat(maxBytes - encoded.size) { output.put(0) }
    }

    private fun truncateUtf8(value: String, maxBytes: Int): ByteArray {
        val full = value.toByteArray(StandardCharsets.UTF_8)
        if (full.size <= maxBytes) return full
        val output = StringBuilder()
        var index = 0
        var byteCount = 0
        while (index < value.length) {
            val codePoint = value.codePointAt(index)
            val codePointText = String(Character.toChars(codePoint))
            val codePointBytes = codePointText.toByteArray(StandardCharsets.UTF_8)
            if (byteCount + codePointBytes.size > maxBytes) break
            output.appendCodePoint(codePoint)
            byteCount += codePointBytes.size
            index += Character.charCount(codePoint)
        }
        return output.toString().toByteArray(StandardCharsets.UTF_8)
    }
}
