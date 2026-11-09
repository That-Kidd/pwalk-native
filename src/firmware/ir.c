#include "pw_internal.h"

uint16_t calcPacketCrc(pw_state_t *s, uint16_t data, uint32_t len) {
    uint32_t sum = 2;
    for (uint32_t i = 0; i < len; i++) {
        uint8_t b = rd8(s, (uint16_t)(data + i));
        sum += i & 1 ? b : (uint32_t)b << 8;
    }
    uint32_t r = (sum >> 16) + (sum & 0xFFFF);
    r += r >> 16;
    return (uint16_t)r;
}

void decompress(pw_state_t *s, uint16_t src, uint16_t dst) {
    uint8_t left = rd8(s, src + 1);
    src += 4;
    while (left) {
        uint8_t flags = rd8(s, src++);
        for (int n = 0; n < 8; n++) {
            if (!(flags & 0x80)) {
                wr8(s, dst++, rd8(s, src++));
                left--;
            } else {
                uint8_t run = (rd8(s, src) >> 4) + 3;
                uint16_t back = rd8(s, src + 1) + 1;
                src += 2;
                left -= run;
                do {
                    wr8(s, dst, rd8(s, (uint16_t)(dst - back)));
                    dst++;
                } while (--run);
            }
            if (!left) {
                break;
            }
            flags <<= 1;
        }
    }
}

void irTxByte(pw_state_t *s, uint8_t byte) {
    if (s->hal && s->hal->ir_send) {
        s->hal->ir_send(s->hal->user, byte ^ 0xAA);
    }
}

void sendPacket(pw_state_t *s, uint8_t len, uint8_t cmd, uint8_t extra) {
    wr8(s, RAM_TX_PACKET_cmd, cmd);
    wr8(s, RAM_TX_PACKET_extraByte, extra);
    wr32(s, RAM_TX_PACKET_session_0, rd32(s, RAM_CurrentSessionID));
    wr8(s, RAM_TX_PACKET_crc_0, 0);
    wr8(s, RAM_TX_PACKET_crc_1, 0);
    uint16_t crc = calcPacketCrc(s, RAM_TX_PACKET_cmd, (uint32_t)len + 8);
    wr8(s, RAM_TX_PACKET_crc_0, (uint8_t)crc);
    wr8(s, RAM_TX_PACKET_crc_1, crc >> 8);
    len += 8;
    for (uint16_t i = 0; i < len; i++) {
        irTxByte(s, rd8(s, RAM_TX_PACKET_cmd + i));
    }
}

#define EEPROM_PEERS     0xDE24
#define PEER_RECORD_SIZE 0x224
#define PEER_ID_SIZE     0x28

uint8_t seeIfWeSawThisTrainerBefore(pw_state_t *s, uint16_t id) {
    for (uint8_t i = 0; i < 10; i++) {
        uint16_t rec = EEPROM_PEERS + i * PEER_RECORD_SIZE;
        eepromReadToRamAlso(s, rec + 8, RAM_DECOMPRESSION_BUFFER, PEER_ID_SIZE);
        uint8_t same = 1;
        for (uint8_t k = 0; k < PEER_ID_SIZE; k++) {
            if (rd8(s, RAM_DECOMPRESSION_BUFFER + k) != rd8(s, id + k)) {
                same = 0;
            }
        }
        if (same) {
            return 1;
        }
    }
    return 0;
}

void processSyncPacketFromGame(pw_state_t *s) {
    memoryCopy(s, get_packet_data_pointer(s), RAM_sometimes_new_rxed_data_for_0xED, 0x68);
    uint8_t b = rd8(s, RAM_byte_FFF841);
    if ((b & 0xF8) < 0xC0) {
        uint8_t v = b >> 3;
        wr8(s, RAM_byte_FFF7A8, (uint8_t)(v / 10 * 16 | v % 10));
    }
    uint32_t t = rd32(s, RAM_dword_FFF846);
    if (t) {
        wr32(s, RAM_RamCache_lastSyncTime, t);
        rtcSet(s, t);
    }
}

#define IR_ADVERTS 0x14
#define IR_TIMEOUT 3200

#define PKT     RAM_TX_PACKET_cmd
#define PAYLOAD RAM_TX_PACKET_payload
#define RX_MAX  0x88
#define IR_GAP  4

#define EEPROM_IDENTITY0   0x00ED
#define EEPROM_IDENTITY1   0x01ED
#define IDENTITY_SIZE      0x68
#define EEPROM_EVENT_FLAGS 0xB800

static void fail(pw_state_t *s, uint8_t msg) {
    wr8(s, RAM_commsErrorMessageId, msg);
    maybe_shutDownIr_then_performActionAsRequested(s);
}

static void save_health(pw_state_t *s) {
    writeReliableDataToEeprom(s, EEPROM_HEALTH0, EEPROM_HEALTH1, RAM_RamCache_totalSteps, HEALTH_SIZE);
}

static void advertise(pw_state_t *s) {
    uint16_t pause = (uint16_t)((rand32(s) >> 5 & 0xF) * 0x60);
    uint64_t until = s->hw.now + pause;
    while (hw_wait(s, until, 0)) {
    }
    wr8(s, RAM_ourPeerRole, 1);
    irTxByte(s, 0xFC);
}

static void peer_write_chunk(pw_state_t *s) {
    uint16_t n = rd16(s, RAM_PeerPlayEepromXferSizeLeft);
    if (n > 0x80)
        n = 0x80;
    eepromReadToRamAlso(s, rd16(s, RAM_PeerPlayEepromXferLocalAddr), PAYLOAD, n);
    uint16_t remote = rd16(s, RAM_PeerPlayEepromXferRemoteAddr);
    sendPacket(s, (uint8_t)n, (remote & 0x80) | 2, remote >> 8);
    wr16(s, RAM_PeerPlayEepromXferLocalAddr, rd16(s, RAM_PeerPlayEepromXferLocalAddr) + n);
    wr16(s, RAM_PeerPlayEepromXferRemoteAddr, remote + n);
    wr16(s, RAM_PeerPlayEepromXferSizeLeft, rd16(s, RAM_PeerPlayEepromXferSizeLeft) - n);
    wr8(s, RAM_PeerPlayEepromXfreNumPacketsSentSoFar, rd8(s, RAM_PeerPlayEepromXfreNumPacketsSentSoFar) + 1);
}

static void peer_read_chunk(pw_state_t *s) {
    uint16_t n = rd16(s, RAM_PeerPlayEepromXferSizeLeft);
    if (n > 0x80)
        n = 0x80;
    uint16_t local = rd16(s, RAM_PeerPlayEepromXferLocalAddr);
    wr8(s, PAYLOAD, local >> 8);
    wr8(s, PAYLOAD + 1, (uint8_t)local);
    wr8(s, PAYLOAD + 2, (uint8_t)n);
    sendPacket(s, 3, 0x0C, 2);
}

static void peer_transfer(pw_state_t *s, uint8_t stage, uint16_t local, uint16_t remote, uint16_t size) {
    wr8(s, RAM_PeerPlayStage, stage);
    wr16(s, RAM_PeerPlayEepromXferLocalAddr, local);
    wr16(s, RAM_PeerPlayEepromXferRemoteAddr, remote);
    wr16(s, RAM_PeerPlayEepromXferSizeLeft, size);
    wr8(s, RAM_PeerPlayEepromXfreNumPacketsSentSoFar, 0);
}

static void peer_summary(pw_state_t *s) {
    uint16_t buf = RAM_DECOMPRESSION_BUFFER, p = PAYLOAD;
    wr32(s, p, rd32(s, RAM_stepCountTodaySoFar));
    wr16(s, p + 4, rd16(s, RAM_someWalkingRelatedState));
    eepromReadToRamAlso(s, 0x8F00, buf, 0x10);
    wr8(s, p + 0x37, (rd8(s, p + 0x37) & ~1) | (rd8(s, buf + 0xE) & 1));
    wr16(s, p + 0xE, rd16(s, buf));
    wr8(s, p + 0x36, (rd8(s, p + 0x36) & 0xE0) | (rd8(s, buf + 0xD) & 0x1F));
    bitfieldInsert(s, p + 0x36, rd8(s, buf + 0xD) >> 5 & 3, 2, 1);
    wr8(s, p + 0x36, (rd8(s, p + 0x36) & 0x7F) | (rd8(s, buf + 0xE) >> 1 & 1) << 7);
    readReliableDataFromEeprom(s, EEPROM_IDENTITY0, EEPROM_IDENTITY1, RAM_RamCache_ED_data, IDENTITY_SIZE);
    wr32(s, p + 8, rd32(s, RAM_RamCache_ED_data));
    wr16(s, p + 0xC, rd16(s, RAM_word_FFF856));
    for (int i = 0; i < 0x10; i++) {
        wr8(s, p + 0x26 + i, rd8(s, RAM_unk_FFF896 + i));
    }
    eepromReadToRamAlso(s, 0x8F10, p + 0x10, 0x16);
    sendPacket(s, 0x38, 0x14, 2);
}

static uint8_t check_peer(pw_state_t *s, int reply) {
    memoryCopy(s, PAYLOAD, RAM_sometimes_new_rxed_data_for_0xED, IDENTITY_SIZE);
    readReliableDataFromEeprom(s, EEPROM_IDENTITY0, EEPROM_IDENTITY1, PAYLOAD, IDENTITY_SIZE);
    uint8_t ours = rd8(s, PAYLOAD + 0x5B), theirs = rd8(s, RAM_byte_FFF841);
    uint8_t msg = 0;
    if (!(ours & 1) || !(theirs & 1) || !(theirs & 2) || rd8(s, PAYLOAD + 0x5C) != rd8(s, RAM_byte_FFF842) ||
        rd8(s, RAM_byte_FFF844)) {
        msg = 3;
    } else if (!(ours & 2)) {
        msg = 4;
    }
    if (msg) {
        if (reply) {
            sendPacket(s, IDENTITY_SIZE, 0x12, 2);
        }
        return msg;
    }
    if (seeIfWeSawThisTrainerBefore(s, RAM_unk_FFF7F6)) {
        sendPacket(s, 0, 0x1C, 2);
        return 5;
    }
    return 0;
}

static void event_gift(pw_state_t *s, uint8_t cmd) {
    static const uint8_t full[4] = { 0x10, 0x20, 0x40, 0x80 };
    static const uint8_t part[4] = { 0x1F, 0x2F, 0x4F, 0x8F };
    uint8_t k = (cmd & 0xF) >> 1;
    uint8_t b = eepromReadSingleByte(s, EEPROM_EVENT_FLAGS);
    b |= cmd & 0x10 ? part[k] : full[k];
    eepromWriteSingleByte(s, EEPROM_EVENT_FLAGS, b);
    setThatWeveParticipatedInAGivenEvent(s, RAM_RamCache_ED_data, rd8(s, RAM_byte_FFF840));
    if (k == 3) {
        wr8(s, RAM_RamCache_settingsByte, rd8(s, RAM_RamCache_settingsByte) | 1);
        save_health(s);
    }
    sendPacket(s, 0, 0xC0 | (k << 1), 2);
    wr8(s, RAM_REQUESTED_POKEMON_ACTION_TYPE, 0xC0 | (k << 1));
}

static int event_offer(pw_state_t *s, uint8_t cmd) {
    processSyncPacketFromGame(s);
    for (int i = 0; i < 0x10; i++) {
        wr8(s, PAYLOAD + i, rd8(s, RAM_unk_FFF886 + i));
    }
    wr8(s, PAYLOAD + 0x10, rd8(s, RAM_byte_FFF840));
    if (hasParticipatedInEvent(s, RAM_RamCache_ED_data, rd8(s, RAM_byte_FFF840))) {
        sendPacket(s, 0x11, 0x9E, 2);
        fail(s, 6);
        return 1;
    }
    sendPacket(s, 0x11, cmd, 2);
    return 0;
}

static void handle_packet(pw_state_t *s, uint8_t entry_y_bit, uint16_t size_left) {
    uint8_t cmd = rd8(s, PKT), dir = rd8(s, PKT + 1);
    uint8_t plen = rd8(s, RAM_NumBytesRxedsoFar) - 8;
    uint32_t session = rd32(s, PKT + 4);
    uint16_t hi = (uint16_t)(dir << 8);

    if (cmd < 0xF8 && (session != rd32(s, RAM_CurrentSessionID) || rd8(s, RAM_ourPeerRole) < 3)) {
        return;
    }
    switch (cmd) {
    case 0xFA:
        if (dir != 1 && dir != 2) {
            fail(s, 3);
            return;
        }
        switch (rd8(s, RAM_ourPeerRole)) {
        case 1:
            wr8(s, RAM_ourPeerRole, 3);
            sendPacket(s, 0, 0xF8, 2);
            wr32(s, RAM_CurrentSessionID, session ^ rd32(s, RAM_SuggestedOurSideSessionId));
            return;
        case 2:
        case 3:
        case 4:
            advertise(s);
            wr16(s, RAM_TimeWhenLAstByteRxed, hw_ir_tcnt(s));
            return;
        default: return;
        }
    case 0xF8:
        if (dir != 2) {
            fail(s, 3);
            return;
        }
        if (rd8(s, RAM_ourPeerRole) >= 3) {
            return;
        }
        wr32(s, RAM_CurrentSessionID, session ^ rd32(s, RAM_SuggestedOurSideSessionId));
        wr8(s, RAM_ourPeerRole, 4);
        readReliableDataFromEeprom(s, EEPROM_IDENTITY0, EEPROM_IDENTITY1, PAYLOAD, IDENTITY_SIZE);
        sendPacket(s, IDENTITY_SIZE, 0x10, 2);
        wr8(s, RAM_gCurSubstateY, rd8(s, RAM_gCurSubstateY) & ~1);
        return;
    case 0xF4: maybe_shutDownIr_then_performActionAsRequested(s); return;

    case 0x10: {
        wr8(s, RAM_gCurSubstateY, rd8(s, RAM_gCurSubstateY) | 1);
        uint8_t msg = check_peer(s, 1);
        if (msg) {
            fail(s, msg);
            return;
        }
        sendPacket(s, IDENTITY_SIZE, 0x12, 2);
        return;
    }
    case 0x12: {
        uint8_t msg = check_peer(s, 0);
        if (msg) {
            fail(s, msg);
            return;
        }
        peer_transfer(s, 1, 0x91BE, 0xF400, 0x180);
        peer_write_chunk(s);
        return;
    }
    case 0x04:
        if (size_left) {
            peer_write_chunk(s);
            return;
        }
        switch (rd8(s, RAM_PeerPlayStage)) {
        case 1:
            peer_transfer(s, 3, 0x993E, 0xF580, 0x140);
            peer_write_chunk(s);
            return;
        case 3:
            peer_transfer(s, 5, 0xCC00, 0xDC00, 0x224);
            peer_write_chunk(s);
            return;
        case 5:
            peer_transfer(s, 2, 0x91BE, 0xF400, 0x180);
            peer_read_chunk(s);
            return;
        default: return;
        }
    case 0x0E: {
        eepromWriteData(s, rd16(s, RAM_PeerPlayEepromXferRemoteAddr), PAYLOAD, plen);
        wr16(s, RAM_PeerPlayEepromXferLocalAddr, rd16(s, RAM_PeerPlayEepromXferLocalAddr) + plen);
        wr16(s, RAM_PeerPlayEepromXferRemoteAddr, rd16(s, RAM_PeerPlayEepromXferRemoteAddr) + plen);
        wr16(s, RAM_PeerPlayEepromXferSizeLeft, rd16(s, RAM_PeerPlayEepromXferSizeLeft) - plen);
        wr8(s, RAM_PeerPlayEepromXfreNumPacketsSentSoFar, rd8(s, RAM_PeerPlayEepromXfreNumPacketsSentSoFar) + 1);
        if (rd16(s, RAM_PeerPlayEepromXferSizeLeft)) {
            peer_read_chunk(s);
            return;
        }
        switch (rd8(s, RAM_PeerPlayStage)) {
        case 2:
            peer_transfer(s, 4, 0x993E, 0xF580, 0x140);
            peer_read_chunk(s);
            return;
        case 4:
            peer_transfer(s, 6, 0xCC00, 0xDC00, 0x224);
            peer_read_chunk(s);
            return;
        case 6: peer_summary(s); return;
        default: return;
        }
    }
    case 0x14:
        eepromWriteData(s, 0xF6C0, PAYLOAD, 0x38);
        if (entry_y_bit) {
            peer_summary(s);
        } else {
            sendPacket(s, 0, 0x16, 2);
        }
        return;
    case 0x16:
        if (entry_y_bit) {
            sendPacket(s, 0, 0x16, 2);
        }
        wr8(s, RAM_REQUESTED_POKEMON_ACTION_TYPE, 0x16);
        maybe_shutDownIr_then_performActionAsRequested(s);
        return;
    case 0x1C: fail(s, 5); return;

    case 0x20:
        readReliableDataFromEeprom(s, EEPROM_IDENTITY0, EEPROM_IDENTITY1, PAYLOAD, IDENTITY_SIZE);
        wr32(s, PAYLOAD + 0x64, rd32(s, RAM_RamCache_totalSteps));
        save_health(s);
        eepromWriteData(s, 0xCE8A, RAM_RamCache_curWatts, 2);
        memoryCopy(s, PAYLOAD, RAM_RamCache_ED_data, IDENTITY_SIZE);
        sendPacket(s, IDENTITY_SIZE, 0x22, 2);
        return;
    case 0x32:
    case 0x40:
    case 0x52:
        processSyncPacketFromGame(s);
        save_health(s);
        sendPacket(s, 0, cmd + 2, 2);
        return;
    case 0x60:
        processSyncPacketFromGame(s);
        readReliableDataFromEeprom(s, EEPROM_IDENTITY0, EEPROM_IDENTITY1, RAM_RamCache_ED_data, IDENTITY_SIZE);
        sendPacket(s, 0, 0x62, 2);
        return;
    case 0x24: sendPacket(s, 0, 0x26, 2); return;
    case 0x38:
        sendPacket(s, 0, 0x38, 2);
        wr8(s, RAM_REQUESTED_POKEMON_ACTION_TYPE, 0x38);
        return;
    case 0x66:
        sendPacket(s, 0, 0x68, 2);
        wr8(s, RAM_REQUESTED_POKEMON_ACTION_TYPE, 0x66);
        return;
    case 0x4E:
        sendPacket(s, 0, 0x50, 2);
        wr8(s, RAM_REQUESTED_POKEMON_ACTION_TYPE, 0x4E);
        maybe_shutDownIr_then_performActionAsRequested(s);
        return;
    case 0x5A:
        sendPacket(s, 0, 0x5A, 2);
        wr8(s, RAM_REQUESTED_POKEMON_ACTION_TYPE, 0x5A);
        maybe_shutDownIr_then_performActionAsRequested(s);
        return;
    case 0x36:
    case 0x44:
    case 0x56:
    case 0x64:
    case 0xD8: fail(s, 3); return;

    case 0xA0:
    case 0xA2:
    case 0xA4:
    case 0xA6:
    case 0xA8:
    case 0xAA:
    case 0xAC:
    case 0xAE: event_offer(s, cmd); return;
    case 0xC0:
    case 0xC2:
    case 0xC4:
    case 0xC6:
    case 0xD0:
    case 0xD2:
    case 0xD4:
    case 0xD6:
        event_gift(s, cmd);
        maybe_shutDownIr_then_performActionAsRequested(s);
        return;
    case 0xB8:
    case 0xBA:
    case 0xBC:
    case 0xBE: {
        uint8_t b = eepromReadSingleByte(s, EEPROM_EVENT_FLAGS);
        b |= 1 << ((cmd - 0xB8) >> 1);
        wr8(s, RAM_REQUESTED_POKEMON_ACTION_TYPE, cmd);
        eepromWriteSingleByte(s, EEPROM_EVENT_FLAGS, b);
        sendPacket(s, 0, cmd + 0x10, 2);
        maybe_shutDownIr_then_performActionAsRequested(s);
        return;
    }
    case 0x9C:
    case 0x9E:
        sendPacket(s, 0, cmd, 2);
        fail(s, 7);
        return;

    case 0x00:
    case 0x80: {
        uint16_t addr = hi + cmd;
        if (plen == 0x80) {
            eepromWrite128Bytes(s, addr, PAYLOAD);
        } else {
            decompress(s, PAYLOAD, RAM_DECOMPRESSION_BUFFER);
            eepromWrite128Bytes(s, addr, RAM_DECOMPRESSION_BUFFER);
        }
        sendPacket(s, 0, 0x04, dir);
        return;
    }
    case 0x02:
    case 0x82:
        eepromWriteData(s, hi + (cmd & 0x80), PAYLOAD, plen);
        sendPacket(s, 0, 0x04, dir);
        return;
    case 0x0A:

        eepromWriteData(s, hi + rd8(s, PAYLOAD), PAYLOAD + 1, (uint16_t)(plen - 1));
        sendPacket(s, 0, 0x04, dir);
        return;
    case 0x0C: {
        uint16_t addr = (uint16_t)(rd8(s, PAYLOAD) << 8 | rd8(s, PAYLOAD + 1));
        uint8_t n = rd8(s, PAYLOAD + 2);
        eepromReadToRamAlso(s, addr, PAYLOAD, n);
        sendPacket(s, n, 0x0E, 2);
        return;
    }
    case 0x06: {
        uint16_t addr = hi + rd8(s, PAYLOAD);
        for (int i = 0; i < (int)plen - 1; i++) {
            wr8(s, addr + i, rd8(s, PAYLOAD + 1 + i));
        }
        sendPacket(s, 0, 0x06, dir);
        return;
    }

    case 0xF0: {
        writeReliableDataToEeprom(s, 0x83, 0x183, PAYLOAD, 0x28);
        eepromWriteData(s, 8, PAYLOAD + 0x68, 8);
        wr8(s, RAM_gCurSubstateZ, 1);
        wr8(s, RAM_REQUESTED_POKEMON_ACTION_TYPE, 0xF0);
        switch (rd8(s, PAYLOAD + 0x70)) {
        case 0: writeReliableDataToEeprom(s, 0xAC, 0x1AC, PAYLOAD + 0x28, 0x40); break;
        case 1:
            readReliableDataFromEeprom(s, 0xAC, 0x1AC, RAM_DECOMPRESSION_BUFFER, 0x40);
            for (int i = 0; i < 0x40; i++) {
                if (rd8(s, PAYLOAD + 0x28 + i) != rd8(s, RAM_DECOMPRESSION_BUFFER + i)) {
                    wr8(s, RAM_gCurSubstateZ, 0);
                    break;
                }
            }
            break;
        case 3:
            writeReliableDataToEeprom(s, 0xAC, 0x1AC, PAYLOAD + 0x28, 0x40);
            wr8(s, RAM_REQUESTED_POKEMON_ACTION_TYPE, 0xE0);
            break;
        default: break;
        }
        sendPacket(s, 0x28, 0xF0, 2);
        return;
    }
    case 0xFE:
        if (dir != 1 || plen != 8) {
            return;
        }
        eepromWriteData(s, 8, PAYLOAD, 8);
        sendPacket(s, 0, 0xFE, 2);
        wr8(s, RAM_REQUESTED_POKEMON_ACTION_TYPE, 0xFE);
        return;
    case 0x2A:
    case 0x2C:
        readReliableDataFromEeprom(s, 0x83, 0x183, PAYLOAD, 0x28);
        sendPacket(s, 0x28, 0x2A, 2);
        wr8(s, RAM_REQUESTED_POKEMON_ACTION_TYPE, cmd);
        return;
    default: return;
    }
}

void irCommsEventLoop(pw_state_t *s) {
    uint8_t count = rd8(s, RAM_NumBytesRxedsoFar);
    int byte = s->hal && s->hal->ir_recv ? s->hal->ir_recv(s->hal->user) : -1;
    if (byte >= 0) {
        if (count >= RX_MAX) {
            wr8(s, RAM_LastRxedUnusedByte, (uint8_t)byte);
            fail(s, 8);
            return;
        }
        wr8(s, RAM_NumBytesRxedsoFar, count + 1);
        wr8(s, PKT + count, (uint8_t)byte ^ 0xAA);
        wr16(s, RAM_TimeWhenLAstByteRxed, hw_ir_tcnt(s));
        return;
    }
    uint16_t since = (uint16_t)(hw_ir_tcnt(s) - rd16(s, RAM_TimeWhenLAstByteRxed));
    if (since <= IR_GAP) {
        return;
    }
    if (since > IR_TIMEOUT) {
        wr8(s, RAM_numTimesWeSentIrAdv, rd8(s, RAM_numTimesWeSentIrAdv) + 1);
        if (rd8(s, RAM_ourPeerRole) >= 3 || rd8(s, RAM_numTimesWeSentIrAdv) >= IR_ADVERTS) {
            fail(s, (rd8(s, RAM_bool_got_at_least_one_packet) & 1) ? 2 : 1);
            return;
        }
        advertise(s);
        lcdShowBuffer(s, hw_ir_tcnt(s) >> 14 & 1);
        wr16(s, RAM_TimeWhenLAstByteRxed, hw_ir_tcnt(s));
        return;
    }
    if (!count) {
        return;
    }
    wr8(s, RAM_bool_got_at_least_one_packet, rd8(s, RAM_bool_got_at_least_one_packet) | 1);
    if (count == 1) {
        wr8(s, RAM_NumBytesRxedsoFar, 0);
        if (rd8(s, PKT) == 0xFC && rd8(s, RAM_ourPeerRole) == 1) {
            wr8(s, RAM_ourPeerRole, 2);
            sendPacket(s, 0, 0xFA, 2);
        }
        return;
    }
    uint16_t crc = (uint16_t)(rd8(s, PKT + 3) << 8 | rd8(s, PKT + 2));
    wr8(s, PKT + 2, 0);
    wr8(s, PKT + 3, 0);
    if (calcPacketCrc(s, PKT, count) != crc) {
        wr8(s, RAM_NumBytesRxedsoFar, 0);
        wr8(s, RAM_NumPingsSent, rd8(s, RAM_NumPingsSent) + 1);
        if (rd8(s, RAM_NumPingsSent) >= 0x14) {
            fail(s, 2);
        }
        return;
    }
    handle_packet(s, rd8(s, RAM_gCurSubstateY) & 1, rd16(s, RAM_PeerPlayEepromXferSizeLeft));
    lcdShowBuffer(s, hw_ir_tcnt(s) >> 14 & 1);
    wr8(s, RAM_NumBytesRxedsoFar, 0);
    wr16(s, RAM_TimeWhenLAstByteRxed, hw_ir_tcnt(s));
}
