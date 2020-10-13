#include <stdint.h>
#include <array>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>

namespace stonemachine {
typedef uint32_t platter;

/**
 * @brief Memory of the Universal Machine.
 */
class um_ram {
    std::vector<std::unique_ptr<std::vector<platter>>> m_storage;
public:
    um_ram(std::vector<platter> &array_zero): m_storage(1) {
        m_storage[0] = std::make_unique<std::vector<platter>>(array_zero);
    }

    platter read(platter array, platter offset) {
        return m_storage[array]->at(offset);
    }

    void write(platter array, platter offset, platter value) {
        m_storage[array]->at(offset) = value;
    }

    platter allocate(platter size) {
        for (platter i = 0; i < m_storage.size(); ++i) {
            if (m_storage[i] == nullptr) {
                m_storage[i] = std::make_unique<std::vector<platter>>(size);
                return i;
            }
        }

        m_storage.push_back(std::make_unique<std::vector<platter>>(size));
        return m_storage.size() - 1;
    }

    void deallocate(platter array) {
        if (array == 0) {
            throw std::runtime_error("The machine failed");
        }

        if (m_storage[array] == nullptr) {
            throw std::runtime_error("The machine failed");
        }

        m_storage[array] = nullptr;
    }

    void switch_program(platter array) {
        if (array == 0) {
            return;
        }

        if (m_storage[array] == nullptr) {
            throw std::runtime_error("The machine failed");
        }

        m_storage[0] = std::make_unique<std::vector<platter>>(*m_storage[array]);
    }
};

/**
 * @brief I/O interface of the Universal Machine.
 */
class um_io {
public:

    /**
     * @brief Writes data to the device.
     * @param value ASCII code to write.
     */
    void write(platter value) {
        if ((value & 0xff) != value) throw std::runtime_error("The machine failed");
        std::cout << static_cast<char>(value & 0xff);
    }

    /**
     * @brief Read next data ready on the device.
     * @return ASCII code of next character.
     */
    platter read() {
        char c;
        std::cin.get(c);
        return std::cin.eof() ? 0xffffffff : c;
    }
};

/**
 * @brief CPU of the Universal Machine.
 */
class um_cpu {
    std::array<platter, 8> m_registers;
    platter m_execution_finger;
    bool m_halt;

public:

    std::shared_ptr<um_ram> m_ram;
    std::shared_ptr<um_io> m_io;

    um_cpu() :
        m_halt(false),
        m_execution_finger(0),
        m_registers{0, 0, 0, 0, 0, 0, 0, 0} { }

    /**
     * @brief Executes the next instruction.
     */
    void clock_cycle() {
        if (m_halt) return;

        auto opcode = m_ram->read(0, m_execution_finger);
        ++m_execution_finger;
        auto oprator = opcode >> 28;

        if (oprator == 0x0d) {
            auto a = (opcode >> 25) & (1 | 2 | 4);
            m_registers[a] = opcode & ~(
                (1 << 31) | (1 << 30) |
                (1 << 29) | (1 << 28) |
                (1 << 27) | (1 << 26) | (1 << 25)
            );
        } else {
            auto &a = m_registers[(opcode >> 6) & (1 | 2 | 4)];
            auto &b = m_registers[(opcode >> 3) & (1 | 2 | 4)];
            auto &c = m_registers[opcode & (1 | 2 | 4)];

            switch (oprator) {
                case 0x00:
                    if (c) a = b;
                    break;
                case 0x01:
                    a = m_ram->read(b, c);
                    break;
                case 0x02:
                    m_ram->write(a, b, c);
                    break;
                case 0x03:
                    a = b + c;
                    break;
                case 0x04:
                    a = b * c;
                    break;
                case 0x05:
                    a = b / c;
                    break;
                case 0x06:
                    a = ~(b & c);
                    break;
                case 0x07:
                    m_halt = true;
                    break;
                break;
                case 0x08:
                    b = m_ram->allocate(c);
                break;
                case 0x09:
                    m_ram->deallocate(c);
                break;
                case 0x0a:
                    m_io->write(c);
                    break;
                case 0x0b:
                    c = m_io->read();
                    break;
                case 0x0c:
                    m_ram->switch_program(b);
                    m_execution_finger = c;
                    break;
                case 0x0e:
                case 0x0f:
                    throw std::runtime_error("The machine failed");
            }
        }
    }
};

}

stonemachine::platter read_platter(std::istream &stream);

/**
 * @brief Application entry point.
 * @param argc Command line arguments count.
 * @param argv Command line arguments.
 */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cout << "You need a carved stone.\n";
        return -1;
    }

    const std::ios_base::openmode mode = std::ios_base::in | std::ios_base::binary;
    std::fstream scroll(argv[1], mode);
    std::vector<stonemachine::platter> array_zero;

    while (1) {
        auto platter = read_platter(scroll);

        if (scroll.eof()) break;

        array_zero.push_back(platter);
    }

    scroll.close();

    stonemachine::um_ram ram(array_zero);
    stonemachine::um_io io;
    stonemachine::um_cpu cpu;
    cpu.m_ram.reset(&ram);
    cpu.m_io.reset(&io);

    while (1) {
        cpu.clock_cycle();
    }
}

/**
 * @brief Reads a platter from the scroll.
 * @param stream File stream to read.
 * @return The 32-bit platter found on the stream. Undefined if EOF.
 */
stonemachine::platter read_platter(std::istream &stream) {
    uint8_t bytes[4];
    std::streamsize read = 0;

    while (read != 4) {
        stream.read(reinterpret_cast<char*>(bytes + read), 4 - read);
        read += stream.gcount();

        if (stream.eof()) return 0;
    }

    return  (bytes[0] << 24) |
            (bytes[1] << 16) |
            (bytes[2] << 8) |
            (bytes[3]);
}
