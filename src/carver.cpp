#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <stdint.h>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_uint.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/include/support_line_pos_iterator.hpp>

using namespace std::literals::string_literals;

namespace spirit = boost::spirit;
namespace ascii = boost::spirit::ascii;
namespace qi = boost::spirit::qi;
namespace phoenix = boost::phoenix;

using str_iterator = std::string::iterator;
using skipper = qi::space_type;

struct scroll_statement {
    std::string operator_name;
    std::string expression;
    std::string r1;
    std::string r2;
    std::string r3;
};

BOOST_FUSION_ADAPT_STRUCT(scroll_statement,
    (std::string,   operator_name)
    (std::string,   expression)
    (std::string,   r1)
    (std::string,   r2)
    (std::string,   r3)
)

struct scroll_grammar : qi::grammar<str_iterator,
                                    scroll_statement()> {
    scroll_grammar() : scroll_grammar::base_type(root) {
        register_name = qi::char_('A', 'H');
        expression = spirit::uint_(std::numeric_limits<uint32_t>::max());
        ternary_operator = qi::lit("CondMove") | "Index" | "Amend" |
                            "Add" | "Mult" | "Div" | "Nand";
        binary_operator = qi::lit("Alloc") | "Abandon" | "Load";
        unary_operator = qi::lit("Input") | "Output";
        void_operator = qi::lit("Halt");
        special_operator = qi::lit("Ortography");
        data_operator = qi::lit("Data");

        root =
            (
                ternary_operator("operator_name"s) >>
                register_name("r1"s) >> ',' >>
                register_name("r2"s) >> ',' >>
                register_name("r3"s)
            ) |
            (
                binary_operator("operator_name"s) >>
                register_name("r2"s) >> ',' >>
                register_name("r3"s)
            ) |
            (
                unary_operator("operator_name"s) >>
                register_name("r3"s)
            ) |
            (
                void_operator("operator_name"s)
            ) |
            (
                special_operator("operator_name"s) >>
                register_name("r1"s) >> ',' >>
                expression("expression"s)
            ) |
            (
                data_operator("operator_name"s) >> expression("expression"s)
            );
    }

private:
    qi::rule<str_iterator, scroll_statement()>  root;
    qi::rule<str_iterator, void(std::string)>   ternary_operator,
                                                binary_operator,
                                                unary_operator,
                                                void_operator,
                                                special_operator,
                                                data_operator;
    qi::rule<str_iterator, void(std::string)>   register_name;
    qi::rule<str_iterator, void(std::string)>   expression;
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

    while (!scroll.eof()) {
        ++line_num;
        std::string line;
        scroll >> line;
        auto iter = line.begin();
        scroll_grammar g;
        scroll_statement parse_result;
        bool r = qi::parse(iter,
                        line.end(), g, parse_result);

        if (!r) {
            std::cerr << "Error on line " << line_num << ".\n";
            stone.close();
            std::filesystem::remove(argv[2]);
            return -1;
        }

        uint32_t opcode = 0;

        uint8_t bytes[4] = {
            static_cast<uint8_t>((opcode >> 24) & 0xff),
            static_cast<uint8_t>((opcode >> 16) & 0xff),
            static_cast<uint8_t>((opcode >> 8) & 0xff),
            static_cast<uint8_t>((opcode) & 0xff)
        };
        stone.write(reinterpret_cast<char*>(&bytes), sizeof(bytes));
    }
}
