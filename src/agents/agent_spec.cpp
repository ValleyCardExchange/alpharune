#include "agent_spec.h"

#include <stdexcept>

namespace riftbound {

const char* toString(EvaluatorKind k) {
    switch (k) {
        case EvaluatorKind::Score:  return "score";
        case EvaluatorKind::Corpus: return "corpus";
    }
    return "score";
}

AgentSpec parseAgentSpec(const std::string& s) {
    AgentSpec out;
    out.raw = s;
    auto colon = s.find(':');
    out.kind = (colon == std::string::npos) ? s : s.substr(0, colon);
    if (out.kind != "random" && out.kind != "human" &&
        out.kind != "mcts" && out.kind != "ismcts") {
        throw std::runtime_error(
            "Unknown agent kind '" + out.kind + "'. Expected one of: "
            "random, human, mcts, ismcts.");
    }
    if (colon != std::string::npos) {
        auto rest = s.substr(colon + 1);
        // key=value pairs separated by commas. Unrecognised keys are
        // ignored (pre-existing behaviour); recognised keys with a bad
        // value are rejected.
        size_t i = 0;
        while (i < rest.size()) {
            auto eq    = rest.find('=', i);
            auto comma = rest.find(',', i);
            if (comma == std::string::npos) comma = rest.size();
            if (eq != std::string::npos && eq < comma) {
                auto key = rest.substr(i, eq - i);
                auto val = rest.substr(eq + 1, comma - eq - 1);
                if (key == "sims") out.sims = std::stoi(val);
                if (key == "eval") {
                    if      (val == "score")  out.eval = EvaluatorKind::Score;
                    else if (val == "corpus") out.eval = EvaluatorKind::Corpus;
                    else {
                        throw std::runtime_error(
                            "Unknown eval '" + val + "' in agent spec '" + s +
                            "'. Expected one of: score, corpus.");
                    }
                }
            }
            i = comma + 1;
        }
        if ((out.kind == "mcts" || out.kind == "ismcts") && out.sims <= 0) {
            throw std::runtime_error(
                "Agent '" + out.kind + "' requires sims=N (e.g. " + out.kind + ":sims=50).");
        }
    }
    return out;
}

}  // namespace riftbound
