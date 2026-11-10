#include "graph.h"

#include <algorithm>
#include <set>
#include <utility>

namespace {

enum class State { InProgress, Done };

/// Элемент стека обхода: пакет и номер следующей зависимости.
struct Frame {
    std::string node;
    std::size_t next;
};

}  // namespace

const std::vector<std::string>& Graph::children(const std::string& name) const {
    static const std::vector<std::string> none;
    auto found = edges.find(name);
    return found == edges.end() ? none : found->second;
}

Graph buildGraph(const std::string& root, DependencySource& source) {
    Graph graph;
    graph.root = root;
    graph.nodes.push_back(root);
    graph.edges[root] = source.dependencies(root);
    std::map<std::string, State> state = {{root, State::InProgress}};
    std::vector<Frame> stack = {{root, 0}};
    while (!stack.empty()) {
        Frame& top = stack.back();
        const std::vector<std::string>& children = graph.edges[top.node];
        if (top.next == children.size()) {
            state[top.node] = State::Done;
            stack.pop_back();
            continue;
        }
        std::string child = children[top.next++];
        auto known = state.find(child);
        if (known != state.end() && known->second == State::InProgress) {
            auto start = std::find_if(stack.begin(), stack.end(),
                                      [&child](const Frame& frame) { return frame.node == child; });
            std::vector<std::string> cycle;
            for (auto it = start; it != stack.end(); ++it) {
                cycle.push_back(it->node);
            }
            cycle.push_back(child);
            graph.cycles.push_back(cycle);
            continue;
        }
        if (known != state.end()) {
            continue;
        }
        state[child] = State::InProgress;
        graph.nodes.push_back(child);
        graph.edges[child] = source.dependencies(child);
        stack.push_back({child, 0});
    }
    for (const std::string& name : graph.nodes) {
        graph.labels[name] = source.label(name);
    }
    return graph;
}

std::vector<std::string> loadOrder(const Graph& graph) {
    std::vector<std::string> order;
    std::set<std::string> seen = {graph.root};
    std::vector<Frame> stack = {{graph.root, 0}};
    while (!stack.empty()) {
        Frame& top = stack.back();
        const std::vector<std::string>& children = graph.children(top.node);
        if (top.next == children.size()) {
            order.push_back(top.node);
            stack.pop_back();
            continue;
        }
        const std::string& child = children[top.next++];
        if (seen.insert(child).second) {
            stack.push_back({child, 0});
        }
    }
    return order;
}
