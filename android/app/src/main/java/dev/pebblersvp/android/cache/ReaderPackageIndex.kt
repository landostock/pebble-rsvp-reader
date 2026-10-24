package dev.pebblersvp.android.cache

import org.json.JSONObject
import java.io.ByteArrayOutputStream
import java.io.DataOutputStream
import java.io.File
import java.io.RandomAccessFile
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.channels.FileChannel
import java.nio.charset.StandardCharsets

data class IndexedReaderToken(
    val display: String,
    val durationMsAt300Wpm: Int,
    val flags: Int,
)

class ReaderPackageIndex private constructor(
    private val data: ByteBuffer,
    val title: String,
    val tokenCount: Int,
    val recommendedStartTokenIndex: Int,
    private val offsetsStart: Int,
    private val recordsStart: Int,
) {
    fun token(index: Int): IndexedReaderToken {
        require(index in 0 until tokenCount) { "token index out of range" }
        val recordOffset = data.getInt(offsetsStart + index * Int.SIZE_BYTES)
        val position = recordsStart + recordOffset
        val length = data.get(position).toInt() and 0xff
        val flags = data.get(position + 1).toInt() and 0xff
        val duration = data.getShort(position + 2).toInt() and 0xffff
        val encoded = ByteArray(length)
        data.duplicate().apply {
            position(position + RecordHeaderBytes)
            get(encoded)
        }
        return IndexedReaderToken(String(encoded, StandardCharsets.UTF_8), duration, flags)
    }

    fun sentenceStartAtOrBefore(index: Int): Int {
        var cursor = index.coerceIn(0, tokenCount - 1)
        while (cursor > 0 && token(cursor).flags and FlagSentenceStart == 0) cursor -= 1
        return cursor
    }

    fun nextSentenceStart(index: Int): Int {
        var cursor = (index + 1).coerceAtMost(tokenCount - 1)
        while (cursor < tokenCount - 1 && token(cursor).flags and FlagSentenceStart == 0) cursor += 1
        return cursor
    }

    companion object {
        const val Version = 2
        const val FlagParagraphEnd = 1
        const val FlagChapterStart = 2
        const val FlagSentenceStart = 4
        const val FlagFocusPresent = 0x80
        const val FocusShift = 3
        const val FocusMask = 0x0f
        private const val Magic = 0x52504958
        private const val HeaderBytes = 20
        private const val RecordHeaderBytes = 4

        fun build(packageBytes: ByteArray, destination: File) {
            val root = JSONObject(String(packageBytes, StandardCharsets.UTF_8))
            val title = root.optJSONObject("metadata")?.optString("title", "Untitled") ?: "Untitled"
            val recommendedStart = root.optJSONObject("metadata")
                ?.optInt("recommended_start_token_index", 0)
                ?.coerceIn(0, root.getJSONArray("tokens").length() - 1) ?: 0
            val titleBytes = title.toByteArray(StandardCharsets.UTF_8)
            require(titleBytes.size <= 0xffff) { "reader package title is too long" }
            val tokens = root.getJSONArray("tokens")
            require(tokens.length() > 0) { "reader package has no tokens" }

            val offsets = IntArray(tokens.length() + 1)
            val recordBytes = ByteArrayOutputStream(tokens.length() * 12)
            DataOutputStream(recordBytes).use { records ->
                for (index in 0 until tokens.length()) {
                    offsets[index] = recordBytes.size()
                    val token = tokens.getJSONObject(index)
                    var flags = if (token.optBoolean("paragraph_end", false)) FlagParagraphEnd else 0
                    val previous = if (index > 0) tokens.getJSONObject(index - 1) else null
                    if (index == 0 || token.optInt("chapter_id", 0) != previous?.optInt("chapter_id", 0)) {
                        flags = flags or FlagChapterStart
                    }
                    val sentenceId = token.optInt("sentence_id", Int.MIN_VALUE)
                    val previousSentenceId = previous?.optInt("sentence_id", Int.MIN_VALUE) ?: Int.MIN_VALUE
                    if (index == 0 || (sentenceId != Int.MIN_VALUE && sentenceId != previousSentenceId)) {
                        flags = flags or FlagSentenceStart
                    }
                    val focus = token.optInt("focus_char_index", -1)
                    if (focus in 0..FocusMask) {
                        flags = flags or FlagFocusPresent or (focus shl FocusShift)
                    }
                    val encoded = truncateUtf8(token.optString("display", ""), 64)
                    val duration = token.optLong("duration_ms_at_300wpm", 200L).coerceIn(0L, 0xffffL)
                    records.writeByte(encoded.size)
                    records.writeByte(flags)
                    records.writeShort(duration.toInt())
                    records.write(encoded)
                }
                offsets[tokens.length()] = recordBytes.size()
            }

            destination.parentFile?.mkdirs()
            val temporary = File(destination.parentFile, "${destination.name}.tmp")
            DataOutputStream(temporary.outputStream().buffered()).use { output ->
                output.writeInt(Magic)
                output.writeInt(Version)
                output.writeInt(tokens.length())
                output.writeInt(recommendedStart)
                output.writeShort(titleBytes.size)
                output.writeShort(0)
                output.write(titleBytes)
                offsets.forEach(output::writeInt)
                recordBytes.writeTo(output)
            }
            if (!temporary.renameTo(destination)) {
                temporary.delete()
                error("failed to atomically publish reader package index")
            }
        }

        fun open(file: File): ReaderPackageIndex {
            val mapped = RandomAccessFile(file, "r").use { input ->
                input.channel.map(FileChannel.MapMode.READ_ONLY, 0, input.length())
            }.order(ByteOrder.BIG_ENDIAN)
            require(mapped.remaining() >= HeaderBytes && mapped.int == Magic) { "invalid reader package index" }
            require(mapped.int == Version) { "unsupported reader package index version" }
            val tokenCount = mapped.int
            require(tokenCount > 0) { "reader package index has no tokens" }
            val recommendedStart = mapped.int
            require(recommendedStart in 0 until tokenCount) { "invalid recommended reader start" }
            val titleLength = mapped.short.toInt() and 0xffff
            mapped.short
            require(mapped.remaining() >= titleLength + (tokenCount + 1) * Int.SIZE_BYTES) {
                "truncated reader package index"
            }
            val titleBytes = ByteArray(titleLength)
            mapped.get(titleBytes)
            val offsetsStart = mapped.position()
            val recordsStart = offsetsStart + (tokenCount + 1) * Int.SIZE_BYTES
            val recordsLength = mapped.getInt(offsetsStart + tokenCount * Int.SIZE_BYTES)
            require(recordsLength >= 0 && recordsStart + recordsLength <= mapped.limit()) {
                "invalid reader package index offsets"
            }
            return ReaderPackageIndex(
                data = mapped.asReadOnlyBuffer().order(ByteOrder.BIG_ENDIAN),
                title = String(titleBytes, StandardCharsets.UTF_8),
                tokenCount = tokenCount,
                recommendedStartTokenIndex = recommendedStart,
                offsetsStart = offsetsStart,
                recordsStart = recordsStart,
            )
        }

        private fun truncateUtf8(value: String, maxBytes: Int): ByteArray {
            val full = value.toByteArray(StandardCharsets.UTF_8)
            if (full.size <= maxBytes) return full
            val output = StringBuilder()
            var index = 0
            var byteCount = 0
            while (index < value.length) {
                val codePoint = value.codePointAt(index)
                val encoded = String(Character.toChars(codePoint)).toByteArray(StandardCharsets.UTF_8)
                if (byteCount + encoded.size > maxBytes) break
                output.appendCodePoint(codePoint)
                byteCount += encoded.size
                index += Character.charCount(codePoint)
            }
            return output.toString().toByteArray(StandardCharsets.UTF_8)
        }
    }
}
