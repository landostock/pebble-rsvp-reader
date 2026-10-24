package dev.pebblersvp.android.transport

import android.content.Context
import dev.pebblersvp.android.AppConstants
import io.rebble.pebblekit2.client.DefaultPebbleSender
import io.rebble.pebblekit2.common.model.PebbleDictionaryItem
import io.rebble.pebblekit2.common.model.TransmissionResult
import io.rebble.pebblekit2.common.model.WatchIdentifier
import kotlinx.coroutines.delay
import java.io.Closeable
import java.io.IOException

class PebblePacketBridge(context: Context) : Closeable {
    private val sender = DefaultPebbleSender(context)

    suspend fun sendPacket(
        packet: ByteArray,
        watch: WatchIdentifier? = null,
    ): Map<WatchIdentifier, TransmissionResult>? =
        sender.sendDataToPebble(
            AppConstants.WatchAppUuid,
            mapOf(AppConstants.PacketKey to PebbleDictionaryItem.Bytes(packet)),
            if (watch == null) null else listOf(watch),
        )

    suspend fun sendPacketsSequentially(
        packets: List<ByteArray>,
        watch: WatchIdentifier? = null,
    ): List<Map<WatchIdentifier, TransmissionResult>?> {
        val results = mutableListOf<Map<WatchIdentifier, TransmissionResult>?>()
        for (packet in packets) {
            results += sendPacket(packet, watch)
        }
        return results
    }

    suspend fun sendPacketReliably(
        packet: ByteArray,
        watch: WatchIdentifier,
        attempts: Int = 4,
    ): Map<WatchIdentifier, TransmissionResult> {
        require(attempts > 0)
        var lastResult: TransmissionResult? = null
        repeat(attempts) { attempt ->
            val results = sendPacket(packet, watch)
            lastResult = results?.get(watch)
            if (lastResult is TransmissionResult.Success) return requireNotNull(results)
            if (lastResult is TransmissionResult.FailedNoPermissions ||
                lastResult is TransmissionResult.FailedDifferentAppOpen
            ) {
                throw IOException("Pebble packet rejected: $lastResult")
            }
            if (attempt + 1 < attempts) delay(RetryDelaysMs[attempt])
        }
        throw IOException("Pebble packet delivery failed after $attempts attempts: $lastResult")
    }

    override fun close() {
        sender.close()
    }

    private companion object {
        val RetryDelaysMs = longArrayOf(40, 120, 320)
    }
}
