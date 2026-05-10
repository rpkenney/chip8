#include "register_allocator.h"
#include <stdexcept>

RegisterAllocator::RegisterAllocator()
    : num_user_registers_(0), next_temp_id_(0), next_spill_addr_(SPILL_BASE) {
    // Initialize all registers as available for temporaries
    for (uint8_t i = 0; i < TOTAL_REGISTERS; ++i) {
        available_temp_registers_.insert(i);
    }
}

uint8_t RegisterAllocator::allocateUserVariable(const std::string& var_name) {
    if (user_variables_reg_.count(var_name) || user_variables_spill_.count(var_name)) {
        throw std::runtime_error("Variable already allocated: " + var_name);
    }

    // Try to allocate a register first
    if (num_user_registers_ < TOTAL_REGISTERS) {
        uint8_t reg = num_user_registers_++;
        user_variables_reg_[var_name] = reg;
        available_temp_registers_.erase(reg);
        return reg;
    }

    // No registers available; spill to memory
    uint16_t spill_addr = allocateSpill();
    user_variables_spill_[var_name] = spill_addr;
    return 0xFF;  // Return sentinel indicating spilled (caller won't use this directly)
}

uint8_t RegisterAllocator::allocateTemporary() {
    // Try to allocate a register first
    if (!available_temp_registers_.empty()) {
        uint8_t reg = *available_temp_registers_.begin();
        available_temp_registers_.erase(reg);
        active_temps_[reg] = 0;  // 0 means in register, not spilled
        return reg;
    }

    // No registers available; spill to memory
    int temp_id = next_temp_id_++;
    uint16_t spill_addr = allocateSpill();
    spilled_temps_[temp_id] = spill_addr;
    
    // Return a sentinel value (>14) to indicate this temp is spilled
    // Caller should use getSpilledTemporaryAddress(temp_id) to get actual address
    return 0xFF;  // Sentinel for "spilled"
}

void RegisterAllocator::freeTemporary(uint8_t reg) {
    if (reg == 0xFF) {
        throw std::runtime_error("Attempt to free spilled temporary (use freeSpilledTemporary instead)");
    }
    
    if (active_temps_.count(reg) == 0) {
        throw std::runtime_error("Attempt to free register that is not allocated: V" +
                                 std::to_string(reg));
    }

    active_temps_.erase(reg);

    // Don't return to temp pool if it's a user variable register
    if (reg >= num_user_registers_) {
        available_temp_registers_.insert(reg);
    }
}

bool RegisterAllocator::isUserVariable(const std::string& var_name) const {
    return user_variables_reg_.count(var_name) > 0 || user_variables_spill_.count(var_name) > 0;
}

bool RegisterAllocator::isUserVariableInRegister(const std::string& var_name) const {
    return user_variables_reg_.count(var_name) > 0;
}

bool RegisterAllocator::isUserVariableSpilled(const std::string& var_name) const {
    return user_variables_spill_.count(var_name) > 0;
}

uint8_t RegisterAllocator::getUserVariableRegister(const std::string& var_name) const {
    auto it = user_variables_reg_.find(var_name);
    if (it == user_variables_reg_.end()) {
        throw std::runtime_error("Variable not in register (may be spilled): " + var_name);
    }
    return it->second;
}

uint16_t RegisterAllocator::getUserVariableSpillAddress(const std::string& var_name) const {
    auto it = user_variables_spill_.find(var_name);
    if (it == user_variables_spill_.end()) {
        throw std::runtime_error("Variable not spilled: " + var_name);
    }
    return it->second;
}

bool RegisterAllocator::isTemporaryInRegister(uint8_t reg) const {
    // A register is a temporary if it's in available or has an active temp
    if (available_temp_registers_.count(reg) > 0) {
        return false;  // Not active (available for allocation)
    }
    
    // Check if it's an active temporary
    auto it = active_temps_.find(reg);
    if (it != active_temps_.end()) {
        return it->second == 0;  // 0 means in register, not spilled
    }
    
    return false;  // Not a temporary
}

int RegisterAllocator::getNumUserVariables() const {
    return user_variables_reg_.size() + user_variables_spill_.size();
}

int RegisterAllocator::getNumAvailableTemps() const {
    return available_temp_registers_.size();
}

uint16_t RegisterAllocator::allocateSpill() {
    if (next_spill_addr_ >= SPILL_MAX) {
        throw std::runtime_error("Out of spill memory (0xD00–0xDFF)");
    }

    uint16_t addr = next_spill_addr_;
    next_spill_addr_++;
    spill_slots_[addr] = true;

    return addr;
}

void RegisterAllocator::freeSpill(uint16_t address) {
    if (spill_slots_.count(address) == 0) {
        throw std::runtime_error("Attempt to free unknown spill address: 0x" +
                                 std::to_string(address));
    }

    spill_slots_[address] = false;
    // Could reuse slots here in the future (MVP: just allocate sequentially)
}

void RegisterAllocator::reset() {
    user_variables_reg_.clear();
    user_variables_spill_.clear();
    active_temps_.clear();
    available_temp_registers_.clear();
    spilled_temps_.clear();
    spill_slots_.clear();
    num_user_registers_ = 0;
    next_temp_id_ = 0;
    next_spill_addr_ = SPILL_BASE;

    // Re-initialize all registers as available
    for (uint8_t i = 0; i < TOTAL_REGISTERS; ++i) {
        available_temp_registers_.insert(i);
    }
}
