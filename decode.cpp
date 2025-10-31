#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

enum Displacement { NONE, BYTE8BIT, BYTE16BIT };

static const std::vector<std::string> rm00 = {
    "bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"};

static const std::vector<std::string> regNames = {
    "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh",
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};

uint8_t extractBits(uint8_t byte, int start, int length) {
  return (byte >> start) & ((1 << length) - 1);
}

std::string displacementToString(Displacement dispType, uint8_t disp8,
                                 uint8_t disp16) {
  if (dispType == NONE)
    return "";

  std::ostringstream oss;
  if (dispType == BYTE8BIT) {
    int disp8Int = static_cast<int>(disp8);
    if (disp8Int > 0)
      oss << " + " << disp8Int;
  } else if (dispType == BYTE16BIT) {
    uint16_t disp16Int = (static_cast<uint16_t>(disp16) << 8) | disp8;
    if (disp16Int > 0)
      oss << " + " << disp16Int;
  }
  return oss.str();
}

std::string formatMemoryOperand(uint8_t mod, uint8_t rm, Displacement dispType,
                                uint8_t disp8, uint8_t disp16) {
  std::ostringstream oss;
  if (mod == 0b00 && rm == 0b110) {
    uint16_t addr = (static_cast<uint16_t>(disp16) << 8) | disp8;
    oss << "[" << addr << "]";
  } else {
    oss << "[" << rm00[rm] << displacementToString(dispType, disp8, disp16)
        << "]";
  }
  return oss.str();
}

struct RMTFRInstruction {
  uint8_t d, w, mod, reg, rm;
  Displacement disp = NONE;
  uint8_t disp8 = 0, disp16 = 0;

  explicit RMTFRInstruction(uint8_t byte1, uint8_t byte2) {
    d = extractBits(byte1, 1, 1);
    w = extractBits(byte1, 0, 1);
    mod = extractBits(byte2, 6, 2);
    reg = extractBits(byte2, 3, 3);
    rm = extractBits(byte2, 0, 3);
  }

  int displacement8(uint8_t byte) {
    disp = BYTE8BIT;
    disp8 = byte;
    return 1;
  }

  int displacement16(uint8_t byteLo, uint8_t byteHi) {
    disp = BYTE16BIT;
    disp8 = byteLo;
    disp16 = byteHi;
    return 2;
  }

  void print() const {
    size_t regIdx = reg + (w ? 8 : 0);
    size_t rmIdx = rm + (w ? 8 : 0);

    std::cout << "mov ";
    if (mod == 0b11) {
      std::cout << (d ? regNames[regIdx] : regNames[rmIdx]) << ", "
                << (d ? regNames[rmIdx] : regNames[regIdx]);
    } else {
      std::string mem = formatMemoryOperand(mod, rm, disp, disp8, disp16);
      if (d == 0)
        std::cout << mem << ", " << regNames[regIdx];
      else
        std::cout << regNames[regIdx] << ", " << mem;
    }
    std::cout << "\n";
  }
};

struct ITRInstruction {
  uint8_t w, reg;
  Displacement dispType = NONE;
  uint8_t disp8 = 0, disp16 = 0;

  explicit ITRInstruction(uint8_t byte) {
    w = extractBits(byte, 3, 1);
    reg = extractBits(byte, 0, 3);
    dispType = w ? BYTE16BIT : BYTE8BIT;
  }

  void print() const {
    size_t regIdx = reg + (w ? 8 : 0);
    uint16_t imm = (static_cast<uint16_t>(disp16) << 8) | disp8;
    std::cout << "mov " << regNames[regIdx] << ", " << imm << "\n";
  }
};

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <filename>\n";
    return 1;
  }

  std::ifstream file(argv[1], std::ios::binary);
  if (!file) {
    std::cerr << "Error: Could not open " << argv[1] << "\n";
    return 1;
  }

  std::vector<uint8_t> buf((std::istreambuf_iterator<char>(file)), {});
  if (buf.empty()) {
    std::cerr << "Error: File is empty.\n";
    return 1;
  }

  size_t i = 0;
  while (i < buf.size()) {
    uint8_t opcode = buf[i];

    if (extractBits(opcode, 2, 6) == 0b100010) {
      auto inst = RMTFRInstruction(opcode, buf[i + 1]);
      size_t size = 2;

      switch (inst.mod) {
      case 0b00:
        if (inst.rm == 0b110) {
          size += inst.displacement16(buf[i + 2], buf[i + 3]);
        }
        break;
      case 0b01:
        size += inst.displacement8(buf[i + 2]);
        break;
      case 0b10:
        size += inst.displacement16(buf[i + 2], buf[i + 3]);
        break;
      }
      inst.print();
      i += size;
    }

    else if (extractBits(opcode, 4, 4) == 0b1011) {
      auto inst = ITRInstruction(opcode);
      size_t immSize = inst.w ? 2 : 1;

      inst.disp8 = buf[i + 1];
      if (inst.w)
        inst.disp16 = buf[i + 2];

      inst.print();
      i += 1 + immSize;
    }

    else {
      std::cerr << "Unknown opcode at byte " << i << ": " << std::hex
                << static_cast<int>(opcode) << "\n";
      break;
    }
  }

  return 0;
}
