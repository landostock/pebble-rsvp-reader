package dev.pebblersvp.android.transport

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import java.nio.charset.StandardCharsets
import java.util.UUID

class ReaderWindowEncoderTest {
    @Test
    fun encodesMetadataAndTheFirstWindowOfTokens() {
        val packageJson = """
            {
              "metadata": {"title": "Ein langer Buchtitel"},
              "tokens": [
                {"display": "Hallo", "duration_ms_at_300wpm": 240, "paragraph_end": false},
                {"display": "Welt.", "duration_ms_at_300wpm": 520, "paragraph_end": true}
              ]
            }
        """.trimIndent().toByteArray(StandardCharsets.UTF_8)

        val window = ReaderWindowEncoder.encode(UUID.randomUUID(), 3, packageJson)

        assertEquals(2, window.wordCount)
        assertEquals(2, window.totalWordCount)
        assertEquals(49 + (4 + 5) + (4 + 5), window.payload.size)
        assertEquals("RWND", String(window.payload, 0, 4, StandardCharsets.US_ASCII))
        assertEquals("Ein langer Buchtitel".toByteArray(StandardCharsets.UTF_8).size, window.payload[16].toInt())
        assertTrue(window.payload.copyOfRange(17, 49).decodeToString().contains("Ein langer"))
    }

    @Test
    fun truncatesUtf8WithoutSplittingACharacter() {
        val packageJson = """
            {"metadata":{"title":"Ä"},"tokens":[{"display":"äöüäöüäöüäöüäöüäöüäöüäöüäöüäöü", "paragraph_end":true}]}
        """.trimIndent().toByteArray(StandardCharsets.UTF_8)

        val window = ReaderWindowEncoder.encode(UUID.randomUUID(), 1, packageJson)
        val wordLength = window.payload[49].toInt()
        val word = window.payload.copyOfRange(53, 53 + wordLength).toString(StandardCharsets.UTF_8)

        assertTrue(word.isNotEmpty())
        assertTrue(wordLength <= ReaderWindowEncoder.MaxWordBytes)
        assertEquals(word.toByteArray(StandardCharsets.UTF_8).size, wordLength)
    }

    @Test
    fun encodesAWindowStartingAfterTheFirstToken() {
        val packageJson = """
            {"metadata":{"title":"Test"},"tokens":[
              {"display":"Eins"}, {"display":"Zwei"}, {"display":"Drei"}
            ]}
        """.trimIndent().toByteArray(StandardCharsets.UTF_8)

        val window = ReaderWindowEncoder.encode(UUID.randomUUID(), 1, packageJson, startWordIndex = 2)

        assertEquals(2, window.startWordIndex)
        assertEquals(1, window.wordCount)
        assertEquals(2, java.nio.ByteBuffer.wrap(window.payload, 8, 4).int)
    }

    @Test
    fun prefersEditedManifestTitleOverEmbeddedPackageTitle() {
        val packageJson = """
            {"metadata":{"title":"Alter EPUB-Titel"},"tokens":[{"display":"Eins"}]}
        """.trimIndent().toByteArray(StandardCharsets.UTF_8)

        val window = ReaderWindowEncoder.encode(
            UUID.randomUUID(),
            1,
            packageJson,
            titleOverride = "Bearbeiteter Titel",
        )

        assertTrue(window.payload.copyOfRange(17, 49).decodeToString().contains("Bearbeiteter Titel"))
    }

    @Test
    fun marksChapterStartsWithoutSplittingPunctuationTokens() {
        val packageJson = """
            {"metadata":{"title":"Test"},"tokens":[
              {"display":"Kapitel", "chapter_id":0, "paragraph_end":true},
              {"display":"Zweiter", "chapter_id":1, "paragraph_end":false},
              {"display":"Abschnitt.", "chapter_id":1, "paragraph_end":true}
            ]}
        """.trimIndent().toByteArray(StandardCharsets.UTF_8)

        val window = ReaderWindowEncoder.encode(UUID.randomUUID(), 1, packageJson)

        val secondRecord = 49 + 4 + "Kapitel".toByteArray().size
        val thirdRecord = secondRecord + 4 + "Zweiter".toByteArray().size
        assertEquals(2, window.payload[secondRecord + 1].toInt() and 2)
        assertEquals("Abschnitt.", window.payload.copyOfRange(thirdRecord + 4, thirdRecord + 4 + 10)
            .decodeToString())
    }

    @Test
    fun resumesAtTheBeginningOfTheCurrentSentence() {
        val packageJson = packageWithSentences()

        val window = ReaderWindowEncoder.encode(
            UUID.randomUUID(),
            1,
            packageJson,
            startWordIndex = 3,
            startMode = ReaderWindowStartMode.SentenceStart,
        )

        assertEquals(2, window.startWordIndex)
        assertEquals(4, window.payload[49 + 1].toInt() and 4)
    }

    @Test
    fun rewindingAtASentenceStartSelectsThePreviousSentence() {
        val window = ReaderWindowEncoder.encode(
            UUID.randomUUID(),
            1,
            packageWithSentences(),
            startWordIndex = 2,
            startMode = ReaderWindowStartMode.PreviousSentence,
        )

        assertEquals(0, window.startWordIndex)
    }

    @Test
    fun rewindingInsideASentenceStillSelectsThePreviousSentence() {
        val window = ReaderWindowEncoder.encode(
            UUID.randomUUID(),
            1,
            packageWithSentences(),
            startWordIndex = 3,
            startMode = ReaderWindowStartMode.PreviousSentence,
        )

        assertEquals(0, window.startWordIndex)
    }

    @Test
    fun advancesToTheNextSentence() {
        val window = ReaderWindowEncoder.encode(
            UUID.randomUUID(),
            1,
            packageWithSentences(),
            startWordIndex = 1,
            startMode = ReaderWindowStartMode.NextSentence,
        )

        assertEquals(2, window.startWordIndex)
    }

    @Test
    fun carriesThePreparedOrpDisplayCharacterIndex() {
        val packageJson = """
            {"metadata":{"title":"Test"},"tokens":[
              {"display":"„Lesen", "focus_char_index":2}
            ]}
        """.trimIndent().toByteArray(StandardCharsets.UTF_8)

        val window = ReaderWindowEncoder.encode(UUID.randomUUID(), 1, packageJson)
        val flags = window.payload[49 + 1].toInt() and 0xff

        assertEquals(0x80, flags and 0x80)
        assertEquals(2, (flags shr 3) and 0x0f)
    }

    @Test
    fun packsNinetySixShortWordsWithoutFixedWidthPadding() {
        val tokens = (0 until 97).joinToString(",") { "{\"display\":\"x\"}" }
        val packageJson = "{\"metadata\":{\"title\":\"Test\"},\"tokens\":[$tokens]}"

        val window = ReaderWindowEncoder.encode(
            UUID.randomUUID(),
            1,
            packageJson.toByteArray(StandardCharsets.UTF_8),
        )

        assertEquals(80, window.wordCount)
        assertEquals(49 + 80 * 5, window.payload.size)
        assertEquals(2, window.payload[4].toInt())
    }

    private fun packageWithSentences() = """
        {"metadata":{"title":"Test"},"tokens":[
          {"display":"Eins", "sentence_id":0},
          {"display":"zwei.", "sentence_id":0},
          {"display":"Drei", "sentence_id":1},
          {"display":"vier.", "sentence_id":1},
          {"display":"Fünf.", "sentence_id":2}
        ]}
    """.trimIndent().toByteArray(StandardCharsets.UTF_8)
}
