package dev.pebblersvp.android.protocol

import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.charset.StandardCharsets
import java.util.UUID
import java.util.zip.CRC32

enum class MessageType(val code: Int) {
    Data(1),
    Ack(2),
    Nack(3),
    Request(4),
    Progress(5),
    Settings(6);

    companion object {
        fun fromCode(code: Int): MessageType =
            entries.firstOrNull { it.code == code } ?: error("unknown message type code: $code")
    }
}

data class RsvpPacket(
    val messageType: MessageType,
    val requestId: Long,
    val bookId: UUID = ZeroUuid,
    val bookRevision: Long = 0,
    val chunkId: Long = 0,
    val sequenceNumber: Int = 0,
    val sequenceCount: Int = 1,
    val flags: Int = 0,
    val payload: ByteArray = ByteArray(0),
) {
    fun encode(): ByteArray {
        require(requestId in UIntRange) { "requestId must fit uint32" }
        require(bookRevision in UIntRange) { "bookRevision must fit uint32" }
        require(chunkId in UIntRange) { "chunkId must fit uint32" }
        require(sequenceNumber in UShortRange) { "sequenceNumber must fit uint16" }
        require(sequenceCount in UShortRange && sequenceCount > 0) { "sequenceCount must fit uint16 and be non-zero" }
        require(sequenceNumber < sequenceCount) { "sequenceNumber must be less than sequenceCount" }
        require(payload.size in UShortRange) { "payload must fit uint16 length" }

        return ByteBuffer.allocate(HeaderLength + payload.size).order(ByteOrder.BIG_ENDIAN).apply {
            put(Magic.toByteArray(StandardCharsets.US_ASCII))
            put(ProtocolVersion.toByte())
            put(messageType.code.toByte())
            put(flags.toByte())
            put(HeaderLength.toByte())
            putUInt32(requestId)
            putUInt32(bookRevision)
            putUInt32(chunkId)
            putShort(sequenceNumber.toShort())
            putShort(sequenceCount.toShort())
            putShort(payload.size.toShort())
            putUInt32(crc32(payload))
            putLong(bookId.mostSignificantBits)
            putLong(bookId.leastSignificantBits)
            put(payload)
        }.array()
    }

    fun ack(payload: ByteArray = ByteArray(0)): RsvpPacket =
        copy(messageType = MessageType.Ack, payload = payload)

    fun nack(reason: String): RsvpPacket =
        copy(messageType = MessageType.Nack, payload = reason.toByteArray(StandardCharsets.UTF_8))

    companion object {
        const val ProtocolVersion = 1
        const val Magic = "PRSP"
        const val HeaderLength = 46
        val ZeroUuid: UUID = UUID.fromString("00000000-0000-0000-0000-000000000000")

        private val UIntRange = 0L..0xffffffffL
        private val UShortRange = 0..0xffff

        fun decode(bytes: ByteArray): RsvpPacket {
            require(bytes.size >= HeaderLength) { "packet is shorter than the protocol header" }
            val buffer = ByteBuffer.wrap(bytes).order(ByteOrder.BIG_ENDIAN)
            val magic = ByteArray(4)
            buffer.get(magic)
            require(String(magic, StandardCharsets.US_ASCII) == Magic) { "packet magic mismatch" }

            val protocolVersion = buffer.get().toInt() and 0xff
            require(protocolVersion == ProtocolVersion) { "unsupported protocol version: $protocolVersion" }
            val messageType = MessageType.fromCode(buffer.get().toInt() and 0xff)
            val flags = buffer.get().toInt() and 0xff
            val headerLength = buffer.get().toInt() and 0xff
            require(headerLength == HeaderLength) { "unsupported header length: $headerLength" }

            val requestId = buffer.getUInt32()
            val bookRevision = buffer.getUInt32()
            val chunkId = buffer.getUInt32()
            val sequenceNumber = buffer.short.toInt() and 0xffff
            val sequenceCount = buffer.short.toInt() and 0xffff
            val payloadLength = buffer.short.toInt() and 0xffff
            val expectedChecksum = buffer.getUInt32()
            val bookId = UUID(buffer.long, buffer.long)

            require(sequenceCount > 0 && sequenceNumber < sequenceCount) { "invalid packet sequence fields" }
            require(bytes.size == HeaderLength + payloadLength) { "packet payload length mismatch" }
            val payload = bytes.copyOfRange(HeaderLength, bytes.size)
            require(expectedChecksum == crc32(payload)) { "packet checksum mismatch" }

            return RsvpPacket(
                messageType = messageType,
                requestId = requestId,
                bookId = bookId,
                bookRevision = bookRevision,
                chunkId = chunkId,
                sequenceNumber = sequenceNumber,
                sequenceCount = sequenceCount,
                flags = flags,
                payload = payload,
            )
        }

        fun splitPayload(
            requestId: Long,
            bookId: UUID,
            bookRevision: Long,
            chunkId: Long,
            payload: ByteArray,
            maxPayloadLength: Int,
        ): List<ByteArray> {
            require(maxPayloadLength > 0) { "maxPayloadLength must be positive" }
            val sequenceCount = maxOf(1, (payload.size + maxPayloadLength - 1) / maxPayloadLength)
            return (0 until sequenceCount).map { sequenceNumber ->
                val start = sequenceNumber * maxPayloadLength
                val end = minOf(start + maxPayloadLength, payload.size)
                RsvpPacket(
                    messageType = MessageType.Data,
                    requestId = requestId,
                    bookId = bookId,
                    bookRevision = bookRevision,
                    chunkId = chunkId,
                    sequenceNumber = sequenceNumber,
                    sequenceCount = sequenceCount,
                    payload = payload.copyOfRange(start, end),
                ).encode()
            }
        }

        private fun crc32(bytes: ByteArray): Long {
            val crc = CRC32()
            crc.update(bytes)
            return crc.value and 0xffffffffL
        }

        private fun ByteBuffer.putUInt32(value: Long) {
            putInt((value and 0xffffffffL).toInt())
        }

        private fun ByteBuffer.getUInt32(): Long =
            int.toLong() and 0xffffffffL
    }
}
