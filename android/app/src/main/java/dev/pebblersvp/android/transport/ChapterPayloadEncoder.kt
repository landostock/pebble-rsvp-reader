package dev.pebblersvp.android.transport

import org.json.JSONObject
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.charset.StandardCharsets

object ChapterPayloadEncoder {
    const val Version = 1
    const val MaxChapters = 64
    const val MaxTitleBytes = 24
    private const val HeaderBytes = 8
    private const val EntryBytes = 4 + 1 + MaxTitleBytes

    fun encode(packageBytes: ByteArray): ByteArray {
        val root = JSONObject(String(packageBytes, StandardCharsets.UTF_8))
        val chapters = root.getJSONArray("chapters")
        val count = minOf(chapters.length(), MaxChapters)
        val output = ByteBuffer.allocate(HeaderBytes + count * EntryBytes).order(ByteOrder.BIG_ENDIAN)
        output.put("CHAP".toByteArray(StandardCharsets.US_ASCII))
        output.put(Version.toByte())
        output.put(count.toByte())
        output.putShort(chapters.length().coerceAtMost(0xffff).toShort())
        for (index in 0 until count) {
            val chapter = chapters.getJSONObject(index)
            output.putInt(chapter.optInt("token_start", 0).coerceAtLeast(0))
            putUtf8(output, chapter.optString("title", "Kapitel ${index + 1}"), MaxTitleBytes)
        }
        return output.array()
    }

    private fun putUtf8(output: ByteBuffer, value: String, maxBytes: Int) {
        val result = StringBuilder()
        var index = 0
        var bytes = 0
        while (index < value.length) {
            val codePoint = value.codePointAt(index)
            val encoded = String(Character.toChars(codePoint)).toByteArray(StandardCharsets.UTF_8)
            if (bytes + encoded.size > maxBytes) break
            result.appendCodePoint(codePoint)
            bytes += encoded.size
            index += Character.charCount(codePoint)
        }
        val encoded = result.toString().toByteArray(StandardCharsets.UTF_8)
        output.put(encoded.size.toByte())
        output.put(encoded)
        repeat(maxBytes - encoded.size) { output.put(0) }
    }
}
