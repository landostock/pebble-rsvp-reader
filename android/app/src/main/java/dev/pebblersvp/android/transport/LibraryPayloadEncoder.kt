package dev.pebblersvp.android.transport

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Color
import dev.pebblersvp.android.cache.CoverCache
import dev.pebblersvp.android.sync.LibraryBook
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.charset.StandardCharsets
import java.text.SimpleDateFormat
import java.util.Locale
import java.util.TimeZone

object LibraryPayloadEncoder {
    const val Version = 6
    const val MaxBooks = 12
    const val MaxTitleBytes = 48
    const val MaxAuthorBytes = 24
    const val CoverWidth = 36
    const val CoverHeight = 52
    private const val HeaderBytes = 12
    private const val CoverRowBytes = CoverWidth / 2
    private const val CoverBytes = CoverRowBytes * CoverHeight
    private const val CoverPaletteBytes = 16
    private const val EntryBytes = 16 + 4 + 1 + MaxTitleBytes + 1 + MaxAuthorBytes +
        4 + 4 + 4 + 1 + 4 + 4 + 1 + CoverPaletteBytes + CoverBytes

    fun encode(
        books: List<LibraryBook>,
        coverCache: CoverCache,
        language: String = "en",
        librarySort: String = "last_read",
    ): ByteArray {
        val selected = books.take(MaxBooks)
        val output = ByteBuffer.allocate(HeaderBytes + selected.size * EntryBytes).order(ByteOrder.BIG_ENDIAN)
        output.put("LIBR".toByteArray(StandardCharsets.US_ASCII))
        output.put(Version.toByte())
        output.put(selected.size.toByte())
        output.put((if (language == "de") 1 else 0).toByte())
        output.put(sortCode(librarySort).toByte())
        output.putInt(books.size)

        selected.forEach { book ->
            output.putLong(book.bookId.mostSignificantBits)
            output.putLong(book.bookId.leastSignificantBits)
            output.putInt(book.revision.toInt())
            putUtf8(output, book.title, MaxTitleBytes)
            putUtf8(output, book.authors, MaxAuthorBytes)
            output.putInt(book.progressTokenIndex.coerceAtLeast(0))
            output.putInt(book.progressFurthestTokenIndex.coerceAtLeast(0))
            output.putInt(book.tokenCount.coerceAtLeast(0))
            output.put(book.progressPercentage.coerceIn(0, 100).toByte())
            output.putInt(epochSeconds(book.progressUpdatedAt).toInt())
            output.putInt(epochSeconds(book.createdAt).toInt())
            val cover = book.coverRevision.takeIf { it > 0 }?.let {
                coverCache.read(book.bookId, it)?.let(::toAdaptiveThumbnail)
            }
            output.put(if (cover == null) 0 else 1)
            output.put(cover?.palette ?: ByteArray(CoverPaletteBytes))
            output.put(cover?.pixels ?: ByteArray(CoverBytes))
        }
        return output.array()
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
            val text = String(Character.toChars(codePoint))
            val bytes = text.toByteArray(StandardCharsets.UTF_8)
            if (byteCount + bytes.size > maxBytes) break
            output.appendCodePoint(codePoint)
            byteCount += bytes.size
            index += Character.charCount(codePoint)
        }
        return output.toString().toByteArray(StandardCharsets.UTF_8)
    }

    private data class EncodedCover(val palette: ByteArray, val pixels: ByteArray)

    private data class ColorBucket(val colors: List<Int>, val weight: Int)

    private fun toAdaptiveThumbnail(bytes: ByteArray): EncodedCover? {
        val bounds = BitmapFactory.Options().apply { inJustDecodeBounds = true }
        BitmapFactory.decodeByteArray(bytes, 0, bytes.size, bounds)
        if (bounds.outWidth <= 0 || bounds.outHeight <= 0 ||
            bounds.outWidth > MaxCoverDimension || bounds.outHeight > MaxCoverDimension ||
            bounds.outWidth.toLong() * bounds.outHeight.toLong() > MaxCoverPixels
        ) return null
        var sampleSize = 1
        while (bounds.outWidth / sampleSize > CoverWidth * 4 ||
            bounds.outHeight / sampleSize > CoverHeight * 4
        ) {
            sampleSize *= 2
        }
        val source = BitmapFactory.decodeByteArray(
            bytes,
            0,
            bytes.size,
            BitmapFactory.Options().apply { inSampleSize = sampleSize },
        ) ?: return null
        val scaled = Bitmap.createScaledBitmap(source, CoverWidth, CoverHeight, true)
        val histogram = IntArray(64)
        val quantized = IntArray(CoverWidth * CoverHeight)
        for (y in 0 until CoverHeight) {
            for (x in 0 until CoverWidth) {
                val color = scaled.getPixel(x, y)
                val alpha = Color.alpha(color)
                val red = (Color.red(color) * alpha + 255 * (255 - alpha)) / 255
                val green = (Color.green(color) * alpha + 255 * (255 - alpha)) / 255
                val blue = (Color.blue(color) * alpha + 255 * (255 - alpha)) / 255
                val code = rgb222(red, green, blue)
                quantized[y * CoverWidth + x] = code
                histogram[code] += 1
            }
        }
        val paletteCodes = adaptivePalette(histogram)
        val palette = ByteArray(CoverPaletteBytes)
        paletteCodes.forEachIndexed { index, code -> palette[index] = argb8(code).toByte() }
        val output = ByteArray(CoverBytes)
        for (y in 0 until CoverHeight) {
            for (x in 0 until CoverWidth) {
                val code = quantized[y * CoverWidth + x]
                val paletteIndex = nearestPaletteIndex(code, paletteCodes)
                val byteIndex = y * CoverRowBytes + x / 2
                val shift = if (x % 2 == 0) 4 else 0
                output[byteIndex] = (output[byteIndex].toInt() or (paletteIndex shl shift)).toByte()
            }
        }
        if (scaled !== source) scaled.recycle()
        source.recycle()
        return EncodedCover(palette, output)
    }

    private fun rgb222(red: Int, green: Int, blue: Int): Int =
        (((red + 42) / 85).coerceIn(0, 3) shl 4) or
            (((green + 42) / 85).coerceIn(0, 3) shl 2) or
            ((blue + 42) / 85).coerceIn(0, 3)

    private fun argb8(code: Int): Int = 0xc0 or code

    private fun channel(code: Int, axis: Int): Int = when (axis) {
        0 -> (code shr 4) and 3
        1 -> (code shr 2) and 3
        else -> code and 3
    }

    private fun adaptivePalette(histogram: IntArray): IntArray {
        val used = histogram.indices.filter { histogram[it] > 0 }
        if (used.isEmpty()) return IntArray(CoverPaletteBytes) { 0x3f }
        val buckets = mutableListOf(ColorBucket(used, used.sumOf { histogram[it] }))
        while (buckets.size < CoverPaletteBytes) {
            val splitIndex = buckets.indices
                .filter { buckets[it].colors.size > 1 }
                .maxByOrNull { bucketScore(buckets[it], histogram) } ?: break
            val bucket = buckets.removeAt(splitIndex)
            val axis = widestAxis(bucket.colors)
            val sorted = bucket.colors.sortedBy { channel(it, axis) }
            val half = bucket.weight / 2
            var accumulated = 0
            var cut = 1
            for (index in 0 until sorted.lastIndex) {
                accumulated += histogram[sorted[index]]
                cut = index + 1
                if (accumulated >= half) break
            }
            val left = sorted.subList(0, cut)
            val right = sorted.subList(cut, sorted.size)
            buckets.add(splitIndex, ColorBucket(right, right.sumOf { histogram[it] }))
            buckets.add(splitIndex, ColorBucket(left, left.sumOf { histogram[it] }))
        }
        val representatives = buckets.map { representative(it.colors, histogram) }.toMutableList()
        while (representatives.size < CoverPaletteBytes) representatives += representatives.last()
        return representatives.toIntArray()
    }

    private fun widestAxis(colors: List<Int>): Int = (0..2).maxBy { axis ->
        colors.maxOf { channel(it, axis) } - colors.minOf { channel(it, axis) }
    }

    private fun bucketScore(bucket: ColorBucket, histogram: IntArray): Long {
        val range = (0..2).maxOf { axis ->
            bucket.colors.maxOf { channel(it, axis) } - bucket.colors.minOf { channel(it, axis) }
        }
        val variance = bucket.colors.sumOf { code ->
            histogram[code].toLong() * (range + 1L)
        }
        return variance
    }

    private fun representative(colors: List<Int>, histogram: IntArray): Int {
        val total = colors.sumOf { histogram[it] }.coerceAtLeast(1)
        val means = IntArray(3) { axis ->
            colors.sumOf { channel(it, axis) * histogram[it] }.toDouble().div(total).toInt()
        }
        return colors.minBy { code ->
            val dr = channel(code, 0) - means[0]
            val dg = channel(code, 1) - means[1]
            val db = channel(code, 2) - means[2]
            dr * dr * 3 + dg * dg * 6 + db * db * 2
        }
    }

    private fun nearestPaletteIndex(code: Int, palette: IntArray): Int {
        var bestIndex = 0
        var bestDistance = Int.MAX_VALUE
        palette.forEachIndexed { index, candidate ->
            val deltaRed = channel(code, 0) - channel(candidate, 0)
            val deltaGreen = channel(code, 1) - channel(candidate, 1)
            val deltaBlue = channel(code, 2) - channel(candidate, 2)
            val distance = deltaRed * deltaRed * 3 + deltaGreen * deltaGreen * 6 + deltaBlue * deltaBlue * 2
            if (distance < bestDistance) {
                bestDistance = distance
                bestIndex = index
            }
        }
        return bestIndex
    }

    private fun sortCode(value: String): Int = when (value) {
        "title" -> 1
        "author" -> 2
        "recent" -> 3
        "progress" -> 4
        else -> 0
    }

    private fun epochSeconds(value: String): Long {
        if (value.isBlank()) return 0
        val parsed = runCatching {
            SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'", Locale.US).apply {
                isLenient = false
                timeZone = TimeZone.getTimeZone("UTC")
            }.parse(value)
        }.getOrNull() ?: return 0
        return (parsed.time / 1_000L).coerceIn(0L, 0xffffffffL)
    }

    private const val MaxCoverDimension = 12_000
    private const val MaxCoverPixels = 40_000_000L
}
