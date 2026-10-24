package dev.pebblersvp.android.protocol

import java.io.ByteArrayOutputStream

enum class TransferStatus {
    Control,
    Ack,
    Nack,
    Duplicate,
    Complete,
}

data class TransferReceipt(
    val status: TransferStatus,
    val packet: RsvpPacket? = null,
    val payload: ByteArray? = null,
    val reason: String? = null,
    val expectedSequence: Int? = null,
    val response: ByteArray? = null,
)

class TransferReceiver {
    private data class TransferKey(
        val requestId: Long,
        val bookId: String,
        val bookRevision: Long,
        val chunkId: Long,
    )

    private data class TransferState(
        val sequenceCount: Int,
        var nextSequence: Int = 0,
        var byteCount: Int = 0,
        val parts: MutableList<ByteArray> = mutableListOf(),
    )

    private val seenPackets = mutableSetOf<String>()
    private val transfers = mutableMapOf<TransferKey, TransferState>()

    fun receive(bytes: ByteArray): TransferReceipt {
        val packet = try {
            RsvpPacket.decode(bytes)
        } catch (error: IllegalArgumentException) {
            return TransferReceipt(status = TransferStatus.Nack, reason = error.message)
        }

        if (packet.messageType != MessageType.Data) {
            return TransferReceipt(status = TransferStatus.Control, packet = packet)
        }
        if (packet.sequenceCount > MaxSequenceCount) {
            return TransferReceipt(
                status = TransferStatus.Nack,
                packet = packet,
                reason = "transfer_too_large",
                response = packet.nack("transfer_too_large").encode(),
            )
        }

        val seenKey = packetKey(packet)
        if (seenKey in seenPackets) {
            return TransferReceipt(status = TransferStatus.Duplicate, packet = packet, response = packet.ack().encode())
        }

        val key = transferKey(packet)
        if (key !in transfers && transfers.size >= MaxActiveTransfers) {
            return TransferReceipt(
                status = TransferStatus.Nack,
                packet = packet,
                reason = "too_many_transfers",
                response = packet.nack("too_many_transfers").encode(),
            )
        }
        val transfer = transfers.getOrPut(key) { TransferState(sequenceCount = packet.sequenceCount) }
        if (packet.sequenceCount != transfer.sequenceCount) {
            return TransferReceipt(
                status = TransferStatus.Nack,
                packet = packet,
                reason = "sequence_count_changed",
                response = packet.nack("sequence_count_changed").encode(),
            )
        }
        if (packet.sequenceNumber != transfer.nextSequence) {
            return TransferReceipt(
                status = TransferStatus.Nack,
                packet = packet,
                reason = "out_of_order",
                expectedSequence = transfer.nextSequence,
                response = packet.nack("out_of_order:${transfer.nextSequence}").encode(),
            )
        }
        if (transfer.byteCount + packet.payload.size > MaxTransferBytes) {
            transfers.remove(key)
            return TransferReceipt(
                status = TransferStatus.Nack,
                packet = packet,
                reason = "transfer_too_large",
                response = packet.nack("transfer_too_large").encode(),
            )
        }

        transfer.parts += packet.payload
        transfer.byteCount += packet.payload.size
        transfer.nextSequence += 1
        seenPackets += seenKey
        while (seenPackets.size > MaxSeenPackets) {
            seenPackets.remove(seenPackets.first())
        }

        if (transfer.nextSequence == transfer.sequenceCount) {
            transfers.remove(key)
            val output = ByteArrayOutputStream()
            for (part in transfer.parts) {
                output.write(part)
            }
            return TransferReceipt(
                status = TransferStatus.Complete,
                packet = packet,
                payload = output.toByteArray(),
                response = packet.ack().encode(),
            )
        }

        return TransferReceipt(
            status = TransferStatus.Ack,
            packet = packet,
            expectedSequence = transfer.nextSequence,
            response = packet.ack().encode(),
        )
    }

    private fun transferKey(packet: RsvpPacket): TransferKey =
        TransferKey(packet.requestId, packet.bookId.toString(), packet.bookRevision, packet.chunkId)

    private fun packetKey(packet: RsvpPacket): String =
        "${packet.requestId}:${packet.bookId}:${packet.bookRevision}:${packet.chunkId}:${packet.sequenceNumber}"

    private companion object {
        const val MaxActiveTransfers = 8
        const val MaxSequenceCount = 512
        const val MaxTransferBytes = 256 * 1024
        const val MaxSeenPackets = 2_048
    }
}
