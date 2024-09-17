import eldarcpp

# index = eldarcpp.Index()
# index.add_document(["president", "obama"])
# index.add_document(["president", "biden"])
# index.add_document(["president", "trump"])
# index.add_document(["obama"])
# index.add_document(["biden"])
# index.add_document(["obama", "kamala"])
# index.save("index.dat")

# index = eldarcpp.Index()
# index.load("index.dat")

# query = eldarcpp.QueryTree("(president OR obama OR biden) AND NOT (trump OR kamala)")
# print(query.to_string())
# result = index.count(query)
# print("Search result:", result)

# result = index.count("(president OR obama OR biden) AND NOT (trump OR kamala)")
# print("Search result:", result)
import eldarcpp


def print_tree_structure(node, indent=""):
    if isinstance(node, eldarcpp.WordNode):
        print(f"{indent}Word: {node.get_word()}")
    elif isinstance(node, eldarcpp.NotNode):
        print(f"{indent}NOT:")
        print_tree_structure(node.get_child(), indent + "  ")
    elif isinstance(node, eldarcpp.BinaryOpNode):
        op = (
            "AND"
            if isinstance(node, eldarcpp.AndNode)
            else "OR" if isinstance(node, eldarcpp.OrNode) else "AND NOT"
        )
        print(f"{indent}{op}:")
        print_tree_structure(node.get_left(), indent + "  ")
        print_tree_structure(node.get_right(), indent + "  ")


# Create and expand the query tree
query_tree = eldarcpp.QueryTree("obama OR president")
print("Original query:", query_tree.to_string())

query_tree.expand("biden")
print("Expanded query:", query_tree.to_string())

# Traverse the tree structure
root = query_tree.get_root()
print("\nTree structure:")
print_tree_structure(root)

# Demonstrate different expansions
operators = ["AND", "OR", "AND NOT"]
for op in operators:
    new_tree = eldarcpp.QueryTree("obama OR president")
    new_tree.expand("biden", op)
    print(f"\nExpanded with {op}:", new_tree.to_string())
    print(f"Tree structure for {op} expansion:")
    print_tree_structure(new_tree.get_root())

# Accessing specific node types
if isinstance(root, eldarcpp.AndNode):
    left = root.get_left()
    right = root.get_right()
    if isinstance(left, eldarcpp.OrNode) and isinstance(right, eldarcpp.WordNode):
        print("\nAccessing specific nodes:")
        print(f"Left OR node: {left.to_string()}")
        print(f"Right word: {right.get_word()}")
