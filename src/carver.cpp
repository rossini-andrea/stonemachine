#include <iostream>
#include <fstream>
#include <filesystem>
#include <functional>
#include <cstdlib>
#include <stdint.h>
#include <map>
#include <vector>
#include <variant>
#include <type_traits>
#include <optional>
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

struct scroll_label {
    std::string name;
};

using expression_variant = std::variant<uint32_t, scroll_label>;

struct expression {
    expression_variant val;
};

using params_variant = std::variant<register_name, expression>;
using params_vector = std::vector<params_variant>;

struct scroll_statement {
    std::string operator_name;
    params_vector params;
    uint32_t line;
    std::optional<uint32_t> address;
};

BOOST_FUSION_ADAPT_STRUCT(register_name,
    (char,   val)
)

BOOST_FUSION_ADAPT_STRUCT(expression,
    (expression_variant,   val)
)

BOOST_FUSION_ADAPT_STRUCT(scroll_statement,
    (std::string,   operator_name)
    (params_vector, params)
)

BOOST_FUSION_ADAPT_STRUCT(scroll_label,
    (std::string, name)
)

using grammar_output_t = std::optional<std::variant<
            scroll_statement,
            scroll_label
        >>;

/**
 * @brief The Boost::Spirit grammar to parse th Universal machine assembly.
 */
struct scroll_grammar : qi::grammar<str_iterator, grammar_output_t(), skipper> {

    scroll_grammar() : scroll_grammar::base_type(root) {
        register_name = qi::char_('A', 'H');
        label = qi::lexeme[qi::char_("_a-zA-Z") >> +qi::char_("_a-zA-Z0-9")] >> qi::lit(':');
        expression = qi::uint_ | ('\'' >> qi::char_ >> '\'') | label;
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
        statement = operator_name >> -(parameter % ',');
        root = -(statement | label) >> (qi::eoi | qi::eol);
    }

private:
    qi::rule<str_iterator, grammar_output_t(), skipper> root;
    qi::rule<str_iterator, scroll_statement(), skipper> statement;
    qi::rule<str_iterator, scroll_label()>              label;
    qi::rule<str_iterator, std::string()>               operator_name;
    qi::rule<str_iterator, params_variant()>            parameter;
    qi::rule<str_iterator, register_name()>             register_name;
    qi::rule<str_iterator, expression()>                expression;
};

/**
 * @brief Parsed program with symbol references.
 */
struct object_file {
    std::vector<scroll_statement>   instructions;
    std::map<std::string, uint32_t> labelmap;
};

/**
 * @brief Offers method for carving statements to stone.
 */
struct stone_compiler {
    using operator_compiler = std::function<bool(const scroll_statement &, uint32_t &)>;

    stone_compiler(object_file &input_object) : object(input_object), compilers {
        {"CondMove",    [this](auto &a, auto &b){return stone_compiler::ennary_op<3>(0, a, b);}},
        {"Index",       [this](auto &a, auto &b){return stone_compiler::ennary_op<3>(1, a, b);}},
        {"Amend",       [this](auto &a, auto &b){return stone_compiler::ennary_op<3>(2, a, b);}},
        {"Add",         [this](auto &a, auto &b){return stone_compiler::ennary_op<3>(3, a, b);}},
        {"Mult",        [this](auto &a, auto &b){return stone_compiler::ennary_op<3>(4, a, b);}},
        {"Div",         [this](auto &a, auto &b){return stone_compiler::ennary_op<3>(5, a, b);}},
        {"Nand",        [this](auto &a, auto &b){return stone_compiler::ennary_op<3>(6, a, b);}},
        {"Halt",        [this](auto &a, auto &b){return stone_compiler::void_op(7, a, b);}},
        {"Alloc",       [this](auto &a, auto &b){return stone_compiler::ennary_op<2>(8, a, b);}},
        {"Abandon",     [this](auto &a, auto &b){return stone_compiler::ennary_op<1>(9, a, b);}},
        {"Output",      [this](auto &a, auto &b){return stone_compiler::ennary_op<1>(10, a, b);}},
        {"Input",       [this](auto &a, auto &b){return stone_compiler::ennary_op<1>(11, a, b);}},
        {"Load",        [this](auto &a, auto &b){return stone_compiler::ennary_op<2>(12, a, b);}},
        {"Orthography", [this](auto &a, auto &b){return stone_compiler::ortho_op(13, a, b);}},
        {"Data",        [this](auto &a, auto &b){return stone_compiler::data(a, b);}}
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

    /**
     * @brief Compiles the program.
     */
    bool compile(std::ostream &file) const {
        for (auto &statement: object.instructions)
        {
            uint32_t opcode;

            if (!compile(statement, opcode)) {
                return false;
            }

            uint8_t bytes[4] = {
                static_cast<uint8_t>((opcode >> 24) & 0xff),
                static_cast<uint8_t>((opcode >> 16) & 0xff),
                static_cast<uint8_t>((opcode >> 8) & 0xff),
                static_cast<uint8_t>((opcode) & 0xff)
            };
            file.write(reinterpret_cast<char*>(bytes), sizeof(bytes));
        }

        return true;
    }
private:
    object_file &object;
    std::map<std::string, operator_compiler> compilers;

    bool resolve_expression(const expression &expr, uint32_t &value) {
        return std::visit([&value, this](auto &&expr) -> bool {
            using T = std::decay_t<decltype(expr)>;
            if constexpr (std::is_same_v<T, uint32_t>) {
                value = expr;
            } else if constexpr (std::is_same_v<T, scroll_label>) {
                value = 0;
                auto it = object.labelmap.find(expr.name);

                if (it == object.labelmap.end()) {
                    return false;
                }

                value = object.instructions[it->second].address.value();
            }

            return true;
        }, expr.val);
    }

    /**
     * @brief Compiles the Data statement.
     * @remarks The method carves on the output platter the exact content of the
     *          one and only expression present in the statement.
     * @param statement Reference to input statement.
     * @param opcode Reference to output platter.
     */
    bool data(const scroll_statement &statement, uint32_t &opcode) {
        if (statement.params.size() != 1) {
            return false;
        }

        try {
            const expression &expr = std::get<expression>(statement.params[0]);

            if (!resolve_expression(expr, opcode)) {
                return false;
            }
        }
        catch (std::bad_variant_access&) {
            return false;
        }

        return true;
    }

    /**
     * @brief Compiles a void statement.
     * @remarks The method carves on the output platter only the opcode of the
     *          statement.
     * @param op Numeric opcode.
     * @param statement Reference to input statement.
     * @param opcode Reference to output platter.
     */
    bool void_op(uint32_t op, const scroll_statement &statement, uint32_t &opcode) {
        if (statement.params.size() > 0) {
            return false;
        }

        opcode = (op << 28);
        return true;
    }

    /**
     * @brief Compiles an N-ary statement.
     * @remarks The method carves on the output platter the opcode of the
     *          statement, followed by the N registers.
     * @tparam N Numer of expected parameters in the statement.
     * @param op Numeric opcode.
     * @param statement Reference to input statement.
     * @param opcode Reference to output platter.
     */
    template<size_t N>
    bool ennary_op(uint32_t op, const scroll_statement &statement, uint32_t &opcode) {
        if (statement.params.size() != N) {
            return false;
        }

        uint32_t registers = 0;

        for (auto &p: statement.params) {
            try {
                const register_name &r = std::get<register_name>(p);
                registers = (registers << 3) | r.get();
            }
            catch (std::bad_variant_access&) {
                return false;
            }
        }

        opcode = (op << 28) | registers;
        return true;
    }

    /**
     * @brief Compiles the Orthography statement.
     * @remarks The method carves on the output platter the opcode of the
     *          statement, followed by the destination register and the data
     *          expression.
     * @param op Numeric opcode.
     * @param statement Reference to input statement.
     * @param opcode Reference to output platter.
     */
    bool ortho_op(uint32_t op, const scroll_statement &statement, uint32_t &opcode) {
        if (statement.params.size() != 2) {
            return false;
        }

        try {
            const register_name &r = std::get<register_name>(statement.params[0]);
            const expression &expr = std::get<expression>(statement.params[1]);
            uint32_t val;

            if (!resolve_expression(expr, val)) {
                return false;
            }

            if (val & 0xe0000000) {
                return false;
            }

            opcode = (op << 28) | (r.get() << 25) | val;
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

    uint32_t line_num = 0;
    uint32_t address = 0;
    object_file object;

    while (!scroll.eof()) {
        ++line_num;
        std::string line;
        std::getline(scroll, line);
        auto iter = line.begin();
        scroll_grammar g;
        grammar_output_t parse_result;
        bool r = qi::phrase_parse(iter, line.end(), g,
                                    qi::ascii::space, parse_result);

        if (!r) {
            std::cerr << "Error on line " << line_num << ".\n";
            return -1;
        }

        if (!parse_result) {
            continue;
        }

        std::visit([&object, line_num, &address](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, scroll_statement>) {
                arg.line = line_num;
                arg.address = address;
                ++address;
                object.instructions.push_back(arg);
            } else if constexpr (std::is_same_v<T, scroll_label>) {
                object.labelmap[arg.name] = object.instructions.size();
            }
        }, *parse_result);
    }

    scroll.close();

    stone_compiler compiler(object);
    std::fstream stone(argv[2], std::ios_base::out | std::ios_base::binary);

    if (!compiler.compile(stone)) {
        //std::cerr << "Error on line " << line_num << ".\n";
        std::cerr << "Error - Sorry for the missing line, I will restore soon.\n";
        stone.close();
        std::filesystem::remove(argv[2]);
        return -1;
    }
}
