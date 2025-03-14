from collections import defaultdict, deque
import headq

class DAG:
    def __init__(self):
        self.graph = defaultdict(list)  # Adjacency list representation

    def add_edge(self, u, v):
        """Add a directed edge from u to v."""
        self.graph[u].append(v)
        # Ensure v exists in the graph even if it has no outgoing edges
        if v not in self.graph:
            self.graph[v] = []

    def dfs(self, node, visited, result):
        """Perform Depth-First Search (DFS) in a DAG."""
        visited.add(node)
        for neighbor in self.graph[node]:
            if neighbor not in visited:
                self.dfs(neighbor, visited, result)
        result.append(node)  # Store node in topological order

    def topological_sort(self):
        """Return a topological order of DAG nodes using DFS."""
        visited = set()
        result = []
        nodes = list(self.graph.keys())  # **Fix: Avoid modifying dictionary during iteration**
        for node in nodes:
            if node not in visited:
                self.dfs(node, visited, result)
        return result[::-1]  # Reverse to get the correct topological order

    def bfs(self, start):
        """Perform Breadth-First Search (BFS) in a DAG."""
        queue = deque([start])
        visited = set([start])
        bfs_order = []

        while queue:
            node = queue.popleft()
            bfs_order.append(node)
            for neighbor in self.graph[node]:
                if neighbor not in visited:
                    visited.add(neighbor)
                    queue.append(neighbor)
        
        return bfs_order

def test_DAG():
    # Example Usage
    dag = DAG()
    dag.add_edge(5, 0)
    dag.add_edge(5, 2)
    dag.add_edge(4, 0)
    dag.add_edge(4, 1)
    dag.add_edge(2, 3)
    dag.add_edge(3, 1)

    # Topological Sorting
    print("Topological Sort:", dag.topological_sort())
    # Expected Output: [5, 4, 2, 3, 1, 0] (Order may vary)

    # BFS Traversal from Node 5
    print("BFS Traversal:", dag.bfs(5))
    # Expected Output: [5, 0, 2, 3, 1] (Order may vary)

if __name__ == '__main__':
    test_DAG()