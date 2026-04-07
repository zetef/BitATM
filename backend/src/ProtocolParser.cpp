#include "ProtocolParser.h"

#include <sstream>

#include "../../common/AppException.h"

std::string ProtocolParser::serialize(const Packet& packet) const {
    std::ostringstream os;
    os << static_cast<int>(packet.type) << "|" << packet.version << "|" << packet.from << "|"
       << packet.to << "|" << packet.body << "|" << packet.key << "|" << packet.timestamp << "|"
       << packet.errorMsg;
    return os.str();
}

Packet ProtocolParser::deserialize(const std::string& data) const {
    if (data.empty()) throw ProtocolException("ProtocolParser: empty input");

    std::istringstream ss(data);
    Packet p;

    try {
        ss >> p;
    } catch (const std::exception& e) {
        throw ProtocolException(std::string("ProtocolParser: malformed packet - ") + e.what());
    }

    if (ss.fail()) throw ProtocolException("ProtocolParser: incomplete packet fields");

    return p;
}
