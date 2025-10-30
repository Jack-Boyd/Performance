#include <bitset>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

// Map available instructions here
const std::unordered_map<int, std::string> instrCodes = {{0b100010, "mov"}};

const std::vector<std::string> reg8 = {"al", "cl", "dl", "bl",
                                       "ah", "ch", "dh", "bh"};
const std::vector<std::string> reg16 = {"ax", "cx", "dx", "bx",
                                        "sp", "bp", "si", "di"};

struct DecodedInstruction {
  std::string mnemonic;
  std::string operand1;
  std::string operand2;
};

uint8_t extractBits(unsigned char byte, int start, int length) {
  return (byte >> start) & ((1 << length) - 1);
}

DecodedInstruction decode(uint8_t byte1, uint8_t byte2) {
  DecodedInstruction result;
  // Hardcoded values for 'mov' instruction bytes
  uint8_t instruction = extractBits(byte1, 2, 6);
  uint8_t d = extractBits(byte1, 1, 1);
  uint8_t w = extractBits(byte1, 0, 1);

  uint8_t mod = extractBits(byte2, 6, 2);
  uint8_t reg = extractBits(byte2, 3, 3);
  uint8_t rm = extractBits(byte2, 0, 3);

  auto instructionCode = instrCodes.find(instruction);
  if (instructionCode == instrCodes.end()) {
    result.mnemonic = "db";
    return result;
  }
  result.mnemonic = instructionCode->second;

  std::string regName = w ? reg16[reg] : reg8[reg];
  std::string rmName = w ? reg16[rm] : reg8[rm];

  result.operand1 = d ? regName : rmName;
  result.operand2 = d ? rmName : regName;

  return result;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <filename>\n";
    return 1;
  }
  const std::string filename = argv[1];

  std::ifstream instructions(filename, std::ios::binary);
  if (!instructions) {
    std::cerr << "Error: Could not open " << filename << std::endl;
    return 1;
  }
  std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(instructions), {});
  if (buffer.empty()) {
    std::cerr << "Error: File is empty.\n";
    return 1;
  }

  instructions.close();

  // auto instr = decode(buffer[i], buffer[i + 1]);
  // std::cout << instr.mnemonic << " " << instr.operand1 << ", "
  //           << instr.operand2 << "\n";

  int byte = 0;
  while (byte < 20) {
    if (extractBits(buffer[byte], 2, 6) == 0b100010) {
      std::cout << "Register/Memory to/from register" << std::endl;
      std::cout << std::bitset<8>(buffer[byte]) << " ";
      std::cout << std::bitset<8>(buffer[byte + 1]) << std::endl;
      byte += 2;
    } else if (extractBits(buffer[byte], 4, 4) == 0b1011) {
      auto w = extractBits(buffer[byte], 3, 1);
      if (w == 0) {
        std::cout << "Immediate to register 8 bit" << std::endl;
        std::cout << std::bitset<8>(buffer[byte]) << " ";
        std::cout << std::bitset<8>(buffer[byte + 1]) << std::endl;
        byte += 2;
      } else {
        std::cout << "Immediate to register 16 bit" << std::endl;
        std::cout << std::bitset<8>(buffer[byte]) << " ";
        std::cout << std::bitset<8>(buffer[byte + 1]) << " ";
        std::cout << std::bitset<8>(buffer[byte + 2]) << std::endl;
        byte += 3;
      }
    }
  }
  return 0;
}
