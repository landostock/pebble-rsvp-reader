package dev.pebblersvp.android.transport

import org.junit.Assert.assertEquals
import org.junit.Test
import java.nio.ByteBuffer

class ChapterPayloadEncoderTest {
    @Test
    fun encodesChapterTitlesAndAbsoluteTokenStarts() {
        val payload = ChapterPayloadEncoder.encode(
            """
            {"chapters":[
              {"title":"Anfang","token_start":0},
              {"title":"Zweites Kapitel","token_start":321}
            ]}
            """.trimIndent().toByteArray(),
        )

        assertEquals("CHAP", String(payload, 0, 4, Charsets.US_ASCII))
        assertEquals(2, payload[5].toInt())
        assertEquals(0, ByteBuffer.wrap(payload, 8, 4).int)
        assertEquals("Anfang", String(payload, 13, 6, Charsets.UTF_8))
        assertEquals(321, ByteBuffer.wrap(payload, 37, 4).int)
        assertEquals("Zweites Kapitel", String(payload, 42, 15, Charsets.UTF_8))
    }
}
