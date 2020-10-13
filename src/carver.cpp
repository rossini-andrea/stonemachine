#include <iostream>
#include <fstream>
#include <filesystem>
#include <functional>
#include <cstdlib>
#include <stdint.h>
#include <map>
#include <vector>
#include <variant>
#include <functional>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_uint.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/include/qi_lexeme.hpp>
#include <boost/spirit/include/support_line_pos_iterator.hpp>

using namespace std::literals::string_literals;

namespace spirit = boost::spirit;
namespace ascii = boost::spirit::ascii;
namespace qi = boost::spirit::qi;

using str_iterator = std::string::iterator;
using skipper = ascii::space_type;

struct register_name {
    char val;
    uint32_t get() const { return val - 'A'; }
};

struct expression {
    uint32_t val;
};

using params_variant = std::variant<register_name, expression>;
using params_vector = std::vector<params_variant>;

struct scroll_statement {
    std::string operator_name;
    params_vector params;
};

BOOST_FUSION_ADAPT_STRUCT(register_name,
    (char,   val)
)

BOOST_FUSION_ADAPT_STRUCT(expression,
    (uint32_t,   val)
)

BOOST_FUSION_ADAPT_STRUCT(scroll_statement,
    (std::string,   operator_name)
    (params_vector, params)
)

struct scroll_grammar : qi::grammar<str_iterator,
                                    scroll_statement(),
                                    skipper> {

    scroll_grammar() : scroll_grammar::base_type(root) {
        register_name = qi::char_('A', 'H');
        expression = qi::uint_ | ('\'' >> qi::char_ >> '\'');
        parameter = expression | register_name;
        operator_name = qi::string("CondMove") |
                        qi::string("Index") |
                        qi::string("Amend") |
                        qi::string("Add") |
                        qi::string("Mult") |
                        qi::string("Div") |
                        qi::string("Nand") |
                        qi::string("Alloc") |
                        qi::string("Abandon") |
                        qi::string("Load") |
                        qi::string("Input") |
                        qi::string("Output") |
                        qi::string("Halt") |
                        qi::string("Orthography") |
                        qi::string("Data");
        root = operator_name >> -(parameter % ',') >> (qi::eoi | qi::eol);
    }

private:
    qi::rule<str_iterator, scroll_statement(), skipper> root;
    qi::rule<str_iterator, std::string()>               operator_name;
    qi::rule<str_iterator, params_variant()>            parameter;
    qi::rule<str_iterator, register_name()>             register_name;
    qi::rule<str_iterator, expression()>                expression;
};

/**
 * @brief Offers method for carving statements to stone.
 */
struct stone_compiler {
    using operator_compiler = std::function<bool(const scroll_statement &, uint32_t &)>;

    stone_compiler() : compilers {
        {"CondMove",    [](auto &a, auto &b){return stone_compiler::ennary_op<3>(0, a, b);}},
        {"Index",       [](auto &a, auto &b){return stone_compiler::ennary_op<3>(1, a, b);}},
        {"Amend",       [](auto &a, auto &b){return stone_compiler::ennary_op<3>(2, a, b);}},
        {"Add",         [](auto &a, auto &b){return stone_compiler::ennary_op<3>(3, a, b);}},
        {"Mult",        [](auto &a, auto &b){return stone_compiler::ennary_op<3>(4, a, b);}},
        {"Div",         [](auto &a, auto &b){return stone_compiler::ennary_op<3>(5, a, b);}},
        {"Nand",        [](auto &a, auto &b){return stone_compiler::ennary_op<3>(6, a, b);}},
        {"Halt",        [](auto &a, auto &b){return stone_compiler::void_op(7, a, b);}},
        {"Alloc",       [](auto &a, auto &b){return stone_compiler::ennary_op<2>(8, a, b);}},
        {"Abandon",     [](auto &a, auto &b){return stone_compiler::ennary_op<1>(9, a, b);}},
        {"Output",      [](auto &a, auto &b){return stone_compiler::ennary_op<1>(10, a, b);}},
        {"Input",       [](auto &a, auto &b){return stone_compiler::ennary_op<1>(11, a, b);}},
        {"Load",        [](auto &a, auto &b){return stone_compiler::ennary_op<2>(12, a, b);}},
        {"Orthography", [](auto &a, auto &b){return stone_compiler::ortho_op(13, a, b);}}
    }
    {

    }

    /**
     * @brief Compiles the parsed statement to a carved platter.
     * @param statement Reference to input statement.
     * @param opcode Reference to output platter.
     */
    bool compile(const scroll_statement &statement, uint32_t &opcode) const {
        return compilers.at(statement.operator_name)(statement, opcode);
    }
private:
    std::map<std::string, operator_compiler> compilers;

    static bool void_op(uint32_t op, const scroll_statement &statement, uint32_t &opcode) {
        if (statement.params.size() > 0) {
            return false;
        }

        opcode = (op << 28);
        return true;
    }

    template<size_t N>
    static bool ennary_op(uint32_t op, const scroll_statement &statement, uint32_t &opcode) {
        if (statement.params.size() != N) {
            return false;
        }

        uint32_t result = 0;

        for (auto &p: statement.params) {
            try {
                const register_name &r = std::get<register_name>(statement.params[0]);
                result = (result << 3) | r.get();
            }
            catch (std::bad_variant_access&) {
                return false;
            }

        }

        opcode = (op << 28) | result;
        return true;
    }

    static bool ortho_op(uint32_t op, const scroll_statement &statement, uint32_t &opcode) {
        if (statement.params.size() != 2) {
            return false;
        }

        try {
            const register_name &r = std::get<register_name>(statement.params[0]);
            const expression &expr = std::get<expression>(statement.params[1]);

            if (expr.val & 0xe0000000) {
                return false;
            }

            opcode = (op << 28) | (r.get() << 25) | expr.val;
        }
        catch (std::bad_variant_access&) {
            return false;
        }

        return true;
    }
};

/**
 * @brief Application entry point.
 * @param argc Command line arguments count.
 * @param argv Command line arguments.
 */
int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "You need a scroll and a stone.\n";
        return -1;
    }

    std::fstream scroll(argv[1], std::ios_base::in);
    std::fstream stone(argv[2], std::ios_base::out | std::ios_base::binary);

    int line_num = 0;
    stone_compiler compiler;

    while (!scroll.eof()) {
        ++line_num;
        std::string line;
        std::getline(scroll, line);
        auto iter = line.begin();
        scroll_grammar g;
        scroll_statement parse_result;
        bool r = qi::phrase_parse(iter, line.end(), g,
                                    qi::ascii::space, parse_result);

        if (!r) {
            std::cerr << "Error on line " << line_num << ".\n";
            stone.close();
            //std::filesystem::remove(argv[2]);
            return -1;
        }

        uint32_t opcode;

        if (!compiler.compile(parse_result, opcode)) {
            std::cerr << "Error on line " << line_num << ".\n";
            stone.close();
            std::filesystem::remove(argv[2]);
            return -1;
        }

        uint8_t bytes[4] = {
            static_cast<uint8_t>((opcode >> 24) & 0xff),
            static_cast<uint8_t>((opcode >> 16) & 0xff),
            static_cast<uint8_t>((opcode >> 8) & 0xff),
            static_cast<uint8_t>((opcode) & 0xff)
        };
        stone.write(reinterpret_cast<char*>(bytes), sizeof(bytes));
    }
}
