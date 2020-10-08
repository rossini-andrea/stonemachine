#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <stdint.h>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_uint.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/include/support_line_pos_iterator.hpp>

namespace spirit = boost::spirit;
namespace qi = boost::spirit::qi;
namespace phoenix = boost::phoenix;

class ast_value;
class ast_object;
class ast_array;

using ast_variant = boost::variant<
    boost::recursive_wrapper<ast_object>,
    boost::recursive_wrapper<ast_array>,
    std::string>;
using ast_attributemap_t = std::map<std::string, ast_value>;
using ast_value_t = std::vector<ast_value>;

struct ast_value {
   ast_variant value;
};

struct ast_object {
    ast_attributemap_t attributes;
};

struct ast_array {
    ast_value_t values;
};

BOOST_FUSION_ADAPT_STRUCT(ast_value, (ast_variant, value))
BOOST_FUSION_ADAPT_STRUCT(ast_object, (ast_attributemap_t, attributes ))
BOOST_FUSION_ADAPT_STRUCT(ast_array, (ast_value_t, values))

using ast_pair_t = std::pair<std::string, ast_value>;

template<typename IteratorT>
class scroll_grammar : public qi::grammar<IteratorT, ast_object()> {
public:
    scroll_grammar() : scroll_grammar::base_type(root, "Object") {
        register_name %= qi::char_('A', 'H');

        conditional_move %= "CondMove" >> register_name >> ',' >>
                                    register_name >> ',' >>
                                    register_name;
        index %= "Index" >> register_name >> ',' >>
                                    register_name >> ',' >>
                                    register_name;
        amend %= "Amend" >> register_name >> ',' >>
                                    register_name >> ',' >>
                                    register_name;
        add %= "Add" >> register_name >> ',' >>
                                    register_name >> ',' >>
                                    register_name;
        mult %= "Mult" >> register_name >> ',' >>
                                    register_name >> ',' >>
                                    register_name;
        div %= "Div" >> register_name >> ',' >>
                                    register_name >> ',' >>
                                    register_name;
        nand %= "Nand" >> register_name >> ',' >>
                                    register_name >> ',' >>
                                    register_name;
        halt %= "Halt";
        alloc %= "Alloc" >> register_name >> ',' >> register_name;
        abandon %= "Abandon" >> register_name;
        output %= "Output" >> register_name;
        input %= "Input" >> register_name;
        load %= "Load" >> register_name >> ',' >> register_name;
        expression %= spirit::uint_(std::numeric_limits<uint32_t>::max());
        orthography %= "Ortography" >> register_name >> ',' >> expression;
        data %= "Data" >> (expression >> *(',' >> expression));
        oprator %= conditional_move | index | amend | add | mult | div | nand |
                halt | alloc | abandon | output | input | load |
                orthography;
        root %= (oprator | data);

        root.name("root");
        register_name.name("register");
        conditional_move.name("cond_move");
        index.name("index");
        amend.name("amend");
        add.name("add");
        mult.name("mult");
        div.name("div");
        nand.name("nand");
        halt.name("halt");
        alloc.name("alloc");
        abandon.name("abandon");
        output.name("output");
        input.name("input");
        load.name("load");
        expression.name("expr");
        orthography.name("ortho");
        data.name("data");
        oprator.name("operator");
    }

    qi::rule<IteratorT> const& start() const { return root; }

    qi::rule<IteratorT, ast_object()>
        root, register_name, conditional_move, index, amend, add, mult, div,
        nand, halt, alloc, abandon, output, input, load, expression,
        orthography, data, oprator;
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
        scroll_grammar<std::string::iterator> g;
        ast_object parse_result;
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
