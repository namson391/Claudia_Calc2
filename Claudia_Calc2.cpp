#include "claudia_calc_example.h"
#include <sys/types.h>
#include <iostream>
#include "spdlog/spdlog.h"
#include <iomanip>

#include <variant>
#include <limits>
#include <optional>
#include <string>
#include <sstream>

using namespace std;

namespace claudia_calc {

/*
 * Calculator functions
 */

inline bool is_register(char const ch) { return tolower(ch) >= 'a' && ch <= 'd'; }

inline reg_name to_reg_name(char const ch) {
    assert(is_register(ch));
    return static_cast<reg_name>(tolower(ch) - 'a');
}

inline char to_char(reg_name rn) { return static_cast<char>(rn + 'a'); }

string format_register(const reg_value& value) {
    if (std::holds_alternative<float>(value)) {
        return fmt::format("{:.2f}", std::get<float>(value));
    } else {
        return std::get<std::string>(value);
    }
}

inline void print_line() { cout << std::string(MENU_WIDTH, '-') << endl; }
inline void print_title(string const title) { cout << fmt::format("{:^{}}", title, MENU_WIDTH) << endl; }

void print_registers(Calculator & calc) {
    cout << "\t\tA = " << format_register(calc.registers[A]) << "\t\t";
    cout << "B = " << format_register(calc.registers[B]) << "\t\t";
    cout << "C = " << format_register(calc.registers[C]) << "\t\t";
    cout << "D = " << format_register(calc.registers[D]) << endl;
}

void print_menu(Calculator & calc) {
    print_title("ClaudiaCalc");
    print_line();
    print_registers(calc);
    print_line();
    cout << "+\tAdd" << endl;
    cout << "-\tSubtract" << endl;
    cout << "*\tMultiply" << endl;
    cout << "/\tDivide" << endl;
    cout << "a-d\tEnter a number or string for A,B,C,D" << endl;
    cout << "1-4\tClear register A,B,C,D" << endl;
    cout << "m\tPrints the menu" << endl;
    cout << "p\tPrints the registers" << endl;
    cout << "q\tQuits the app" << endl;
    print_line();
}

/*
 * Main functions
 */
 
void handle_arithmetic(char op, Calculator & calc) {
    char lhs, rhs;
    cout << "Enter a lhs register: ";
    cin >> lhs;
    cout << "Enter a rhs register: ";
    cin >> rhs;
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    reg_name lhs_reg = to_reg_name(lhs);
    reg_name rhs_reg = to_reg_name(rhs);

    const auto& val1 = calc.registers[lhs_reg];
    const auto& val2 = calc.registers[rhs_reg];

    reg_value result;

    if (std::holds_alternative<float>(val1) && std::holds_alternative<float>(val2)) {
        float f1 = std::get<float>(val1);
        float f2 = std::get<float>(val2);

        switch (op) {
            case '+': result = f1 + f2; break;
            case '-': result = f1 - f2; break;
            case '*': result = f1 * f2; break;
            case '/':
                if (f2 == 0.0f) {
                    spdlog::error("{} / {} is invalid. Cannot divide by zero", lhs, rhs);
                    return;
                }
                result = f1 / f2;
                break;
        }
    }
    else if (op == '+' && std::holds_alternative<std::string>(val1) && std::holds_alternative<std::string>(val2)) {
        result = std::get<std::string>(val1) + std::get<std::string>(val2);
    }
    else if (op == '*' && std::holds_alternative<std::string>(val1) && std::holds_alternative<float>(val2)) {
        std::string str = std::get<std::string>(val1);
        int times = static_cast<int>(std::get<float>(val2));
        if (times < 0) {
            spdlog::error("Cannot multiply string by a negative number");
            return;
        }
        std::string repeated;
        for (int i = 0; i < times; ++i) repeated += str;
        result = repeated;
    }
    else if (op == '*' && std::holds_alternative<float>(val1) && std::holds_alternative<std::string>(val2)) {
        std::string str = std::get<std::string>(val2);
        int times = static_cast<int>(std::get<float>(val1));
        if (times < 0) {
            spdlog::error("Cannot multiply string by a negative number");
            return;
        }
        std::string repeated;
        for (int i = 0; i < times; ++i) repeated += str;
        result = repeated;
    }
    else if (op == '/' && std::holds_alternative<std::string>(val1) && std::holds_alternative<float>(val2)) {
        std::string str = std::get<std::string>(val1);
        int parts = static_cast<int>(std::get<float>(val2));
        if (parts <= 0) {
            spdlog::error("Cannot divide string by zero or negative integer");
            return;
        }
        int split_len = str.length() / parts;
        result = str.substr(0, split_len);
    }
    else {
        spdlog::error("Cannot perform '{}' on types {} and {}",
                      op,
                      std::holds_alternative<float>(val1) ? "number" : "string",
                      std::holds_alternative<float>(val2) ? "number" : "string");
        return;
    }

    calc.registers[A] = result;
    print_registers(calc);
}

void execute(const string& cmd, Calculator& calc, optional<reg_name>& active_register) {
    if (cmd.empty()) {
        spdlog::error("Empty command");
        return;
    }

    char cmd_ch = tolower(cmd[0]);

    switch (cmd_ch) {
        case 'a': case 'b': case 'c': case 'd':
            active_register = to_reg_name(cmd_ch);
            cout << "Enter value for register " << cmd_ch << ": ";
            break;
        case '+': case '-': case '*': case '/':
            handle_arithmetic(cmd_ch, calc);
            break;
        case '1': case '2': case '3': case '4': {
            reg_name reg = static_cast<reg_name>(cmd_ch - '1');
            cout << "Clear register " << to_char(reg) << " to [0] number or [s] empty string? ";
            char mode;
            cin >> mode;
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (mode == 's' || mode == 'S') {
                calc.registers[reg] = std::string("");
            } else {
                calc.registers[reg] = 0.0f;
            }
            print_registers(calc);
            break;
        }
        case 'm':
            print_menu(calc);
            break;
        case 'p':
            print_registers(calc);
            break;
        case 'q':
            break;
        default:
            spdlog::error("{} is an unknown command", cmd);
            break;
    }
}

void start() {
    string cmd;
    Calculator calc;
    optional<reg_name> active_register;

    for (int i = 0; i < NUM_REGISTERS; ++i) {
        calc.registers[i] = 0.0f;
    }

    print_menu(calc);

    while (true) {
        cout << "Enter a command or value: ";
        if (cin.peek() == '\n') cin.ignore(); 
        getline(cin, cmd);

        if (cmd == "q") break;

        if (active_register.has_value()) {
            try {
                float num = stof(cmd);
                calc.registers[active_register.value()] = num;
            } catch (...) {
                calc.registers[active_register.value()] = std::string(cmd);
            }
            print_registers(calc);
            active_register.reset();
        } else {
            execute(cmd, calc, active_register);
        }
    }
}

}  // namespace claudia_calc

using namespace claudia_calc;

int main() {
    // Disable info/debug logs
    spdlog::set_level(spdlog::level::warn); 
    start();
    return 0;
