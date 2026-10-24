package dev.pebblersvp.android.protocol

import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Test
import java.util.UUID

class RsvpPacketTest {
    @Test
    fun roundTripsReaderSettingsMessages() {
        val payload = byteArrayOf(1, 1, 2)
        val packet = RsvpPacket(
            messageType = MessageType.Settings,
            requestId = 42,
            payload = payload,
        )

        val decoded = RsvpPacket.decode(packet.encode())
        assertEquals(MessageType.Settings, decoded.messageType)
        assertArrayEquals(payload, decoded.payload)
    }

    @Test
    fun rejectsTransfersWithExcessiveFragmentCounts() {
        val packet = RsvpPacket(
            messageType = MessageType.Data,
            requestId = 1,
            sequenceNumber = 0,
            sequenceCount = 513,
            payload = byteArrayOf(1),
        )

        val receipt = TransferReceiver().receive(packet.encode())

        assertEquals(TransferStatus.Nack, receipt.status)
        assertEquals("transfer_too_large", receipt.reason)
    }

    @Test
    fun capsConcurrentInboundTransfers() {
        val receiver = TransferReceiver()
        repeat(8) { index ->
            val receipt = receiver.receive(
                RsvpPacket(
                    messageType = MessageType.Data,
                    requestId = index.toLong(),
                    bookId = UUID(0, index.toLong() + 1),
                    sequenceNumber = 0,
                    sequenceCount = 2,
                    payload = byteArrayOf(1),
                ).encode(),
            )
            assertEquals(TransferStatus.Ack, receipt.status)
        }

        val overflow = receiver.receive(
            RsvpPacket(
                messageType = MessageType.Data,
                requestId = 99,
                bookId = UUID(0, 99),
                sequenceNumber = 0,
                sequenceCount = 2,
                payload = byteArrayOf(1),
            ).encode(),
        )

        assertEquals(TransferStatus.Nack, overflow.status)
        assertEquals("too_many_transfers", overflow.reason)
    }
}
