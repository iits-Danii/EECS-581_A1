#include <iostream>
#include <string>
#include <cctype>

// ---------------------------------------------------------------------------
// validateToken
//
// A "token" is a maximal run of characters drawn only from {digit, '.', ':'}.
// This function checks whether such a token matches the FULL grammar below —
// the entire token must be consumed, with nothing left over and nothing
// missing. There is no attempt to trim characters off either end to find a
// valid address buried inside a longer token; if the token as a whole
// doesn't match, it is rejected outright.
//
//   address := octet '.' octet '.' octet '.' octet [ ':' port ]
//   octet   := 1-3 digits, numeric value 0-255, no leading zero unless the
//              octet is exactly the single digit "0"
//   port    := 1-5 digits, numeric value 0-65535, same leading-zero rule
//
// If a ':' is present, the port must be fully valid or the whole token
// (address included) is rejected.
// ---------------------------------------------------------------------------
static bool validateToken(const std::string& tok, unsigned long& outAddress, int& outPort) {
    const size_t len = tok.size();
    size_t pos = 0;
    unsigned long octets[4] = {0, 0, 0, 0};

    for (int i = 0; i < 4; ++i) {
        size_t start = pos;
        while (pos < len && std::isdigit(static_cast<unsigned char>(tok[pos]))) {
            ++pos;
        }
        size_t digitCount = pos - start;

        if (digitCount == 0 || digitCount > 3) {
            return false; // empty octet, or too many digits to ever be 0-255
        }
        if (tok[start] == '0' && digitCount > 1) {
            return false; // leading zero not allowed unless octet is just "0"
        }

        unsigned long value = 0;
        for (size_t k = start; k < pos; ++k) {
            value = value * 10 + static_cast<unsigned long>(tok[k] - '0');
        }
        if (value > 255) {
            return false; // out of range
        }
        octets[i] = value;

        if (i < 3) {
            // The first three octets must each be followed by a '.'
            if (pos >= len || tok[pos] != '.') {
                return false;
            }
            ++pos; // consume '.'
        }
    }

    int port = -1;
    if (pos < len) {
        // Anything left over after the fourth octet must be a port
        // introduced by ':'. Anything else (a 5th octet, a stray '.', etc.)
        // is invalid.
        if (tok[pos] != ':') {
            return false;
        }
        ++pos; // consume ':'

        size_t start = pos;
        while (pos < len && std::isdigit(static_cast<unsigned char>(tok[pos]))) {
            ++pos;
        }
        size_t digitCount = pos - start;

        if (digitCount == 0 || digitCount > 5) {
            return false; // missing port digits, or too many to ever fit 0-65535
        }
        if (tok[start] == '0' && digitCount > 1) {
            return false; // leading zero not allowed unless port is just "0"
        }

        unsigned long value = 0;
        for (size_t k = start; k < pos; ++k) {
            value = value * 10 + static_cast<unsigned long>(tok[k] - '0');
        }
        if (value > 65535) {
            return false; // out of range
        }
        port = static_cast<int>(value);

        if (pos != len) {
            // Trailing characters after the port (e.g. a second ':') mean
            // the token doesn't match the grammar in full.
            return false;
        }
    }

    outAddress = (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];
    outPort = port;
    return true;
}

// A character that may legally appear inside an address/port token.
static inline bool isTokenChar(char c) {
    return std::isdigit(static_cast<unsigned char>(c)) || c == '.' || c == ':';
}

// Returns true if a valid address was found, false otherwise.
// On success: outAddress holds the 32-bit value,
// and outPort holds the port number, or -1 if no port was present.
// On failure: outAddress is set to 0 and outPort is set to -1.
bool extractIPv4(const std::string& str, unsigned long& outAddress, int& outPort) {
    const size_t n = str.size();
    size_t i = 0;

    while (i < n) {
        if (isTokenChar(str[i])) {
            // Gather the maximal run of digit/'.'/':' characters.
            size_t start = i;
            while (i < n && isTokenChar(str[i])) {
                ++i;
            }
            std::string token = str.substr(start, i - start);

            unsigned long addr = 0;
            int port = -1;
            if (validateToken(token, addr, port)) {
                outAddress = addr;
                outPort = port;
                return true;
            }
            // This run failed validation as a whole; keep scanning the rest
            // of the line for another candidate run.
        } else {
            ++i; // garbage character, skip it
        }
    }

    outAddress = 0;
    outPort = -1;
    return false;
}

int main() {
    std::string line;

    while (true) {
        std::cout << "Enter a string (or 'END' to quit): ";
        if (!std::getline(std::cin, line)) {
            break; // no more input
        }
        if (line == "END") {
            std::cout << "Program terminated." << std::endl;
            break;
        }

        unsigned long address = 0;
        int port = -1;

        if (extractIPv4(line, address, port)) {
            unsigned long a = (address >> 24) & 0xFFUL;
            unsigned long b = (address >> 16) & 0xFFUL;
            unsigned long c = (address >> 8) & 0xFFUL;
            unsigned long d = address & 0xFFUL;

            std::cout << "Extracted IPv4 address: " << a << "." << b << "." << c << "." << d
                      << " (decimal value: " << address << ", port: ";
            if (port == -1) {
                std::cout << "none";
            } else {
                std::cout << port;
            }
            std::cout << ")" << std::endl;
        } else {
            std::cout << "Invalid input: no valid IPv4 address found" << std::endl;
        }
    }

    return 0;
}