#include "run_metadata.hpp"

namespace core {
namespace bench {

const char* RunStatusToString(RunStatus status) noexcept {
    switch (status) {
        case RunStatus::Valid: return "valid";
        case RunStatus::Invalid: return "invalid";
        default: return "unknown";
    }
}

const char* FailureClassToString(FailureClass fc) noexcept {
    switch (fc) {
        case FailureClass::None: return "none";
        case FailureClass::InsufficientRepetitions: return "insufficient_repetitions";
        case FailureClass::RuntimeError: return "runtime_error";
        case FailureClass::IsolationViolation: return "isolation_violation";
        case FailureClass::Timeout: return "timeout";
        case FailureClass::UserAbort: return "user_abort";
        default: return "unknown";
    }
}

} // namespace bench
} // namespace core
