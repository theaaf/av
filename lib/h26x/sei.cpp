#include "sei.hpp"

namespace h264 {

error sei_message::decode(bitstream* bs) {
    while (bs->next_bits(8) == 0xff) {
        bs->advance_bits(8);
        payloadType += 255;
    }
    u<8> last_payload_type_byte;
    auto err = last_payload_type_byte.decode(bs);
    if (err) {
        return err;
    }
    payloadType += last_payload_type_byte;

    while (bs->next_bits(8) == 0xff) {
        bs->advance_bits(8);
        payloadSize += 255;
    }
    u<8> last_payload_size_byte;
    err = last_payload_size_byte.decode(bs);
    if (err) {
        return err;
    }
    payloadSize += last_payload_size_byte;

    const void* data = nullptr;
    if (!bs->read_byte_aligned_bytes(&data, payloadSize)) {
        return {"unable to read sei_payload"};
    }

    sei_payload.assign(reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + payloadSize);

    return {};
}

error sei_rbsp::decode(bitstream* bs) {
    while (bs->more_rbsp_data()) {
        struct sei_message msg;
        auto err = msg.decode(bs);
        if (err) {
            return err;
        }
        sei_message.emplace_back(std::move(msg));
    }
    return {};
}

} // namespace h264
