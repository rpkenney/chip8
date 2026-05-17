#pragma once

#include <string>
#include <map>
#include <set>
#include <vector>
#include <cstdint>

// Manages allocation of CHIP-8 registers (V0–VE) to variables and temporaries
class RegisterAllocator {
public:
    RegisterAllocator();

    // Allocate a register for a user variable
    // Returns the register number (0–14), or throws if no registers available
    uint8_t allocateUserVariable(const std::string& var_name);

    // Allocate a register for a temporary value
    // Returns the register number, or throws if no temp registers available
    uint8_t allocateTemporary();

    // Release a temporary register back to the pool
    void freeTemporary(uint8_t reg);

    // Query methods for variable locations
    bool isUserVariable(const std::string& var_name) const;
    bool isUserVariableInRegister(const std::string& var_name) const;
    bool isUserVariableSpilled(const std::string& var_name) const;
    uint8_t getUserVariableRegister(const std::string& var_name) const;
    uint16_t getUserVariableSpillAddress(const std::string& var_name) const;
    
    // Check if a register holds an active temporary
    bool isTemporaryInRegister(uint8_t reg) const;
    
    // Convenience methods (shorter names for codegen)
    bool isInRegister(const std::string& var_name) const {
        return isUserVariableInRegister(var_name);
    }
    bool isSpilled(const std::string& var_name) const {
        return isUserVariableSpilled(var_name);
    }
    uint8_t getRegister(const std::string& var_name) const {
        return getUserVariableRegister(var_name);
    }
    uint16_t getSpillAddress(const std::string& var_name) const {
        return getUserVariableSpillAddress(var_name);
    }
    
    int getNumUserVariables() const;
    int getNumAvailableTemps() const;

    // Spill management (for overflowed variables/temps)
    uint16_t allocateSpill();
    void freeSpill(uint16_t address);

    // Reset state
    void reset();

private:
    static constexpr uint8_t TOTAL_REGISTERS = 15;      // V0–VE (VF reserved)
    static constexpr uint16_t SPILL_BASE = 0xD00;       // Spill area start
    static constexpr uint16_t SPILL_MAX = 0xE00;        // Spill area end

    std::map<std::string, uint8_t> user_variables_reg_;  // var_name → register (if in reg)
    std::map<std::string, uint16_t> user_variables_spill_;  // var_name → spill address (if spilled)
    std::set<uint8_t> available_temp_registers_;         // unused registers
    std::map<uint8_t, uint16_t> active_temps_;           // reg → spill_addr (if spilled, addr is set; otherwise 0)
    int next_temp_id_;                                   // unique ID for each temp
    std::map<int, uint16_t> spilled_temps_;              // temp_id → spill_addr
    uint8_t num_user_registers_;                         // count of user vars in registers

    // Spill management
    std::map<uint16_t, bool> spill_slots_;               // address → in_use
    uint16_t next_spill_addr_;
};
