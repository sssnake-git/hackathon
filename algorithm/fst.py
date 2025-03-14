import heapq
from collections import defaultdict

class WFST:
    def __init__(self):
        self.states = set()  # Set of states
        self.transitions = defaultdict(list)  # Adjacency list {state: [(next_state, input, output, weight)]}
        self.initial_state = None
        self.final_states = set()

    def add_transition(self, from_state, to_state, input_sym, output_sym, weight):
        """Add a transition from `from_state` to `to_state` with given input, output, and weight."""
        self.states.update([from_state, to_state])
        self.transitions[from_state].append((to_state, input_sym, output_sym, weight))

    def set_initial_state(self, state):
        """Set the initial state."""
        self.initial_state = state
        self.states.add(state)

    def add_final_state(self, state):
        """Mark a state as final."""
        self.final_states.add(state)
        self.states.add(state)

    def print_wfst(self):
        """Print the transitions of the WFST."""
        print("Weighted Finite-State Transducer:")
        for state, edges in self.transitions.items():
            for next_state, input_sym, output_sym, weight in edges:
                print(f"({state}) --[{input_sym}:{output_sym} | {weight}]--> ({next_state})")
        print(f"Initial state: {self.initial_state}")
        print(f"Final states: {self.final_states}")

    def shortest_path(self):
        """Find the shortest path from the initial state to any final state using Dijkstra's algorithm."""
        if not self.initial_state:
            return None
        
        # Min-heap for Dijkstra’s algorithm (cost, current_state, path)
        heap = [(0, self.initial_state, [])]  # (cost, state, path)
        visited = {}

        while heap:
            cost, state, path = heapq.heappop(heap)

            # If reaching a final state, return the path
            if state in self.final_states:
                return path + [(state, cost)]

            if state in visited and visited[state] <= cost:
                continue
            visited[state] = cost

            for next_state, input_sym, output_sym, weight in self.transitions.get(state, []):
                heapq.heappush(heap, (cost + weight, next_state, path + [(state, input_sym, output_sym, weight)]))

        return None  # No path found

if __name__ == '__main__':
    # Example Usage
    wfst = WFST()
    wfst.set_initial_state(0)
    wfst.add_transition(0, 1, 'a', 'x', 2)
    wfst.add_transition(0, 2, 'b', 'y', 5)
    wfst.add_transition(1, 3, 'c', 'z', 1)
    wfst.add_transition(2, 3, 'd', 'w', 2)
    wfst.add_final_state(3)

    # Print WFST structure
    wfst.print_wfst()

    # Find shortest path from initial state to any final state
    shortest_path = wfst.shortest_path()
    print("Shortest Path:", shortest_path)
