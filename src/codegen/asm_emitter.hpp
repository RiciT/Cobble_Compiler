#pragma once

#include <string>
#include <sstream>

class AsmEmitter {
public:
    enum class Section {
        Main,
        Functions,
        Data,
    };

    //stream control
    void set_section(const Section section) { m_active_section = section; }

    //basic instructions
    void emit(const std::string_view mnemonic)
    {
        current_section() << "    " << mnemonic << "\n";
    }

    void emit(const std::string_view mnemonic, const std::string_view op1) {
        current_section() << "    " << mnemonic << " " << op1 << "\n";
    }

    void emit(const std::string_view mnemonic, const unsigned long& op1) {
        current_section() << "    " << mnemonic << " " << op1 << "\n";
    }

    void emit(const std::string_view mnemonic, const std::string_view op1, const std::string_view op2) {
        current_section() << "    " << mnemonic << " " << op1 << ", " << op2 << "\n";
    }

    void emit(const std::string_view mnemonic, const std::string_view op1, const unsigned long& op2) {
        current_section() << "    " << mnemonic << " " << op1 << ", " << op2 << "\n";
    }

    void emit_mov_offset(const std::string_view reg1, const std::string_view reg2, const int offset)
    {
        current_section() << "    mov [" << reg1 << " + " << offset << "], " << reg2 << "\n";
    }

    //structure instructions
    void emit_label(const std::string_view label) {
        current_section() << label << ":\n";
    }

    //comment
    void emit_comment(const std::string_view comment) {
        current_section() << "    ;; " << comment << "\n";
    }

    //final output
    [[nodiscard]] std::string build_output() const {
        // Replicating your original logic: Main first, then Functions
        return "global _start\n_start:\n" + m_main.str() +
               "\n\n; Functions\n" + m_functions.str();
    }

private:
    std::stringstream& current_section()
    {
        switch (m_active_section)
        {
            case Section::Functions:
                return m_functions;
            default:
            case Section::Main:
                return m_main;
        }
    }

    Section m_active_section = Section::Main;
    std::stringstream m_main;
    std::stringstream m_functions;
};